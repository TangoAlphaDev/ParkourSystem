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

	// Montage to play for this parkour type
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> Montage;

	// Name of the warp targets (must match anim notify / montage)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName WarpTargetStartName = "ParkourTargetStart";

	UPROPERTY(EditAnywhere,BlueprintReadOnly)
	FName WarpTargetTicTacName = "ParkourTargetTicTac";

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName WarpTargetMidName = "ParkourTargetMid";

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName WarpTargetEndName = "ParkourTargetEnd";
};

UCLASS()
class UCarbonParkourDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FCarbonParkourMove> Moves;
};
