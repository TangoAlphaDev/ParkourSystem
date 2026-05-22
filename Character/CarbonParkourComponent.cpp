// Copyright Tango Alpha LLC. All Rights Reserved.

#include "CarbonParkourComponent.h"

#include "EngineUtils.h"
#include "LyraCharacter.h"
#include "GameFramework/Character.h"
#include "AbilitySystem/Abilities/CarbonParkourTypes.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"


// Run traces for parkour detection
void UCarbonParkourComponent::VaultSolution()
{
	// Clear cached trace results
	CachedForwardHit = FHitResult();
	CachedBackwardHit = FHitResult();
	CachedStartHeightHit = FHitResult();
	CachedLedgeHeightHit = FHitResult();
	CachedMidHeightHit = FHitResult();
	CachedLandHeightHit = FHitResult();
	CachedLeftSideHit = FHitResult();
	CachedRightSideHit = FHitResult();
	CachedLeftSideRotation = FRotator::ZeroRotator;
	CachedRightSideRotation = FRotator::ZeroRotator;
	CachedTicTacHitLocation = FVector::ZeroVector;
	CachedGroundOffset = 0.f;
	CachedObjectHeights.Reset();
	CachedParkourSolution.Reset();
	bTicTacHitDistance = false;
	bTicTacOffset = false;
	bClimbLedge = false;

	OwnerChar = Cast<ACharacter>(GetOwner());

	// Run traces
	SideTrace(true); // Left side
	SideTrace(false); // Right side
	DepthTrace();
	RunHeightTraces();

	ValidateSideHits();

	// Build parkour solution
	BuildParkourSolution();
}


// Side trace to check for space on sides of obstacle
bool UCarbonParkourComponent::SideTrace(bool bLeftSide)
{
	//ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (!OwnerChar.IsValid())
	{
		return false;
	}

	// Calculate offsets
	const FVector ForwardVector = OwnerChar->GetActorForwardVector();
	const FVector RightVector = OwnerChar->GetActorRightVector();
	const FVector TraceDirection = bLeftSide ? -RightVector : RightVector;

	// Calculate trace start and end locations
	const FVector StartLocation = OwnerChar->GetActorLocation();
	const FVector EndLocation = StartLocation + (ForwardVector * 150.f) + FVector(0.f, 0.f, 45.f) + (TraceDirection * SideTraceDistance);

	FCollisionQueryParams TraceParams(FName(TEXT("ParkourSideTrace")), true, (OwnerChar.Get()));
	TraceParams.bReturnPhysicalMaterial = false;

	// Perform the line trace
	FHitResult HitResult;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		StartLocation,
		EndLocation,
		ECC_Visibility,
		TraceParams
	);

	// Cache the raw hit and the tic-tac warp location
	if (bLeftSide)
	{
		CachedLeftSideHit = bHit ? HitResult : FHitResult();
		if (bHit)
		{
			CachedTicTacHitLocation = HitResult.ImpactPoint; // Cache the tic-tac hit location
		}
		// Rotate the normal 90 degrees clockwise for the left side
		FVector AdjustedNormal = FVector::CrossProduct(HitResult.ImpactNormal, FVector::UpVector);
		CachedLeftSideRotation = FRotationMatrix::MakeFromXZ(AdjustedNormal, FVector::UpVector).Rotator();
	}
	else
	{
		CachedRightSideHit = bHit ? HitResult : FHitResult();
		if (bHit)
		{
			CachedTicTacHitLocation = HitResult.ImpactPoint; // Cache the tic-tac hit location
		}
		// Rotate the normal 90 degrees counterclockwise for the right side
		FVector AdjustedNormal = FVector::CrossProduct(FVector::UpVector, HitResult.ImpactNormal);
		CachedRightSideRotation = FRotationMatrix::MakeFromXZ(AdjustedNormal, FVector::UpVector).Rotator();
	}

	// Debug visualization
	if (bDebugTraces)
	{
		const FColor DebugColor = bHit ? FColor::Green : FColor::Red;
		DrawDebugLine(GetWorld(), StartLocation, EndLocation, DebugColor, false, 2.0f);

		if (bHit)
		{
			DrawDebugPoint(GetWorld(), HitResult.ImpactPoint, 10.f, FColor::Yellow, false, 2.0f);
		}
	}

	return bHit;
}


