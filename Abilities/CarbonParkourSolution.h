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

	// Vault data
	// Name of the motion warp target
	UPROPERTY(BlueprintReadOnly)
	FName WarpTargetLedge = NAME_None;

	// Transform location
	UPROPERTY(BlueprintReadOnly)
	FVector TargetTransformLedge = FVector::ZeroVector;
	
	// Warp transform for motion warping
	UPROPERTY(BlueprintReadOnly)
	FTransform WarpTransformLedge = FTransform::Identity;

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
	FVector TargetTransformTicTac = FVector::ZeroVector;
	
	UPROPERTY(BlueprintReadOnly)
	bool bLeftTicTacHit = false;

	UPROPERTY(BlueprintReadOnly)
	bool bRightTicTacHit = false;

	UPROPERTY(BlueprintReadOnly)
	FVector SideSurfaceNormal = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	FRotator LeftTicTacRotation = FRotator::ZeroRotator;

	UPROPERTY(BlueprintReadOnly)
	FRotator RightTicTacRotation = FRotator::ZeroRotator;

	// Wall Run Data
	UPROPERTY(BlueprintReadOnly)
	FName WarpTargetWallRun = NAME_None;

	UPROPERTY(BlueprintReadOnly)
	FVector TargetTransformWallRun = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	FTransform WarpTransformWallRun = FTransform::Identity;

	UPROPERTY(BlueprintReadOnly)
	bool bLeftRunHit = false;

	UPROPERTY(BlueprintReadOnly)
	bool bRightRunHit = false;

	UPROPERTY(BlueprintReadOnly)
	FVector RunSurfaceNormal = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	FRotator LeftRunRotation = FRotator::ZeroRotator;

	UPROPERTY(BlueprintReadOnly)
	FRotator RightRunRotation = FRotator::ZeroRotator;

	// Bool to select movement state after object
	UPROPERTY(BlueprintReadOnly)
	bool bShouldFall = false;

	// Reset all values
	void Reset()
	{
		bIsValid = false;
		ParkourType = ECarbonParkourType::None;

		WarpTargetLedge = NAME_None;
		WarpTargetTicTac = NAME_None;
		WarpTargetWallRun = NAME_None;

		TargetTransformLedge = FVector::ZeroVector;
		TargetTransformTicTac = FVector::ZeroVector;
		TargetTransformWallRun = FVector::ZeroVector;

		WarpTransformLedge = FTransform::Identity;
		WarpTransformTicTac = FTransform::Identity;
		WarpTransformWallRun = FTransform::Identity;

		ObstacleDistance = 0.0f;
		ObstacleHeight = 0.0f;
		ObstacleLength = 0.0f;

		SurfaceNormal = FVector::ZeroVector;
		SideSurfaceNormal = FVector::ZeroVector;
		RunSurfaceNormal = FVector::ZeroVector;

		bLeftTicTacHit = false;
		bRightTicTacHit = false;
		LeftTicTacRotation = FRotator::ZeroRotator;
		RightTicTacRotation = FRotator::ZeroRotator;

		bLeftRunHit = false;
		bRightRunHit = false;
		LeftRunRotation = FRotator::ZeroRotator;
		RightRunRotation = FRotator::ZeroRotator;

		bShouldFall = false;
	}
};