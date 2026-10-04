// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySysteminterface.h"
#include "GameplayTagAssetInterface.h"
#include "TL_Character.generated.h"

class UAbilitySystemComponent;

UCLASS(Blueprintable, BlueprintType)
class TEACHINGLAB_API ATL_Character : public ACharacter, public IAbilitySystemInterface, public IGameplayTagAssetInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ATL_Character();

	// Implement IAbilitySystemInterface
	virtual class UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	//Implement GameplayTagAssetInterface, this will get this characters AbilitySystemComponent and return OwnedTags from that.
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void PossessedBy(AController* NewController) override;

	virtual void OnRep_PlayerState() override;

	virtual void PostInitializeComponents() override;


	UPROPERTY()
	class UAbilitySystemComponent* AbilitySystemComponent;


public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