// Forward trace and reverse trace for object depth
void UCarbonParkourComponent::DepthTrace()
{
	// Get owner character
    //ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
    if (!OwnerChar.IsValid())
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
	
	const float TraceDistance = GetTraceDistance();
    FVector EndLocation = StartLocation + (ForwardVector * TraceDistance);
	
    FCollisionQueryParams TraceParams(FName(TEXT("ParkourDepthTrace")), true, OwnerChar.Get());
    TraceParams.bReturnPhysicalMaterial = false;

	// Ignore all LyraCharacter instances
	for (TActorIterator<ALyraCharacter> It(GetWorld()); It; ++It)
	{
		TraceParams.AddIgnoredActor(GetOwner());
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
	if (bDebugTraces)
	{
		DrawDebugCapsule(GetWorld(), StartLocation, TraceCapsuleHalfHeight, TraceCapsuleRadius, FQuat::Identity, FColor::Blue, false, 2.0f);
	}

    if (bInitialHit)
    {
        // Draw the forward trace capsule at the hit location
    	if (bDebugTraces)
    	{
    		DrawDebugCapsule(GetWorld(), InitialHitResult.Location, TraceCapsuleHalfHeight, TraceCapsuleRadius, FQuat::Identity, FColor::Green, false, 2.0f);
    	}
    	CachedForwardHit = InitialHitResult; // Cache the forward hit

    	// Calculate the hit distance
    	float HitDistance = FVector::Dist(StartLocation, InitialHitResult.Location);

    	// Set the boolean if the hit distance is more than DefaultTraceDistance
    	bTicTacHitDistance = HitDistance > DefaultTraceDistance;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("No object detected in initial trace."));
    	// Draw the capsule at the end of the trace
    	if (bDebugTraces)
    	{
    		DrawDebugCapsule(GetWorld(),  StartLocation + (ForwardVector * TraceDistance), TraceCapsuleHalfHeight, TraceCapsuleRadius, FQuat::Identity,FColor::Red, false, 2.0f);
    	}
        return;
    }

    // Calculate the reverse capsule trace from TraceDistance ahead of the hit back to the character
    FVector ReverseStartLocation = InitialHitResult.Location + (ForwardVector * DepthTraceDistance);
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
	if (bDebugTraces)
	{
		DrawDebugCapsule(GetWorld(), ReverseStartLocation, TraceCapsuleHalfHeight, TraceCapsuleRadius, FQuat::Identity, FColor::Blue, false, 2.0f);
	}

    if (bReverseHit)
    {
        // Draw the reverse trace capsule at the hit location
    	if (bDebugTraces)
    	{
    		DrawDebugCapsule(GetWorld(), ReverseHitResult.Location, TraceCapsuleHalfHeight, TraceCapsuleRadius, FQuat::Identity, FColor::Green, false, 2.0f);
    	}
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
    //ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
    if (!OwnerChar.IsValid() || !CachedForwardHit.bBlockingHit)
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
    HeightTrace(StartLocation, CachedStartHeightHit);

    // Trace at the very front of the obstacle for height
	FVector FrontLocation = CachedForwardHit.ImpactPoint;
	FrontLocation.Z = CapsuleBottomZ + DownTraceStartHeight; // Get the height at the front of the trace
	HeightTrace(FrontLocation, CachedLedgeHeightHit);

	// Trace at the back of the obstacle
	FVector MidpointLocation = CachedBackwardHit.ImpactPoint;
	MidpointLocation.Z = CapsuleBottomZ + DownTraceStartHeight;
	HeightTrace(MidpointLocation, CachedMidHeightHit); // Get the height at the back of the obstacle
	
    // Calculate a point slightly beyond the back hit location to check for landing surface
    FVector LandLocation = CachedBackwardHit.Location + (ForwardVector * LandTraceDistance);
    LandLocation.Z = CapsuleBottomZ + DownTraceStartHeight; // Get the same Z height as the initial trace
    HeightTrace(LandLocation, CachedLandHeightHit);
}


// The height trace subroutine
void UCarbonParkourComponent::HeightTrace(const FVector& StartLocation, FHitResult& CachedHitResult)
{
	//ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (!OwnerChar.IsValid())
	{
		return;
	}
	
	// Calculate end location by tracing down from the start location
	FVector EndLocation = StartLocation - FVector(0.f, 0.f, DownTraceDepth);

	// Get the ground offset to adjust the trace for character height
	GetGroundOffset();

	FCollisionQueryParams TraceParams(FName(TEXT("ParkourHeightTrace")), true, OwnerChar.Get());
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

		if (bDebugTraces)
		{
			DrawDebugLine(GetWorld(), StartLocation, HitResult.Location, FColor::Green, false, 2.0f);
			DrawDebugPoint(GetWorld(), HitResult.Location, 10.f, FColor::Blue, false, 2.0f);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("HeightTrace: No hit detected."));
		if (bDebugTraces)
		{
			DrawDebugLine(GetWorld(), StartLocation, EndLocation, FColor::Red, false, 2.0f);
		}
	}
}


