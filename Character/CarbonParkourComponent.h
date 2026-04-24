// Copyright Tango Alpha LLC. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/CarbonParkourSolution.h"
#include "AbilitySystem/Abilities/CarbonParkourTypes.h"

#include "CarbonParkourComponent.generated.h"

UCLASS()
class LYRAGAME_API UCarbonParkourComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	
	// Run traces for parkour detection
	UFUNCTION(BlueprintCallable, Category="Carbon|Parkour")
	void VaultSolution();

	// Build parkour solution based on trace results
	void BuildParkourSolution();

	// Get the cached parkour solution to use in the ability
	const FCarbonParkourSolution& GetCachedParkourSolution() const;

	// Turn on and off debug view
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Parkour|Debug")
	bool bDebugTraces = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Parkour|Debug")
	bool bDebugSpheres = false;

protected:

	// Traces for Parkour detection parameters
	// Max distance to search for an obstacle
	UPROPERTY(EditAnywhere, Category="Parkour|Trace")
	float DefaultTraceDistance = 200.f;

	UPROPERTY(EditAnywhere, Category="Parkour|Trace")
	float RunningTraceDistance = 350.f;

	UPROPERTY(EditAnywhere, Category="Parkour|Trace")
	float DepthTraceDistance = 325.f;

	UPROPERTY(EditAnywhere, Category="Parkour|Trace")
	float SideTraceDistance = 150.f;

	// Speed required to trigger running traces
	UPROPERTY(EditAnywhere, Category="Parkour|Trace")
	float SpeedThreshold = 150.f;

	// Tic-Tac adjustment distance threshold
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Parkour|Trace")
	float SideOffsetAdjustment = 80.0f;

	// Landing distance
	UPROPERTY(EditAnywhere, Category="Parkour|Trace")
	float LandTraceDistance = 150.f;

	// Height above character to start downward traces
	UPROPERTY(EditAnywhere, Category="Parkour|Trace")
	float DownTraceStartHeight = 500.f;

	// How far down we trace to find ground
	UPROPERTY(EditAnywhere, Category="Parkour|Trace")
	float DownTraceDepth = 600.f;

	// Vault height distances
	// Depth thresholds (for tuning)
	UPROPERTY(EditAnywhere, Category="Parkour|Vault")
	float ObjectDepthShort = 50.f;

	UPROPERTY(EditAnywhere, Category="Parkour|Vault")
	float ObjectDepthMedium = 201.f;

	UPROPERTY(EditAnywhere, Category="Parkour|Vault")
	float ObjectDepthLong = 305.f;
	
	// Height thresholds (for tuning)
	UPROPERTY(EditAnywhere, Category="Parkour|Vault")
	float MaxStepHeight = 60.f;

	UPROPERTY(EditAnywhere, Category="Parkour|Vault")
	float MaxVaultHeight = 150.f;

	UPROPERTY(EditAnywhere, Category="Parkour|Vault")
	float MaxHighVaultHeight = 201.f;

	UPROPERTY(EditAnywhere, Category="Parkour|Vault")
	float MaxClimbObjectHeight = 301.f;
	
	UPROPERTY(EditAnywhere, Category="Parkour|Vault")
	float MaxDoubleClimbObjectHeight = 400.f;

private:

	// Functions to run individual traces
	void DepthTrace();

	void HeightTrace(const FVector& StartLocation, FHitResult& CachedHitResult, FColor DebugColor);
	
	void RunHeightTraces();
	
	bool SideTrace(bool bLeftSide);

	// Function to get ground offset to adjust for character height in traces
	float GetGroundOffset();

	// Function to validate side hit based on normals
	void ValidateSideHits();

	// Function to determine trace distance based on character speed
	float GetTraceDistance() const;

	// Function to pick vault type based on trace results
	ECarbonParkourType ClassifyParkourType();

	// Functions to cache Tic-Tac results to build parkour solution
	void AdjustCachedTicTacLocations(float SideOffsetAdjustment);

	// Capsule shape
	float TraceCapsuleHalfHeight = 200.f;
	float TraceCapsuleRadius = 1.f;

	// Cached trace results
	FHitResult CachedForwardHit;
	FHitResult CachedBackwardHit;
	FHitResult CachedInitHeightHit;
	FHitResult CachedEndHeightHit; 
	FHitResult CachedLandHeightHit;

	// Side traces
	FHitResult CachedLeftSideHit;
	FHitResult CachedRightSideHit;

	// Bool to check if tic-tac is within distance
	bool bTicTacHitDistance = false;
	
	// Cached adjusted locations for side traces
	UE::Math::TRotator<double> CachedLeftSideRotation;
	UE::Math::TRotator<double> CachedRightSideRotation;

	FVector CachedTicTacHitLocation;

	// Bool for setting tic-tac offset
	bool bTicTacOffset;

	// Cached obstacle measurements
	TArray<float> CachedObjectHeights;

	// Cached obstacle depth (distance from forward trace hit to backward trace hit)
	float CachedObjectDepth = 0.f;

	// Cached ground offset
	float CachedGroundOffset = 0.f;
	
	// Cached parkour type
	ECarbonParkourType CachedParkourType = ECarbonParkourType::None;

	// Cached parkour solution data
	FCarbonParkourSolution CachedParkourSolution;
};
