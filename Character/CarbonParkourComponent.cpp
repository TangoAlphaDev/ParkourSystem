// Copyright Tango Alpha LLC. All Rights Reserved.

#include "CarbonParkourComponent.h"

#include "EngineUtils.h"
#include "LyraCharacter.h"
#include "GameFramework/Character.h"
#include "AbilitySystem/Abilities/CarbonParkourTypes.h"
#include "Components/CapsuleComponent.h"


// Run traces for parkour detection
void UCarbonParkourComponent::VaultSolution()
{
	// Clear cached trace results
	CachedForwardHit = FHitResult();
	CachedBackwardHit = FHitResult();
	CachedInitHeightHit = FHitResult();
	CachedEndHeightHit = FHitResult();
	CachedLandHeightHit = FHitResult();
	CachedLeftSideHit = false;
	CachedRightSideHit = false;
	CachedGroundOffset = 0.f;
	CachedObjectHeights.Reset();
	CachedParkourSolution.Reset();

	// Run traces
	DepthTrace();
	RunHeightTraces();
	SideTrace(true); // Left side
	SideTrace(false); // Right side

	// Select the animation type
	ECarbonParkourType ParkourType = ClassifyParkourType();

	// Log the parkour type
	UE_LOG(LogTemp, Warning, TEXT("Parkour Type: %s"), *UEnum::GetValueAsString(ParkourType));

	// Build parkour solution
	BuildParkourSolution();
}