// Get the capsule offset from the ground
float UCarbonParkourComponent::GetGroundOffset()
{
    //ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
    if (!OwnerChar.IsValid())
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

    FCollisionQueryParams TraceParams(FName(TEXT("GroundOffsetTrace")), true, OwnerChar.Get());
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


// Validate the side hit results by comparing the normals
void UCarbonParkourComponent::ValidateSideHits()
{
	if (!CachedForwardHit.IsValidBlockingHit())
	{
		CachedLeftSideHit = FHitResult();
		CachedRightSideHit = FHitResult();
		return;
	}

	const FVector ForwardNormal = CachedForwardHit.ImpactNormal;
	const float NormalThreshold = 0.8f; // ~36 degrees

	if (CachedLeftSideHit.bBlockingHit)
	{
		float Dot = FVector::DotProduct(ForwardNormal, CachedLeftSideHit.ImpactNormal); 
		if (Dot > NormalThreshold || CachedForwardHit.GetActor() == CachedLeftSideHit.GetActor())
		{
			CachedLeftSideHit = FHitResult();
		}
	}

	if (CachedRightSideHit.bBlockingHit)
	{
		float Dot = FVector::DotProduct(ForwardNormal, CachedRightSideHit.ImpactNormal);
		if (Dot > NormalThreshold || CachedForwardHit.GetActor() == CachedRightSideHit.GetActor())
		{
			CachedRightSideHit = FHitResult();
		}
	}
}


// Determine trace distance based on character speed and tic tac eligibility
float UCarbonParkourComponent::GetTraceDistance() const
{
	//const ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (!OwnerChar.IsValid())
	{
		return DefaultTraceDistance;
	}

	const UCharacterMovementComponent* MoveComp = OwnerChar->GetCharacterMovement();
	if (!MoveComp)
	{
		return DefaultTraceDistance;
	}

	const float Speed2D = MoveComp->Velocity.Size2D();

	// Check for valid side hits and velocity threshold
	if ((CachedLeftSideHit.IsValidBlockingHit() || CachedRightSideHit.IsValidBlockingHit()) && Speed2D >= SpeedThreshold)
	{
		return RunningTraceDistance;
	}

	return DefaultTraceDistance;
}


// Pick vault type based on trace results
ECarbonParkourType UCarbonParkourComponent::ClassifyParkourType()
{
	CachedParkourSolution.bShouldFall = false; // Default to not falling
    if (!CachedForwardHit.IsValidBlockingHit())
    {
        return ECarbonParkourType::None;
    }

    float ObjectHeight = CachedObjectHeights[1];
    if (ObjectHeight <= MaxStepHeight)
    {
        if (CachedObjectDepth <= ObjectDepthMedium) return ECarbonParkourType::Step;
    	return ECarbonParkourType::StepOn;
    }
    if (ObjectHeight <= MaxVaultHeight)
    {
		// Low height tic-tac ok, the animation seems to work. if not, see the bTicTacHitDistance logic below
    	bTicTacOffset = true;
    	if (CachedLeftSideHit.bBlockingHit) return ECarbonParkourType::VaultTicTacLeft;
    	if (CachedRightSideHit.bBlockingHit) return ECarbonParkourType::VaultTicTacRight;

    	bTicTacOffset = false;
    	if (CachedObjectDepth <= ObjectDepthXShort) return ECarbonParkourType::VaultXShort;
        if (CachedObjectDepth <= ObjectDepthShort) return ECarbonParkourType::VaultShort;
        if (CachedObjectDepth <= ObjectDepthMedium) return ECarbonParkourType::VaultMedium;
        if (CachedObjectDepth <= ObjectDepthLong) return ECarbonParkourType::VaultLong;
    	return ECarbonParkourType::VaultOn;
       
    }
    if (ObjectHeight <= MaxHighVaultHeight)
    {
    	CachedParkourSolution.bShouldFall = true; // Falling for high vaults
    	if (bTicTacHitDistance) // Check if tic-tac is allowed
    	{
    		if (CachedLeftSideHit.bBlockingHit)
    		{
    			bTicTacOffset = true;
    			if (CachedObjectDepth <= ObjectDepthShort) return ECarbonParkourType::TicTacLeft;
    			CachedParkourSolution.bShouldFall = false;
				return ECarbonParkourType::TicTacLeftOn;
    		}
    		if (CachedRightSideHit.bBlockingHit)
    		{
    			bTicTacOffset = true;
    			if (CachedObjectDepth <= ObjectDepthShort) return ECarbonParkourType::TicTacRight;
    			CachedParkourSolution.bShouldFall = false;
    			return ECarbonParkourType::TicTacRightOn;
    		}
    	}
        else
        {
			if (CachedObjectDepth <= ObjectDepthXShort) return ECarbonParkourType::VaultHighXShort;
        	if (CachedObjectDepth <= ObjectDepthShort) return ECarbonParkourType::VaultHighShort;
        	
       		CachedParkourSolution.bShouldFall = false;
       		return ECarbonParkourType::VaultHighOn;
        }
    }

	bClimbLedge = true;
    if (ObjectHeight <= MaxClimbObjectHeight)
    {
    	if (CachedObjectDepth <= ObjectDepthShort) return ECarbonParkourType::WallRunOver;
	    return ECarbonParkourType::WallRunOn;
    }

	bClimbLedge = false;
	bTicTacOffset = false;
    return ECarbonParkourType::None;
}


// Function to add adjustment to cached hit locations
void UCarbonParkourComponent::AdjustCachedTicTacLocations(float AdjustmentValue)
{
	if (!bTicTacOffset)
	{
		return; // No adjustment needed
	}

	const FHitResult SideHit = CachedLeftSideHit.bBlockingHit ? CachedLeftSideHit : CachedRightSideHit;

	if (!SideHit.bBlockingHit)
	{
		return;
	}

	const FVector WallNormal = SideHit.ImpactNormal.GetSafeNormal();

	// This sets the target to exactly DesiredWallDistance away from the wall.
	CachedTicTacHitLocation = SideHit.ImpactPoint + (WallNormal * AdjustmentValue);
	CachedMidHeightHit.ImpactPoint += WallNormal * AdjustmentValue;
	CachedLandHeightHit.ImpactPoint += WallNormal * AdjustmentValue;
}


// Function to adjust the cached ledge hit location
void UCarbonParkourComponent::AdjustCachedLedgeLocations(float AdjustmentYValue, float AdjustmentZValue)
{
	if (!CachedLedgeHeightHit.bBlockingHit)
	{
		return; // No adjustment needed
	}

	// Sets cached location away from the wall
	CachedLedgeHeightHit.ImpactPoint -= OwnerChar->GetActorForwardVector() * AdjustmentYValue;
	CachedLedgeHeightHit.ImpactPoint.Z -= AdjustmentZValue;
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

	// Set solution data
	Solution.bIsValid = true;
	Solution.ParkourType = ParkourType;

	// Validate owner character
	//ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (!OwnerChar.IsValid())
	{
		return; // Ensure the owner is valid before proceeding
	}

	// Apply tic-tac offset adjustment to the cached hit locations if needed
	if (bTicTacOffset)
	{
		//FVector Direction = CachedLeftSideHit.bBlockingHit ? -OwnerChar->GetActorRightVector() : OwnerChar->GetActorRightVector();
		AdjustCachedTicTacLocations(SideOffsetAdjustment);
	}

	if (bClimbLedge)
	{
		AdjustCachedLedgeLocations(LedgeOffsetYAdjustment, LedgeOffsetZAdjustment);
	}
	
	// Apply motion warping logic
	Solution.WarpTargetStart = TEXT("ParkourTargetStart");
	Solution.WarpTargetLedge = TEXT("ParkourTargetLedge");
	Solution.WarpTargetMiddle = TEXT("ParkourTargetMid");
	Solution.WarpTargetLand = TEXT("ParkourTargetLand");
	Solution.WarpTargetTicTac = TEXT("ParkourTargetTicTac");

	// Surface data
	Solution.SurfaceNormal = CachedForwardHit.ImpactNormal;

	// Obstacle metrics
	Solution.ObstacleDistance = FVector::Dist(FVector(CachedForwardHit.ImpactPoint.X, CachedForwardHit.ImpactPoint.Y, 0.0f), FVector(CachedStartHeightHit.ImpactPoint.X, CachedStartHeightHit.ImpactPoint.Y, 0.0f));
	Solution.ObstacleHeight = CachedStartHeightHit.ImpactPoint.Z - CachedMidHeightHit.ImpactPoint.Z;
	Solution.ObstacleLength = FVector::Dist(CachedForwardHit.ImpactPoint, CachedBackwardHit.ImpactPoint);

	// Compute warp locations
	FVector Start = CachedStartHeightHit.ImpactPoint;
	FVector Ledge = CachedLedgeHeightHit.ImpactPoint;
	FVector Mid = CachedMidHeightHit.ImpactPoint;
	FVector Land = CachedLandHeightHit.ImpactPoint;
	FVector TicTac = CachedTicTacHitLocation;

	// Cache target locations in solution struct for use in ability
	Solution.TargetTransformStart = Start;
	Solution.TargetTransformLedge = Ledge;
	Solution.TargetTransformMid = Mid;
	Solution.TargetTransformLand = Land;
	Solution.TargetTransformTicTac = TicTac;

	// Build transforms
	const FRotator WarpRotation = FRotationMatrix::MakeFromXZ(- CachedForwardHit.ImpactNormal, FVector::UpVector).Rotator();

	// Note: The rotation is based on the negative impact normal to orient the character facing towards the obstacle, and using the world up vector to keep the character upright. Adjust as needed for specific animation requirements.
	Solution.WarpTransformStart = FTransform(WarpRotation, Start);
	Solution.WarpTransformLedge = FTransform(WarpRotation, Ledge);
	Solution.WarpTransformMid = FTransform(WarpRotation, Mid);
	Solution.WarpTransformLand = FTransform(WarpRotation, Land);
	Solution.WarpTransformTicTac = FTransform(WarpRotation, TicTac);

	// Add side trace results
	Solution.bLeftSideHit = CachedLeftSideHit.bBlockingHit;
	Solution.bRightSideHit = CachedRightSideHit.bBlockingHit;
	Solution.LeftSideRotation = CachedLeftSideRotation;
	Solution.RightSideRotation = CachedRightSideRotation;
	
	// Cache the solution or broadcast solution (which is best?)
	CachedParkourSolution = Solution;
	// OnParkourSolutionReady.Broadcast(Solution);

	if (bDebugSpheres)
	{
		// draw spheres at the warp target locations for debugging
		DrawDebugSphere(GetWorld(), Start, 10.f, 12, FColor::Red, false, 5.0f);
		DrawDebugSphere(GetWorld(), Ledge, 10.f, 12, FColor::Green, false, 5.0f);
		DrawDebugSphere(GetWorld(), Mid, 10.f, 12, FColor::Cyan, false, 5.0f);
		DrawDebugSphere(GetWorld(), Land, 10.f, 12, FColor::Blue, false, 5.0f);

		// draw spheres at the side trace hit locations for debugging
		if (CachedLeftSideHit.bBlockingHit)
		{
			DrawDebugSphere(GetWorld(), TicTac, 10.f, 12, FColor::Yellow, false, 5.0f);
		}
		if (CachedRightSideHit.bBlockingHit)
		{
			DrawDebugSphere(GetWorld(), TicTac, 10.f, 12, FColor::Yellow, false, 5.0f);
		}
	}
}


// Getter for the cached parkour solution
const FCarbonParkourSolution& UCarbonParkourComponent::GetCachedParkourSolution() const
{
	return CachedParkourSolution;
}
