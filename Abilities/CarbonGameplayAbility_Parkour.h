// Copyright Tango Alpha LLC. All Rights Reserved.

#pragma once

#include "LyraGameplayAbility.h"
#include "GameplayEffectTypes.h"
#include "CarbonParkourDataAsset.h"

#include "CarbonGameplayAbility_Parkour.generated.h"

UCLASS()
class LYRAGAME_API UCarbonGameplayAbility_Parkour : public ULyraGameplayAbility
{
	GENERATED_BODY()

public:
	// Ability activation
	virtual void ActivateAbility(
	  const FGameplayAbilitySpecHandle Handle,
	  const FGameplayAbilityActorInfo* ActorInfo,
	  const FGameplayAbilityActivationInfo ActivationInfo,
	  const FGameplayEventData* TriggerEventData) override;

protected:
	UFUNCTION()
	void ApplyWarpTargets(ALyraCharacter* Character, const FCarbonParkourSolution& Solution);

	UPROPERTY(EditDefaultsOnly, Category="Parkour")
	TObjectPtr<UCarbonParkourDataAsset> ParkourData = nullptr;

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageCancelled();

	bool AbilityFalling = false;
};
