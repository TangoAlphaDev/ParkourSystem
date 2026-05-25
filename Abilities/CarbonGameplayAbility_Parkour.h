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
	// The constructor. This configures the net execution policy, security policy and instancing policy for proper multiplayer behaviour
	UCarbonGameplayAbility_Parkour();

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

private:
	// Helper to pick the montage to play for a given parkour type based on data asset
	UAnimMontage* SelectMontageForParkourType(
		ECarbonParkourType ParkourType,
		const FGameplayAbilityActivationInfo& ActivationInfo) const;

	// Helper to determine if we should change movement mode
	static bool ShouldChangeMovementMode(const FGameplayAbilityActorInfo* ActorInfo);
};
