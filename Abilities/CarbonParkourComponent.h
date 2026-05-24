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
	void VaultSolution();

	// Build parkour solution based on trace results
	void BuildParkourSolution();

	// Get the cached parkour solution to use in the ability
	const FCarbonParkourSolution& GetCachedParkourSolution() const;

	// Turn on and off debug view
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Parkour|Debug")
	bool bDebugTraces = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Parkour|Debug")
	bool bDebugSpheres = true;

	// Turn on or off motion warping
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Parkour|MotionWarping")
	bool bEnableMotionWarping = true;

protected:

	// Traces for Parkour detection parameters
	// Max distance to search for an obstacle
	UPROPERTY(EditAnywhere, Category="Parkour|Trace")
	float DefaultTraceDistance = 200.f;

	UPROPERTY(EditAnywhere, Category="Parkour|Trace")
	float RunningTraceDistance = 350.f;

	UPROPERTY(EditAnywhere, Category="Parkour|Trace")
	float DepthTraceDistance = 450.f;

	UPROPERTY(EditAnywhere, Category="Parkour|Trace")
	float SideTraceDistance = 150.f;

	UPROPERTY(EditAnywhere, Category="Parkour|Trace")
	float WallSideTraceDistance = 200.f;

	UPROPERTY(EditAnywhere, Category="Parkour|Trace")
	float WallRunTraceDistance = 575.f;

	// Speed required to trigger running traces
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Parkour|Trace")
	float SpeedThreshold = 450.f;

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
	float ObjectDepthXShort = 35.f;
	
	UPROPERTY(EditAnywhere, Category="Parkour|Vault")
	float ObjectDepthShort = 60.f;

	UPROPERTY(EditAnywhere, Category="Parkour|Vault")
	float ObjectDepthMedium = 100.f;

	UPROPERTY(EditAnywhere, Category="Parkour|Vault")
	float ObjectDepthLong = 250.f;
	
	// Height thresholds (for tuning)
	UPROPERTY(EditAnywhere, Category="Parkour|Vault")
	float MinHeight = 50.f;

	UPROPERTY(EditAnywhere, Category="Parkour|Vault")
	float MaxVaultHeight = 100.f;

	UPROPERTY(EditAnywhere, Category="Parkour|Vault")
	float MaxHighVaultHeight = 250.f;

	UPROPERTY(EditAnywhere, Category="Parkour|Vault")
	float MaxClimbObjectHeight = 350.f;
	
	UPROPERTY(EditAnywhere, Category="Parkour|Vault")
	float MaxDoubleClimbObjectHeight = 500.f;

	// Warp adjustment values
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Parkour|Adjustment")
	float SideOffsetAdjustment = 85.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Parkour|Adjustment")
	float LedgeOffsetYAdjustment = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Parkour|Adjustment")
	float LedgeOffsetZAdjustment = 125.0f;

private:

	// Functions to run individual traces
	void DepthTrace();

	void HeightTrace(const FVector& StartLocation, FHitResult& CachedHitResult);
	
	void RunHeightTraces();
	
	bool TicTacTrace(bool bLeftSide);

	bool WallRunTrace(bool bLeftSide);

	void WallRunValidationTrace(const FVector& EndLocation, const FVector& RunEndLocation);

	// Function to clear cache
	void ClearCachedTraces();

	// Function to get character speed
	void GetRunning();

	// Function to get ground offset to adjust for character height in traces
	float GetGroundOffset();

	// Function to validate side hit based on normals
	void ValidateSideHits();

	// Function to determine trace distance based on character speed
	float GetTraceDistance() const;

	// Helper function to initialize TraceParams with ignored actors
	void InitializeTraceParams(FCollisionQueryParams& TraceParams) const;

	// Function to pick vault type based on trace results
	ECarbonParkourType ClassifyParkourType();

	// Function to adjust cached ledge hit location
	void AdjustCachedLedgeLocations(float AdjustmentYValue, float AdjustmentZValue);

	// Function to cached Tic-Tac results
	void AdjustCachedTicTacLocations(float AdjustmentValue);

	// Function to adjust cached wall run hit location
	void AdjustCachedWallRunLocation(float AdjustmentValue);

	// Cache owner character for quick access
	TWeakObjectPtr<ACharacter> OwnerChar = nullptr;

	// Capsule shape
	float TraceCapsuleHalfHeight = 200.f;
	float TraceCapsuleRadius = 1.f;

	// Bool for speed
	bool isRunning;

	// Bool for climbing ledge
	bool bClimbLedge = false;

	// Bool to check if tic-tac is within distance
	bool bTicTacHitDistance = false;
	
	// Bool for setting tic-tac offset
	bool bTicTacOffset;
	
	// Bool for clean wall run
	bool bWallRunValid;

	// Cached variables for building parkour solution
	// Cached trace results
	FHitResult CachedForwardHit;
	FHitResult CachedBackwardHit;
	FHitResult CachedStartHeightHit;
	FHitResult CachedLedgeHeightHit;
	FHitResult CachedMidHeightHit; 
	FHitResult CachedLandHeightHit;

	// Tic-tac side trace hits
	FHitResult CachedTicTacLeftHit;
	FHitResult CachedTicTacRightHit;
	FHitResult CachedTicTacHit;

	// Wall run validation hits
	FHitResult CachedWallRunLeftHit;
	FHitResult CachedWallRunRightHit;
	FHitResult CachedWallRunHit;
	
	// Cached adjusted locations for side traces
	UE::Math::TRotator<double> CachedLeftTicTacRotation;
	UE::Math::TRotator<double> CachedRightTicTacRotation;

	UE::Math::TRotator<double> CachedLeftRunRotation;
	UE::Math::TRotator<double> CachedRightRunRotation;

	FVector CachedLeftTicTacHitLocation;
	FVector CachedRightTicTacHitLocation;
	FVector AdjustedCachedTicTacHitLocation;

	FVector CachedLeftWallRunHitLocation;
	FVector CachedRightWallRunHitLocation;
	FVector AdjustedCachedRunHitLocation;

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
