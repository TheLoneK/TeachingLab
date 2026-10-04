// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "TL_GameplayAbility.generated.h"

/**
 * 
 */

 //Custom target data struct that adds our own arrays
USTRUCT(BlueprintType)
struct TEACHINGLAB_API FGameplayAbilityTargetData_CustomTargetData : public FGameplayAbilityTargetData
{
	GENERATED_USTRUCT_BODY()
public:

	FGameplayAbilityTargetData_CustomTargetData()
	{
	}

	UPROPERTY(EditAnywhere, Category = Targeting)
	TArray<TObjectPtr<AActor>> TargetActorArray;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FName> Keys;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FRotator> Rotators;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FVector> Vectors;

	UPROPERTY()
	FPredictionKey PredictionKeyData;

	// This is required for all child structs of FGameplayAbilityTargetData
	virtual UScriptStruct* GetScriptStruct() const override
	{
		return FGameplayAbilityTargetData_CustomTargetData::StaticStruct();
	}

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)

	{
		//use 31 for max array size, 16 if you want less network data, anything less is probably not worth it.
		PredictionKeyData.NetSerialize(Ar, Map, bOutSuccess);
		SafeNetSerializeTArray_Default<31>(Ar, TargetActorArray);
		SafeNetSerializeTArray_Default<31>(Ar, Keys);
		SafeNetSerializeTArray_Default<31>(Ar, Rotators);
		SafeNetSerializeTArray_Default<31>(Ar, Vectors);
		

		bOutSuccess = true;
		return true;
	}

};

template<>
struct TStructOpsTypeTraits<FGameplayAbilityTargetData_CustomTargetData> : public TStructOpsTypeTraitsBase2<FGameplayAbilityTargetData_CustomTargetData>
{
	enum
	{
		WithNetSerializer = true // This is REQUIRED for FGameplayAbilityTargetDataHandle net serialization to work
	};
};


UCLASS()
class TEACHINGLAB_API UTL_GameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:

	//custom data setter function  
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Keys, InVectors, InRotators, TargetActors, ApplicationTag", Keywords = "LBTargetData, SetLBTargetData, TargetData, CustomTargetData, ClientSetTarget, Notify"))
	virtual void SetAndNotifyCustomData(const TArray<FName>& Keys, const TArray<FVector>& InVectors, const TArray<FRotator>& InRotators, const TArray<AActor*>& TargetActors, const FGameplayTag& ApplicationTag);

	//Custom helper function that breaks down and retrieves the custom values in a target data handle
	UFUNCTION(BlueprintPure)
	void BreakDataFromCustomTargetData(const FGameplayAbilityTargetDataHandle& Handle, const int Index, TArray<FName>& Keys, TArray<FVector>& Vectors, TArray<FRotator>& Rotators, TArray<AActor*>& Actors);


protected:

	//virtual void CancelAbility(const FGameplayAbilitySpecHandle CurrentSpecHandle, const FGameplayAbilityActorInfo* CurrentActorInfo, const FGameplayAbilityActivationInfo CurrentActivationInfo, bool bWasCancelled) override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;


	// Local player activation
	virtual void ActivateLocalPlayerAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
	{
		// Safely dereference pointers or pass defaults if they are null
		FGameplayEventData SafeEventData = TriggerEventData ? *TriggerEventData : FGameplayEventData();

		// Forward the parameters to the Blueprint event
		K2_ActivateLocalPlayerAbility(Handle, ActivationInfo, SafeEventData);
	}

	// Blueprint local player implementation
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, DisplayName = "Activate Local Player Ability")
	void K2_ActivateLocalPlayerAbility(FGameplayAbilitySpecHandle Handle,FGameplayAbilityActivationInfo ActivationInfo,const FGameplayEventData& TriggerEventData);


	// Server ability activation
	virtual void ActivateServerAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData);

	// blueprint server activation implementation
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, DisplayName = "Activate Server Ability")
	void K2_ActivateServerAbility();


	//Target Data Activation, unimplemented in c++, expected to be overidden.
	void RecievedCustomTargetData(const FGameplayAbilityTargetDataHandle& TargetDataHandle, FGameplayTag ApplicationTag);

	//Blueprint Target data implementation
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "RecievedCustomTargetData")
	void K2_RecievedCustomTargetData(const FGameplayAbilityTargetDataHandle& TargetDataHandle, FGameplayTag ApplicationTag);


	//nofities the server and sends the custom target data to the waiting server, no validation happens here just passing info
	UFUNCTION(BlueprintCallable)
	virtual void NotifyTargetDataReady(const FGameplayAbilityTargetDataHandle& InData, FGameplayTag ApplicationTag);

	//custom ability ending that cleans up the delegate handle and anything we set up in the ASC
	void EndAbilityCleanup(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled);

	// EndAbility override
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicatedEndAbility, bool bWasCancelled) override;


private:

	//Delegate handle for notify target data
	FDelegateHandle NotifyTargetDataReadyDelegateHandle;

};