void UCarbonParkourComponent::DepthTrace()
{
	// Get owner character
    ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
    if (!OwnerChar)
    {
        return;
    }

    // Get the capsule component
    UCapsuleComponent* Capsule = OwnerChar->GetCapsuleComponent();
    if (!Capsule) return;

	// Get capsule half height
	float CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	
	// Calculate trace start and end locations and direction
    FVector StartLocation = OwnerChar->GetActorLocation();
	StartLocation.Z += (TraceCapsuleHalfHeight - CapsuleHalfHeight); // Offset the capsule center to keep the bottom aligned with the larger capsule trace
    FVector ForwardVector = OwnerChar->GetActorForwardVector();
    FVector EndLocation = StartLocation + (ForwardVector * TraceDistance);
	
    FCollisionQueryParams TraceParams(FName(TEXT("ParkourDepthTrace")), true, OwnerChar);
    TraceParams.bReturnPhysicalMaterial = false;

	// Ignore all LyraCharacter instances
	for (TActorIterator<ALyraCharacter> It(GetWorld()); It; ++It)
	{
		TraceParams.AddIgnoredActor(*It);
	}
	
    // First capsule trace to detect the object
    FHitResult InitialHitResult;
    bool bInitialHit = GetWorld()->SweepSingleByChannel(
        InitialHitResult,
        StartLocation,
        EndLocation,
        FQuat::Identity,
        ECC_Visibility,
        FCollisionShape::MakeCapsule(TraceCapsuleRadius, TraceCapsuleHalfHeight),
        TraceParams
    );

    // Draw the forward trace capsule at the start location
    DrawDebugCapsule(GetWorld(), StartLocation, TraceCapsuleHalfHeight, TraceCapsuleRadius, FQuat::Identity, FColor::Blue, false, 2.0f);

    if (bInitialHit)
    {
        // Draw the forward trace capsule at the hit location
        DrawDebugCapsule(GetWorld(), InitialHitResult.Location, TraceCapsuleHalfHeight, TraceCapsuleRadius, FQuat::Identity, FColor::Green, false, 2.0f);
    	CachedForwardHit = InitialHitResult; // Cache the forward hit
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("No object detected in initial trace."));
        return;
    }

    // Calculate the reverse capsule trace from TraceDistance ahead of the hit back to the character
    FVector ReverseStartLocation = InitialHitResult.Location + (ForwardVector * TraceDistance);
    FVector ReverseEndLocation = InitialHitResult.Location;

	// Perform the reverse trace
    FHitResult ReverseHitResult;
    bool bReverseHit = GetWorld()->SweepSingleByChannel(
        ReverseHitResult,
        ReverseStartLocation,
        ReverseEndLocation,
        FQuat::Identity,
        ECC_Visibility,
        FCollisionShape::MakeCapsule(TraceCapsuleRadius, TraceCapsuleHalfHeight),
        TraceParams
    );

    // Draw the reverse trace capsule at the start location
    DrawDebugCapsule(GetWorld(), ReverseStartLocation, TraceCapsuleHalfHeight, TraceCapsuleRadius, FQuat::Identity, FColor::Blue, false, 2.0f);

    if (bReverseHit)
    {
        // Draw the reverse trace capsule at the hit location
        DrawDebugCapsule(GetWorld(), ReverseHitResult.Location, TraceCapsuleHalfHeight, TraceCapsuleRadius, FQuat::Identity, FColor::Green, false, 2.0f);
    	CachedBackwardHit = ReverseHitResult; // Cache the backside hit

        // Calculate depth
        float Depth = FVector::Dist(InitialHitResult.Location, ReverseHitResult.Location);

        if (Depth == 0.0f)
        {
            UE_LOG(LogTemp, Warning, TEXT("Vault on"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Object depth: %f"), Depth);
        	CachedObjectDepth = Depth; // Cache the object depth
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("No object detected in reverse trace. Object may be too tall or thin."));
    	CachedForwardHit = FHitResult();
    }
}


// Run multiple height traces to find obstacle height and height of ground beyond obstacle too
void UCarbonParkourComponent::RunHeightTraces()
{
    ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
    if (!OwnerChar || !CachedForwardHit.bBlockingHit)
    {
        return;
    }

	// Get the capsule component
	UCapsuleComponent* Capsule = OwnerChar->GetCapsuleComponent();
	if (!Capsule) return;

	// Get the characters location and forward vector
    FVector ForwardVector = OwnerChar->GetActorForwardVector();
	FVector CharacterLocation = OwnerChar->GetActorLocation();

    // Calculate the bottom of the capsule at the character's location
	float CapsuleBottomZ = OwnerChar->GetActorLocation().Z - Capsule->GetScaledCapsuleHalfHeight();

    // Adjust the start location for the first height trace
	FVector StartLocation = CharacterLocation;
    StartLocation.Z = CapsuleBottomZ + DownTraceStartHeight;

    // Initial trace at character location
    HeightTrace(StartLocation, CachedInitHeightHit, FColor::Red);

    // Trace in the middle of the object for height, calculate midpoint between CachedForwardHit and CachedBackwardHit
    FVector MidpointLocation = (CachedForwardHit.Location + CachedBackwardHit.Location) * 0.5f;
    MidpointLocation.Z = CapsuleBottomZ + DownTraceStartHeight; // Get the same Z height as the initial trace
    HeightTrace(MidpointLocation, CachedEndHeightHit, FColor::Green);

    // Calculate a point slightly beyond the forward hit location to check for landing surface
    FVector LandLocation = CachedBackwardHit.Location + (ForwardVector * 75.f);
    LandLocation.Z = CapsuleBottomZ + DownTraceStartHeight; // Get the same Z height as the initial trace
    HeightTrace(LandLocation, CachedLandHeightHit, FColor::Blue);
}

// The height trace subroutine
void UCarbonParkourComponent::HeightTrace(const FVector& StartLocation, FHitResult& CachedHitResult, FColor DebugColor)
{
	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (!OwnerChar)
	{
		return;
	}
	
	// Calculate end location by tracing down from the start location
	FVector EndLocation = StartLocation - FVector(0.f, 0.f, DownTraceDepth);

	// Get the ground offset to adjust the trace for character height
	GetGroundOffset();

	FCollisionQueryParams TraceParams(FName(TEXT("ParkourHeightTrace")), true, OwnerChar);
	TraceParams.bReturnPhysicalMaterial = false;
	
	// Ignore all LyraCharacter instances
	for (TActorIterator<ALyraCharacter> It(GetWorld()); It; ++It)
	{
		TraceParams.AddIgnoredActor(*It);
	}

	FHitResult HitResult;
	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		StartLocation,
		EndLocation,
		ECC_Visibility,
		TraceParams
	);

	if (bHit)
	{
		CachedHitResult = HitResult;

		// Calculate and log the height of the object
		float ObjectHeight = (DownTraceStartHeight + CachedGroundOffset) - (StartLocation.Z - HitResult.Location.Z);
		UE_LOG(LogTemp, Warning, TEXT("Object Height: %f"), ObjectHeight);

		// Add the calculated height to the array
		CachedObjectHeights.Add(ObjectHeight);
		
		DrawDebugLine(GetWorld(), StartLocation, HitResult.Location, DebugColor, false, 2.0f);
		DrawDebugPoint(GetWorld(), HitResult.Location, 10.f, FColor::Blue, false, 2.0f);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("HeightTrace: No hit detected."));
		DrawDebugLine(GetWorld(), StartLocation, EndLocation, FColor::Red, false, 2.0f);
	}
}


