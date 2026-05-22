// Copyright Tango Alpha LLC. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "CarbonParkourTypes.h"

#include "CarbonParkourDataAsset.generated.h"

class UAnimMontage;

USTRUCT(BlueprintType)
struct FCarbonParkourMove
{
	GENERATED_BODY()

	// The parkour type this move is for (e.g., VaultShort, ClimbUpHigh)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ECarbonParkourType ParkourType = ECarbonParkourType::None;

	// Array of montages to play for this parkour type
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<TObjectPtr<UAnimMontage>> Montages;
};

UCLASS()
class UCarbonParkourDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FCarbonParkourMove> Moves;
};
