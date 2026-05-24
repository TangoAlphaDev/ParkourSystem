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
	// Cast to character and cache owner for quicker access
	OwnerChar = Cast<ACharacter>(GetOwner());
	
	// Clear cached trace results
	ClearCachedTraces();

	GetRunning();

	GetGroundOffset();
	
	// Run traces
	DepthTrace();
	
	RunHeightTraces();
	
	TicTacTrace(true); // Left side
	TicTacTrace(false); // Right side
	
	ValidateSideHits();

	WallRunTrace(true);
	WallRunTrace(false);

	// Build parkour solution
	BuildParkourSolution();
}


// Get character speed and determine if running
void UCarbonParkourComponent::GetRunning()
{
	if (!OwnerChar.IsValid())
	{
		return;
	}

	const UCharacterMovementComponent* MoveComp = OwnerChar->GetCharacterMovement();
	if (!MoveComp)
	{
		return;
	}

	const float Speed2D = MoveComp->Velocity.Size2D();
	isRunning = Speed2D >= SpeedThreshold;
}


// Get the capsule offset from the ground
float UCarbonParkourComponent::GetGroundOffset()
{
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


// Determine trace distance based on character speed and tic tac eligibility
float UCarbonParkourComponent::GetTraceDistance() const
{
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
	if ((CachedTicTacLeftHit.IsValidBlockingHit() || CachedTicTacRightHit.IsValidBlockingHit()) && Speed2D >= SpeedThreshold)
	{
		return RunningTraceDistance;
	}

	return DefaultTraceDistance;
}


// Helper function to initialize TraceParams with ignored actors
void UCarbonParkourComponent::InitializeTraceParams(FCollisionQueryParams& TraceParams) const
{
	
}


// Forward trace and reverse trace for object depth
void UCarbonParkourComponent::DepthTrace()
{
	// Get owner character
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
	
	// Trace distance
	float TraceDistance = isRunning ? RunningTraceDistance : DefaultTraceDistance;
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


// Side trace to check for space on sides of obstacle
bool UCarbonParkourComponent::TicTacTrace(bool bLeftSide)
{
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
	const FVector EndLocation = StartLocation + (ForwardVector * SideTraceDistance) + FVector(0.f, 0.f, 45.f) + (TraceDirection * SideTraceDistance);

	FCollisionQueryParams TraceParams(FName(TEXT("ParkourSideTrace")), true, (OwnerChar.Get()));
	TraceParams.bReturnPhysicalMaterial = false;

	// Ignore all LyraCharacter instances
	for (TActorIterator<ALyraCharacter> It(GetWorld()); It; ++It)
	{
		TraceParams.AddIgnoredActor(*It);
	}

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
		CachedTicTacLeftHit = bHit ? HitResult : FHitResult();
		if (bHit)
		{
			CachedLeftTicTacHitLocation = HitResult.ImpactPoint; // Cache the tic-tac hit location
		}
		// Rotate the normal 90 degrees clockwise for the left side
		FVector AdjustedNormal = FVector::CrossProduct(HitResult.ImpactNormal, FVector::UpVector);
		CachedLeftTicTacRotation = FRotationMatrix::MakeFromXZ(AdjustedNormal, FVector::UpVector).Rotator();
	}
	else
	{
		CachedTicTacRightHit = bHit ? HitResult : FHitResult();
		if (bHit)
		{
			CachedRightTicTacHitLocation = HitResult.ImpactPoint; // Cache the tic-tac hit location
		}
		// Rotate the normal 90 degrees counterclockwise for the right side
		FVector AdjustedNormal = FVector::CrossProduct(FVector::UpVector, HitResult.ImpactNormal);
		CachedRightTicTacRotation = FRotationMatrix::MakeFromXZ(AdjustedNormal, FVector::UpVector).Rotator();
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


// Validate the side hit results by comparing the normals
void UCarbonParkourComponent::ValidateSideHits()
{
	if (!CachedForwardHit.IsValidBlockingHit())
	{
		CachedTicTacLeftHit = FHitResult();
		CachedTicTacRightHit = FHitResult();
		return;
	}

	const FVector ForwardNormal = CachedForwardHit.ImpactNormal;
	const float NormalThreshold = 0.8f; // ~36 degrees

	if (CachedTicTacLeftHit.bBlockingHit)
	{
		float Dot = FVector::DotProduct(ForwardNormal, CachedTicTacLeftHit.ImpactNormal); 
		if (Dot > NormalThreshold || CachedForwardHit.GetActor() == CachedTicTacLeftHit.GetActor())
		{
			CachedTicTacLeftHit = FHitResult();
		}
	}

	if (CachedTicTacRightHit.bBlockingHit)
	{
		float Dot = FVector::DotProduct(ForwardNormal, CachedTicTacRightHit.ImpactNormal);
		if (Dot > NormalThreshold || CachedForwardHit.GetActor() == CachedTicTacRightHit.GetActor())
		{
			CachedTicTacRightHit = FHitResult();
		}
	}

	// Set cached hit for rotation
	// Right right first to favor left
	if (CachedTicTacRightHit.bBlockingHit)
	{
		CachedTicTacHit = CachedTicTacRightHit;
	}
	if (CachedTicTacLeftHit.bBlockingHit)
	{
		CachedTicTacHit = CachedTicTacLeftHit;
	}
}


// Wall trace to check for wall run
bool UCarbonParkourComponent::WallRunTrace(bool bLeftSide)
{
	if (!OwnerChar.IsValid() || !isRunning) // Check if character is running with get validation
	{
		return false;
	}

	// Calculate offsets
	const FVector ForwardVector = OwnerChar->GetActorForwardVector();
	const FVector RightVector = OwnerChar->GetActorRightVector();
	const FVector TraceDirection = bLeftSide ? -RightVector : RightVector;

	// Calculate trace start and end locations
	const FVector StartLocation = OwnerChar->GetActorLocation();
	const FVector EndLocation = StartLocation + (ForwardVector * WallSideTraceDistance) + FVector(0.f, 0.f, 45.f) + (TraceDirection * SideTraceDistance);

	FCollisionQueryParams TraceParams(FName(TEXT("ParkourWallTrace")), true, (OwnerChar.Get()));
	TraceParams.bReturnPhysicalMaterial = false;

	// Ignore all LyraCharacter instances
	for (TActorIterator<ALyraCharacter> It(GetWorld()); It; ++It)
	{
		TraceParams.AddIgnoredActor(*It);
	}

	// Perform the line trace
	FHitResult HitResult;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		StartLocation,
		EndLocation,
		ECC_Visibility,
		TraceParams
	);

	if (bLeftSide)
	{
		CachedWallRunLeftHit = bHit ? HitResult : FHitResult();
		if (bHit)
		{
			WallRunValidationTrace(HitResult.ImpactPoint, HitResult.ImpactNormal);
			if (bWallRunValid)
			{
				CachedLeftWallRunHitLocation = HitResult.ImpactPoint; // Cache the wall run hit location
			}
			// Rotate the normal 90 degrees clockwise for the left side
			FVector AdjustedNormal = FVector::CrossProduct(FVector::UpVector, CachedWallRunHit.ImpactNormal).GetSafeNormal();
			CachedLeftRunRotation = FRotationMatrix::MakeFromXZ(AdjustedNormal, FVector::UpVector).Rotator();
			// Cache rotation with CachedLeftRunRotation
			CachedLeftRunRotation = CachedLeftRunRotation + FRotator(0.0f, -90.0f, 0.0f);
		}
	}
	else
	{
		CachedWallRunRightHit = bHit ? HitResult : FHitResult();
		if (bHit)
		{
			WallRunValidationTrace(HitResult.ImpactPoint, HitResult.ImpactNormal);
			if (bWallRunValid)
			{
				CachedRightWallRunHitLocation = HitResult.ImpactPoint; // Cache the wall run hit location
			}
			// Rotate the normal 90 degrees counterclockwise for the right side
			FVector AdjustedNormal = FVector::CrossProduct(FVector::UpVector, HitResult.ImpactNormal);
			CachedRightRunRotation = FRotationMatrix::MakeFromXZ(AdjustedNormal, FVector::UpVector).Rotator();
		}
	}

	// Debug visualization
	if (bDebugTraces)
	{
		const FColor DebugColor = bHit ? FColor::Green : FColor::Orange;
		DrawDebugLine(GetWorld(), StartLocation, EndLocation, DebugColor, false, 2.0f);

		if (bHit)
		{
			DrawDebugPoint(GetWorld(), HitResult.ImpactPoint, 10.f, FColor::Yellow, false, 2.0f);
		}
	}

	// Set cached hit for rotation
	// Right right first to favor left
	if (CachedWallRunRightHit.bBlockingHit)
	{
		CachedWallRunHit = CachedWallRunRightHit;
	}
	if (CachedWallRunLeftHit.bBlockingHit)
	{
		CachedWallRunHit = CachedWallRunLeftHit;
	}
	
	return bHit;
}


// Validation trace to check for wall run
void UCarbonParkourComponent::WallRunValidationTrace(const FVector& WallHitLocation, const FVector& WallNormal)
{
	if (!OwnerChar.IsValid())
	{
		return;
	}

	// Get the character's forward vector
	FVector ForwardVector = OwnerChar->GetActorForwardVector();
	
	// Calculate the dot product between the forward vector and the wall normal
	float DotProduct = FVector::DotProduct(ForwardVector, WallNormal);

	// Check if the angle is within the threshold (45 degrees)
	const float Threshold = FMath::Cos(FMath::DegreesToRadians(45.0f)); // Cosine of 45 degrees
	if (FMath::Abs(DotProduct) > Threshold)
	{
		bWallRunValid = false;
		UE_LOG(LogTemp, Warning, TEXT("WallRunValidationTrace: Angle too steep for wall run."));
		return;
	}

	// Get the character's capsule size
	UCapsuleComponent* Capsule = OwnerChar->GetCapsuleComponent();
	if (!Capsule)
	{
		return;
	}

	float CapsuleRadius = Capsule->GetScaledCapsuleRadius();
	float CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();

	// Offset the start location slightly away from the wall
	FVector StartLocation = WallHitLocation + WallNormal * (CapsuleRadius + 1.0f); // Offset by capsule radius + 1 unit
	
	// Calculate the direction along the wall
	FVector WallDirection = FVector::CrossProduct(WallNormal, FVector::UpVector).GetSafeNormal();

	// Adjust the direction based on the side (left or right)
	if (FVector::DotProduct(WallDirection, OwnerChar->GetActorRightVector()) < 0)
	{
		WallDirection *= -1.0f;
	}

	// Set the end location to run along the wall
	FVector EndLocation = StartLocation + WallDirection * WallRunTraceDistance;

	// Capsule trace parameters
	FCollisionQueryParams TraceParams(FName(TEXT("WallRunValidationTrace")), true, OwnerChar.Get());
	TraceParams.bReturnPhysicalMaterial = false;

	// Perform the capsule trace
	FHitResult HitResult;
	bool bHit = GetWorld()->SweepSingleByChannel(
		HitResult,
		StartLocation,
		EndLocation,
		FQuat::Identity,
		ECC_Visibility,
		FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHalfHeight),
		TraceParams
	);

	// Debug visualization
	if (bDebugTraces)
	{
		const FColor DebugColor = bHit ? FColor::Red : FColor::Green;
		DrawDebugCapsule(GetWorld(), StartLocation, CapsuleHalfHeight, CapsuleRadius, FQuat::Identity, DebugColor, false, 2.0f);
		DrawDebugCapsule(GetWorld(), EndLocation, CapsuleHalfHeight, CapsuleRadius, FQuat::Identity, DebugColor, false, 2.0f);
	}

	// Handle the result
	if (bHit)
	{
		bWallRunValid = false;
		UE_LOG(LogTemp, Warning, TEXT("WallRunValidationTrace hit: %s"), *HitResult.GetActor()->GetActorNameOrLabel());
	}
	else
	{
		bWallRunValid = true;
		UE_LOG(LogTemp, Warning, TEXT("WallRunValidationTrace did not hit anything."));
	}
}


// Pick vault type based on trace results
ECarbonParkourType UCarbonParkourComponent::ClassifyParkourType()
{
	CachedParkourSolution.bShouldFall = false; // Default to not falling
	if (bWallRunValid)
	{
		
	}
    if (!CachedForwardHit.IsValidBlockingHit() && (CachedWallRunLeftHit.bBlockingHit || CachedWallRunRightHit.bBlockingHit))
    {
    	if (CachedWallRunLeftHit.bBlockingHit) return ECarbonParkourType::WallRunLeft;
    	if (CachedWallRunRightHit.bBlockingHit) return ECarbonParkourType::WallRunRight;
        return ECarbonParkourType::None;
    }

    float ObjectHeight = CachedObjectHeights[1];
    if (ObjectHeight <= MinHeight)
    {
    	return ECarbonParkourType::None;
    }
    if (ObjectHeight <= MaxVaultHeight)
    {
		// Low height tic-tac ok, the animation seems to work. if not, see the bTicTacHitDistance logic below
    	bTicTacOffset = true;
    	if (CachedTicTacLeftHit.bBlockingHit) return ECarbonParkourType::VaultTicTacLeft;
    	if (CachedTicTacRightHit.bBlockingHit) return ECarbonParkourType::VaultTicTacRight;

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
    		if (CachedTicTacLeftHit.bBlockingHit)
    		{
    			bTicTacOffset = true;
    			if (CachedObjectDepth <= ObjectDepthShort) return ECarbonParkourType::TicTacLeft;
    			CachedParkourSolution.bShouldFall = false;
				return ECarbonParkourType::TicTacLeftOn;
    		}
    		if (CachedTicTacRightHit.bBlockingHit)
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
	
    return ECarbonParkourType::None;
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


// Function to add adjustment to cached hit locations
void UCarbonParkourComponent::AdjustCachedTicTacLocations(float AdjustmentValue)
{
	if (!bTicTacOffset)
	{
		return; // No adjustment needed
	}

	const FHitResult SideHit = CachedTicTacLeftHit.bBlockingHit ? CachedTicTacLeftHit : CachedTicTacRightHit;
	
	if (!SideHit.bBlockingHit)
	{
		return;
	}

	const FVector WallNormal = SideHit.ImpactNormal.GetSafeNormal();

	// This sets the target to exactly DesiredWallDistance away from the wall.
	AdjustedCachedTicTacHitLocation = SideHit.ImpactPoint + (WallNormal * AdjustmentValue);
}


// Function to adjust the cached wall run hit location
void UCarbonParkourComponent::AdjustCachedWallRunLocation(float AdjustmentValue)
{
	const FHitResult SideHit = CachedWallRunLeftHit.bBlockingHit ? CachedWallRunLeftHit : CachedWallRunRightHit;
	
	if (!SideHit.bBlockingHit)
	{
		return;
	}

	const FVector WallNormal = SideHit.ImpactNormal.GetSafeNormal();

	// This sets the target to exactly DesiredWallDistance away from the wall.
	AdjustedCachedRunHitLocation = SideHit.ImpactPoint + (WallNormal * AdjustmentValue);
}


// Build the parkour solution data
void UCarbonParkourComponent::BuildParkourSolution()
{
	// Validate owner character
	if (!OwnerChar.IsValid())
	{
		return; // Ensure the owner is valid before proceeding
	}
	
	// Initialize solution struct
	FCarbonParkourSolution Solution;
	Solution.Reset();

	// Validate forward hit
	if (!CachedForwardHit.IsValidBlockingHit() && !CachedWallRunLeftHit.IsValidBlockingHit() && !CachedWallRunRightHit.IsValidBlockingHit())
	{
		return;
	}

	// Get selected parkour type
	const ECarbonParkourType ParkourType = ClassifyParkourType();
	if (ParkourType == ECarbonParkourType::None)
	{
		return;
	}

	// Apply offset adjustments to the cached hit locations if needed
	if (bClimbLedge)
	{
		AdjustCachedLedgeLocations(LedgeOffsetYAdjustment, LedgeOffsetZAdjustment);
	}

	if (bTicTacOffset)
	{
		AdjustCachedTicTacLocations(SideOffsetAdjustment);
	}

	if (bWallRunValid)
	{
		AdjustCachedWallRunLocation(SideOffsetAdjustment);
	}
	
	// Set solution data
	Solution.bIsValid = true;
	Solution.ParkourType = ParkourType;
	
	// Apply motion warping logic
	Solution.WarpTargetLedge = TEXT("ParkourTargetLedge");
	Solution.WarpTargetTicTac = TEXT("ParkourTargetTicTac");
	Solution.WarpTargetWallRun = TEXT("ParkourTargetWallRun");

	// Surface data
	Solution.SurfaceNormal = CachedForwardHit.ImpactNormal;
	Solution.SideSurfaceNormal = CachedTicTacLeftHit.bBlockingHit ? CachedTicTacLeftHit.ImpactNormal : (CachedTicTacRightHit.bBlockingHit ? CachedTicTacRightHit.ImpactNormal : FVector::ZeroVector);
	Solution.RunSurfaceNormal = CachedWallRunLeftHit.bBlockingHit ? CachedWallRunLeftHit.ImpactNormal : (CachedWallRunRightHit.bBlockingHit ? CachedWallRunRightHit.ImpactNormal : FVector::ZeroVector);

	// Obstacle metrics
	Solution.ObstacleDistance = FVector::Dist(FVector(CachedForwardHit.ImpactPoint.X, CachedForwardHit.ImpactPoint.Y, 0.0f), FVector(CachedStartHeightHit.ImpactPoint.X, CachedStartHeightHit.ImpactPoint.Y, 0.0f));
	Solution.ObstacleHeight = CachedStartHeightHit.ImpactPoint.Z - CachedMidHeightHit.ImpactPoint.Z;
	Solution.ObstacleLength = FVector::Dist(CachedForwardHit.ImpactPoint, CachedBackwardHit.ImpactPoint);

	// Compute warp locations
	FVector Start = CachedStartHeightHit.ImpactPoint;
	FVector Ledge = CachedLedgeHeightHit.ImpactPoint;
	FVector Mid = CachedMidHeightHit.ImpactPoint;
	FVector Land = CachedLandHeightHit.ImpactPoint;
	FVector TicTac = AdjustedCachedTicTacHitLocation;
	FVector WallRun = AdjustedCachedRunHitLocation;

	// Cache target locations in solution struct for use in ability
	Solution.TargetTransformLedge = Ledge;
	Solution.TargetTransformTicTac = TicTac;
	Solution.TargetTransformWallRun = WallRun;

	// Build transforms
	const FRotator WarpRotation = FRotationMatrix::MakeFromXZ(- CachedForwardHit.ImpactNormal, FVector::UpVector).Rotator();
	const FRotator WarpTicRotation = FRotationMatrix::MakeFromXZ(- CachedForwardHit.ImpactNormal, FVector::UpVector).Rotator();
	// Transform the wall run normal
	const float RotationAngle = CachedWallRunLeftHit.bBlockingHit ? -90.0f : 90.0f; // Rotate -90° for left, 90° for right
	const FVector RotatedNormal = FQuat(FVector::UpVector, FMath::DegreesToRadians(RotationAngle)).RotateVector(CachedWallRunHit.ImpactNormal);
	const FRotator WarpRunRotation = FRotationMatrix::MakeFromXZ(RotatedNormal, FVector::UpVector).Rotator();

	// Note: The rotation is based on the negative impact normal to orient the character facing towards the obstacle, and using the world up vector to keep the character upright. Adjust as needed for specific animation requirements.
	Solution.WarpTransformLedge = FTransform(WarpRotation, Ledge);
	Solution.WarpTransformTicTac = FTransform(WarpTicRotation, TicTac);
	Solution.WarpTransformWallRun = FTransform(WarpRunRotation, WallRun);

	// Add side trace results
	Solution.bLeftTicTacHit = CachedTicTacLeftHit.bBlockingHit;
	Solution.bRightTicTacHit = CachedTicTacRightHit.bBlockingHit;
	Solution.LeftTicTacRotation = CachedLeftTicTacRotation;
	Solution.RightTicTacRotation = CachedRightTicTacRotation;

	// Add wall run trace results
	Solution.bLeftRunHit = CachedWallRunLeftHit.bBlockingHit;
	Solution.bRightRunHit = CachedWallRunRightHit.bBlockingHit;
	Solution.LeftRunRotation = CachedLeftRunRotation;
	Solution.RightRunRotation = CachedRightRunRotation;
	
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
		if (CachedTicTacLeftHit.bBlockingHit)
		{
			DrawDebugSphere(GetWorld(), TicTac, 10.f, 12, FColor::Orange, false, 5.0f);
			DrawDebugSphere(GetWorld(), CachedLeftTicTacHitLocation, 10.f, 12, FColor::Yellow, false, 5.0f);
		}
		if (CachedTicTacRightHit.bBlockingHit)
		{
			DrawDebugSphere(GetWorld(), TicTac, 10.f, 12, FColorList::Bronze, false, 5.0f);
			DrawDebugSphere(GetWorld(), CachedRightTicTacHitLocation, 10.f, 12, FColorList::Gold, false, 5.0f);
		}
		if (CachedWallRunLeftHit.bBlockingHit)
		{
			DrawDebugSphere(GetWorld(), WallRun, 10.f, 12, FColor::Black, false, 5.0f);
			DrawDebugSphere(GetWorld(), CachedLeftWallRunHitLocation, 10.f, 12, FColor::Magenta, false, 5.0f);
		}
		if (CachedWallRunRightHit.bBlockingHit)
		{
			DrawDebugSphere(GetWorld(), WallRun, 10.f, 12, FColor::Silver, false, 5.0f);
			DrawDebugSphere(GetWorld(), CachedRightWallRunHitLocation, 10.f, 12, FColor::White, false, 5.0f);
		}
	}
}


// Clear cached trace results
void UCarbonParkourComponent::ClearCachedTraces()
{
	UE_LOG(LogTemp, Warning, TEXT("ClearCachedTraces called"));

	CachedParkourSolution.Reset();
	CachedParkourType = ECarbonParkourType::None;

	CachedForwardHit = FHitResult();
	CachedBackwardHit = FHitResult();
	CachedStartHeightHit = FHitResult();
	CachedLedgeHeightHit = FHitResult();
	CachedMidHeightHit = FHitResult();
	CachedLandHeightHit = FHitResult();

	CachedTicTacLeftHit = FHitResult();
	CachedTicTacRightHit = FHitResult();
	CachedTicTacHit = FHitResult();

	CachedWallRunLeftHit = FHitResult();
	CachedWallRunRightHit = FHitResult();
	CachedWallRunHit = FHitResult();

	CachedLeftTicTacRotation = FRotator::ZeroRotator;
	CachedRightTicTacRotation = FRotator::ZeroRotator;
	CachedLeftRunRotation = FRotator::ZeroRotator;
	CachedRightRunRotation = FRotator::ZeroRotator;

	CachedLeftTicTacHitLocation = FVector::ZeroVector;
	CachedRightTicTacHitLocation = FVector::ZeroVector;
	
	CachedLeftWallRunHitLocation = FVector::ZeroVector;
	CachedRightWallRunHitLocation = FVector::ZeroVector;

	CachedGroundOffset = 0.f;
	CachedObjectDepth = 0.f;
	CachedObjectHeights.Reset();

	bTicTacHitDistance = false;
	bTicTacOffset = false;
	bClimbLedge = false;
	bWallRunValid = false;
	isRunning = false;

	UE_LOG(LogTemp, Warning, TEXT("Cached variables cleared"));
}


// Getter for the cached parkour solution
const FCarbonParkourSolution& UCarbonParkourComponent::GetCachedParkourSolution() const
{
	return CachedParkourSolution;
}
