// Copyright Tango Alpha LLC. All Rights Reserved.

#pragma once

#include "CarbonParkourTypes.h"

#include "CarbonParkourSolution.generated.h"

USTRUCT(BlueprintType)
struct FCarbonParkourSolution
{
	GENERATED_BODY()

public:
	// Validated parkour bool
	UPROPERTY(BlueprintReadOnly)
	bool bIsValid = false;

	// Type of parkour move selected
	UPROPERTY(BlueprintReadOnly)
	ECarbonParkourType ParkourType = ECarbonParkourType::None;

	// Name of the motion warp targets
	UPROPERTY(BlueprintReadOnly)
	FName WarpTargetStart = NAME_None;

	UPROPERTY(BlueprintReadOnly)
	FName WarpTargetMiddle = NAME_None;

	UPROPERTY(BlueprintReadOnly)
	FName WarpTargetEnd = NAME_None;

	// Transform start location
	UPROPERTY(BlueprintReadOnly)
	FVector TargetTransformStart = FVector::ZeroVector;

	// Transform middle location (if needed)
	UPROPERTY(BlueprintReadOnly)
	FVector TargetTransformMid = FVector::ZeroVector;

	// Transform end location
	UPROPERTY(BlueprintReadOnly)
	FVector TargetTransformEnd = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	FTransform WarpTransformStart = FTransform::Identity;

	UPROPERTY(BlueprintReadOnly)
	FTransform WarpTransformMid = FTransform::Identity;

	UPROPERTY(BlueprintReadOnly)
	FTransform WarpTransformEnd = FTransform::Identity;

	// Distance to the obstacle at chosen sample point
	UPROPERTY(BlueprintReadOnly)
	float ObstacleDistance = 0.0f;

	// Height of the obstacle at chosen sample point
	UPROPERTY(BlueprintReadOnly)
	float ObstacleHeight = 0.0f;

	// Length of the obstacle at chosen sample point
	UPROPERTY(BlueprintReadOnly)
	float ObstacleLength = 0.0f;

	// Normal of the surface being interacted with
	UPROPERTY(BlueprintReadOnly)
	FVector SurfaceNormal = FVector::ZeroVector;

	// Tic-Tac vault only data
	UPROPERTY(BlueprintReadOnly)
	FName WarpTargetTicTac = NAME_None;
	
	UPROPERTY(BlueprintReadOnly)
	FTransform WarpTransformTicTac = FTransform::Identity;

	UPROPERTY(BlueprintReadOnly)
	bool bLeftSideHit = false;

	UPROPERTY(BlueprintReadOnly)
	bool bRightSideHit = false;

	UPROPERTY(BlueprintReadOnly)
	FRotator LeftSideRotation = FRotator::ZeroRotator;

	UPROPERTY(BlueprintReadOnly)
	FRotator RightSideRotation = FRotator::ZeroRotator;

	// Bool to select movement state after object
	UPROPERTY(BlueprintReadOnly)
	bool bShouldFall = false;

	// Reset all values
	void Reset()
	{
		bIsValid = false;
		ParkourType = ECarbonParkourType::None;
		WarpTargetStart = NAME_None;
		WarpTargetMiddle = NAME_None;
		WarpTargetEnd = NAME_None;
		WarpTargetTicTac = NAME_None;
		TargetTransformStart = FVector::ZeroVector;
		TargetTransformMid = FVector::ZeroVector;
		TargetTransformEnd = FVector::ZeroVector;
		WarpTransformStart = FTransform::Identity;
		WarpTransformMid = FTransform::Identity;
		WarpTransformEnd = FTransform::Identity;
		WarpTransformTicTac = FTransform::Identity;
		
		ObstacleDistance = 0.0f;
		ObstacleHeight = 0.0f;
		SurfaceNormal = FVector::ZeroVector;
	}
};