// Side trace to check for space on sides of obstacle
bool UCarbonParkourComponent::SideTrace(bool bLeftSide)
{
	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (!OwnerChar)
	{
		return false;
	}

	// Calculate trace start and end locations
	FVector StartLocation = OwnerChar->GetActorLocation();
	FVector RightVector = OwnerChar->GetActorRightVector();
	FVector TraceDirection = bLeftSide ? -RightVector : RightVector;
	FVector EndLocation = StartLocation + (TraceDirection * SideTraceDistance);

	FCollisionQueryParams TraceParams(FName(TEXT("ParkourSideTrace")), true, OwnerChar);
	TraceParams.bReturnPhysicalMaterial = false;

	// Perform the line trace
	FHitResult HitResult;
	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		StartLocation,
		EndLocation,
		ECC_Visibility,
		TraceParams
	);

	// Cache the result based on the side
	if (bLeftSide)
	{
		CachedLeftSideHit = bHit;
	}
	else
	{
		CachedRightSideHit = bHit;
	}
	
	// Debug visualization
	FColor DebugColor = bHit ? FColor::Green : FColor::Red;
	DrawDebugLine(GetWorld(), StartLocation, EndLocation, DebugColor, false, 2.0f);
	
	// Return the result of the trace
	return bHit;
}


// Get the capsule offset from the ground
float UCarbonParkourComponent::GetGroundOffset()
{
    ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
    if (!OwnerChar)
    {
        return 0.f;
    }

    // Get the capsule component
    UCapsuleComponent* Capsule = OwnerChar->GetCapsuleComponent();
    if (!Capsule)
    {
        return 0.f;
    }

    // Calculate the bottom of the capsule
    FVector StartLocation = OwnerChar->GetActorLocation();
    StartLocation.Z -= Capsule->GetScaledCapsuleHalfHeight();

    // End location directly below the capsule
    FVector EndLocation = StartLocation - FVector(0.f, 0.f, DownTraceStartHeight);

    FCollisionQueryParams TraceParams(FName(TEXT("GroundOffsetTrace")), true, OwnerChar);
    TraceParams.bReturnPhysicalMaterial = false;

    FHitResult HitResult;
    bool bHit = GetWorld()->LineTraceSingleByChannel(
        HitResult,
        StartLocation,
        EndLocation,
        ECC_Visibility,
        TraceParams
    );

    if (bHit)
    {
        CachedGroundOffset = StartLocation.Z - HitResult.Location.Z;
        UE_LOG(LogTemp, Warning, TEXT("Offset ground height: %f"), CachedGroundOffset);
        return CachedGroundOffset;
    }

    return 0.f; // Default to 0 if no ground detected
}


// Pick vault type based on trace results
ECarbonParkourType UCarbonParkourComponent::ClassifyParkourType()
{
	if (!CachedForwardHit.IsValidBlockingHit())
	{
		return ECarbonParkourType::None;
	}
	if (CachedObjectHeights[1] < MaxStepHeight)
	{
		if (CachedObjectDepth < ObjectDepthMedium)
		{
			return ECarbonParkourType::Step;
		}
		else
		{
			return ECarbonParkourType::StepOn;
		}
	}
	if (CachedObjectHeights[1] < MaxVaultHeight)
	{
		if (CachedLeftSideHit)
		{
			return ECarbonParkourType::TicTacVaultLeft;
		}
		if (CachedRightSideHit)
		{
			return ECarbonParkourType::TicTacVaultRight;
		}
		else
		{
			if (CachedObjectDepth < ObjectDepthShort)
			{
				return ECarbonParkourType::VaultShort;
			}
			if (CachedObjectDepth < ObjectDepthMedium)
			{
				return ECarbonParkourType::VaultMedium;
			}
			if (CachedObjectDepth < ObjectDepthLong)
			{
				return ECarbonParkourType::VaultLong;
			}
			else
			{
				return ECarbonParkourType::VaultOn;
			}	
		}
	}
	if (CachedObjectHeights[1] < MaxHighVaultHeight)
	{
		if (CachedLeftSideHit)
		{
			return ECarbonParkourType::TicTacHighLeft;
		}
		if (CachedRightSideHit)
		{
			return ECarbonParkourType::TicTacHighRight;
		}
		else
		{
			if (CachedObjectDepth < ObjectDepthShort)
			{
				return ECarbonParkourType::VaultHigh;
			}
			else
			{
				return ECarbonParkourType::VaultHighOn;
			}
		}
	}
	if (CachedObjectHeights[1] < MaxClimbObjectHeight)
	{
		return ECarbonParkourType::ClimbUpLow;
	}
	if (CachedObjectHeights[1] < MaxDoubleClimbObjectHeight)
	{
		return ECarbonParkourType::ClimbUpHigh;
	}
	else
	{
		return ECarbonParkourType::None;
	}
}


