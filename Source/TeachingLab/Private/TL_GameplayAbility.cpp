// Fill out your copyright notice in the Description page of Project Settings.


#include "TL_GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"

void UTL_GameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{

	if (TriggerEventData && bHasBlueprintActivateFromEvent)
	{
		//Checks if we have Event activation event in the blueprint graph.
		K2_ActivateAbilityFromEvent(*TriggerEventData);
		if (ActorInfo->IsLocallyControlledPlayer())
		{
			ActivateLocalPlayerAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

		}
		if (ActorInfo->IsNetAuthority())
		{
			ActivateServerAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
		}
		return;
	}
	else if (bHasBlueprintActivate)
	{
		// A Blueprinted ActivateAbility function must call CommitAbility somewhere in its execution chain.
		K2_ActivateAbility();

		if (ActorInfo->IsLocallyControlledPlayer())
		{
			ActivateLocalPlayerAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

		}
		if (ActorInfo->IsNetAuthority())
		{
			ActivateServerAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
		}
		return;
	}

	//copy of epics catch case for invalid activation data when using event activation
	else if (bHasBlueprintActivateFromEvent)
	{
		UE_LOG(LogTemp, Warning, TEXT("Ability %s expects event data but none is being supplied. Use 'Activate Ability' instead of 'Activate Ability From Event' in the Blueprint."), *GetName());
		constexpr bool bReplicateEndAbility = false;
		constexpr bool bWasCancelled = true;
		EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	}

}

void UTL_GameplayAbility::ActivateServerAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{

	//blueprint event call
	K2_ActivateServerAbility();

	//safety ASC valid check, the check macro doesn't get compiled into shipping builds but will crash a dev build, which is the point.
	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	check(ASC);

	//Setting the notify delegate in the AbilitySystemComponent.
	NotifyTargetDataReadyDelegateHandle = ASC->AbilityTargetDataSetDelegate(Handle, ActivationInfo.GetActivationPredictionKey()).AddUObject(this, &ThisClass::NotifyTargetDataReady);


}

//This is called from inside our NotifyTargetDataReady function, implement any logic here that can't be done in blueprint
void UTL_GameplayAbility::RecievedCustomTargetData(const FGameplayAbilityTargetDataHandle& TargetDataHandle, FGameplayTag ApplicationTag)
{

	//blueprint event call
	K2_RecievedCustomTargetData(TargetDataHandle, ApplicationTag);
}

//this is the magic function, gets called from SetAndNotifyCustomData,sets up our GAS prediction window and sends our target data. Is there a better way? probably.
void UTL_GameplayAbility::NotifyTargetDataReady(const FGameplayAbilityTargetDataHandle& InData, FGameplayTag ApplicationTag)
{
	//safety ASC valid check, the check macro doesn't get compiled into shipping builds but will crash a dev build, which is the point.
	UAbilitySystemComponent* ASC = CurrentActorInfo->AbilitySystemComponent.Get();
	check(ASC);

	// [xist] is this (from Lyra) like an "if is handle valid?" check? seems so, keeping it as such.
	if (!ASC->FindAbilitySpecFromHandle(CurrentSpecHandle))
	{
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false);  // do not replicate
		return;
	}


	// true if we need to replicate this target data to the server
	const bool bShouldNotifyServer = CurrentActorInfo->IsLocallyControlled() && !CurrentActorInfo->IsNetAuthority();

	// Start a scoped prediction window
	FScopedPredictionWindow	ScopedPrediction(ASC);

	// Lyra does this memcopy operation; const cast paranoia is real. We'll keep it.
	// Take ownership of the target data to make sure no callbacks into game code invalidate it out from under us
	const FGameplayAbilityTargetDataHandle LocalTargetDataHandle(MoveTemp(const_cast<FGameplayAbilityTargetDataHandle&>(InData)));

	// if this isn't the local player on the server, then notify the server
	if (bShouldNotifyServer)
	{
		ASC->CallServerSetReplicatedTargetData(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey(), LocalTargetDataHandle, ApplicationTag, ASC->ScopedPredictionKey);
	}

	// Execute the ability we've now successfully committed
	RecievedCustomTargetData(LocalTargetDataHandle, ApplicationTag);

	// We've processed the data, clear it from the RPC buffer
	ASC->ConsumeClientReplicatedTargetData(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());
}


void UTL_GameplayAbility::SetAndNotifyCustomData(const TArray<FName>& Keys, const TArray<FVector>& InVectors, const TArray<FRotator>& InRotators, const TArray<AActor*>& TargetActors, const FGameplayTag& ApplicationTag)
{

	FGameplayAbilityTargetData_CustomTargetData* TargetData = new FGameplayAbilityTargetData_CustomTargetData();//Makes new Target Data from our struct


	TargetData->TargetActorArray = TargetActors;
	TargetData->Vectors = InVectors;
	TargetData->Rotators = InRotators;
	TargetData->Keys = Keys;


	FGameplayAbilityTargetDataHandle TargetDataHandle;

	//Adds the target data to the handle that gets passed around.  Can add more than 1 target data to a handle using the same Add()
	TargetDataHandle.Add(TargetData);

	//Pass the handle to our custom Notify ready function
	NotifyTargetDataReady(TargetDataHandle, ApplicationTag);
}

void UTL_GameplayAbility::BreakDataFromCustomTargetData(const FGameplayAbilityTargetDataHandle& Handle, const int Index, TArray<FName>& Keys, TArray<FVector>& Vectors, TArray<FRotator>& Rotators, TArray<AActor*>& Actors)
{

	const FGameplayAbilityTargetData* NewData = Handle.Get(Index);

	// make sure we have some kind of valid data value.
	if (NewData)
	{
		//Pull our own information from the data struct
		if (NewData->GetScriptStruct() == FGameplayAbilityTargetData_CustomTargetData::StaticStruct())
		{

			const FGameplayAbilityTargetData_CustomTargetData* CustomData = static_cast<const FGameplayAbilityTargetData_CustomTargetData*>(NewData);

			Vectors = CustomData->Vectors;
			Rotators = CustomData->Rotators;
			Keys = CustomData->Keys;
			Actors = CustomData->TargetActorArray;


		}
	}

}

void UTL_GameplayAbility::EndAbilityCleanup(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	//safety ASC valid check, the check macro doesn't get compiled into shipping builds but will crash a dev build, which is the point.
	UAbilitySystemComponent* ASC = CurrentActorInfo->AbilitySystemComponent.Get();
	check(ASC);

	// Clears the delegates we made for the AbilitySystemComponent
	ASC->AbilityTargetDataSetDelegate(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey()).Remove(NotifyTargetDataReadyDelegateHandle);
	ASC->ConsumeClientReplicatedTargetData(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());

}


void UTL_GameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicatedEndAbility, bool bWasCancelled)
{

	EndAbilityCleanup(Handle, ActorInfo, ActivationInfo, bReplicatedEndAbility, bWasCancelled);
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicatedEndAbility, bWasCancelled);

}