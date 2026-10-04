// Fill out your copyright notice in the Description page of Project Settings.


#include "TL_Character.h"
#include "AbilitySystemComponent.h"

// Sets default values
ATL_Character::ATL_Character()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create ability system component, and set it to be explicitly replicated
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);

	// Minimal Mode means that no GameplayEffects will replicate. They will only live on the Server. Attributes, GameplayTags, and GameplayCues will still replicate to us.
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	// Create the attribute set, this replicates by default
	// Adding it as a subobject of the owning actor of an AbilitySystemComponent
	// automatically registers the AttributeSet with the AbilitySystemComponent
	//AttributeSetBase = CreateDefaultSubobject<USP_BaseAttributes>(TEXT("AttributeSetBase"));


	SetReplicateMovement(true);

	SetNetUpdateFrequency(60.0f);
	bReplicates = true;
	NetPriority = 3.0f;

}

UAbilitySystemComponent* ATL_Character::GetAbilitySystemComponent() const
{
	if(AbilitySystemComponent)
	{
		return AbilitySystemComponent;
	}

	return nullptr; // Replace with your actual ability system component if you have one
}

void ATL_Character::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
	if (GetAbilitySystemComponent())
	{
		TagContainer.AppendTags(GetAbilitySystemComponent()->GetOwnedGameplayTags());
		return;
	}
	return;
}

// Called when the game starts or when spawned
void ATL_Character::BeginPlay()
{
	Super::BeginPlay();
	
}

void ATL_Character::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
}

void ATL_Character::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
}

void ATL_Character::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
}

// Called every frame
void ATL_Character::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ATL_Character::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