// Build the parkour solution data
void UCarbonParkourComponent::BuildParkourSolution()
{
	// Initialize solution struct
	FCarbonParkourSolution Solution;
	Solution.Reset();

	// Validate forward hit
	if (!CachedForwardHit.IsValidBlockingHit())
	{
		return;
	}

	// Get selected parkour type
	const ECarbonParkourType ParkourType = ClassifyParkourType();
	if (ParkourType == ECarbonParkourType::None)
	{
		return;
	}
	
	Solution.bIsValid = true;
	Solution.ParkourType = ParkourType;

	// Warp target names
	Solution.WarpTargetStart = TEXT("ParkourTargetStart");
	Solution.WarpTargetMiddle = TEXT("ParkourTargetMid");
	Solution.WarpTargetEnd = TEXT("ParkourTargetEnd");

	// Surface data
	Solution.SurfaceNormal = CachedForwardHit.ImpactNormal;

	// Obstacle metrics
	Solution.ObstacleDistance = FVector::Dist(FVector(CachedForwardHit.ImpactPoint.X, CachedForwardHit.ImpactPoint.Y, 0.0f), FVector(CachedInitHeightHit.ImpactPoint.X, CachedInitHeightHit.ImpactPoint.Y, 0.0f));
	Solution.ObstacleHeight = CachedInitHeightHit.ImpactPoint.Z - CachedEndHeightHit.ImpactPoint.Z;
	Solution.ObstacleLength = FVector::Dist(CachedForwardHit.ImpactPoint, CachedBackwardHit.ImpactPoint);

	// Compute warp locations
	FVector Start = CachedInitHeightHit.ImpactPoint;
	FVector Mid = CachedEndHeightHit.ImpactPoint;
	FVector End = CachedLandHeightHit.ImpactPoint;

	// Cache target locations in solution struct for use in ability
	Solution.TargetTransformStart = Start;
	Solution.TargetTransformMid = Mid;
	Solution.TargetTransformEnd = End;

	// Build transforms
	const FRotator WarpRotation = FRotationMatrix::MakeFromXZ(- CachedForwardHit.ImpactNormal, FVector::UpVector).Rotator();

	// Note: The rotation is based on the negative impact normal to orient the character facing towards the obstacle, and using the world up vector to keep the character upright. Adjust as needed for specific animation requirements.
	Solution.WarpTransformStart = FTransform(WarpRotation, Start);
	Solution.WarpTransformMid = FTransform(WarpRotation, Mid);
	Solution.WarpTransformEnd = FTransform(WarpRotation, End);

	// Cache the solution or broadcast solution (which is best?)
	CachedParkourSolution = Solution;
	// OnParkourSolutionReady.Broadcast(Solution);

	// draw spheres at the warp target locations for debugging
	DrawDebugSphere(GetWorld(), Start, 10.f, 12, FColor::Red, false, 5.0f);
	DrawDebugSphere(GetWorld(), Mid, 10.f, 12, FColor::Green, false, 5.0f);
	DrawDebugSphere(GetWorld(), End, 10.f, 12, FColor::Blue, false, 5.0f);
}


// Getter for the cached parkour solution
const FCarbonParkourSolution& UCarbonParkourComponent::GetCachedParkourSolution() const
{
	return CachedParkourSolution;
}
