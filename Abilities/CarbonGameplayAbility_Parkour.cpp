// Copyright Tango Alpha LLC. All Rights Reserved.

#include "CarbonGameplayAbility_Parkour.h"

#include "CarbonParkourDataAsset.h"
#include "Character/LyraCharacter.h"
#include "Character/CarbonParkourComponent.h"
#include "MotionWarpingComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GameFramework/CharacterMovementComponent.h"

void UCarbonGameplayAbility_Parkour::ActivateAbility(
  const FGameplayAbilitySpecHandle Handle,
  const FGameplayAbilityActorInfo* ActorInfo,
  const FGameplayAbilityActivationInfo ActivationInfo,
  const FGameplayEventData* TriggerEventData)
{
	UE_LOG(LogTemp, Warning, TEXT("Parkour Ability ActivateAbility Called"));

	// Get and validate Lyra Character
	ALyraCharacter* LyraChar = Cast<ALyraCharacter>(ActorInfo->AvatarActor.Get());
	if (!LyraChar)
	{
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	return;
	}

	// Get and validate the ParkourComponent
	UCarbonParkourComponent* ParkourComp = LyraChar->GetCarbonParkourComponent();
	if (!ParkourComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("No CarbonParkourComponent"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// Commit the ability
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		UE_LOG(LogTemp, Warning, TEXT("CommitAbility failed"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Call the VaultSolution function
	ParkourComp->VaultSolution();

	// Get the cached parkour solution and validate
	const FCarbonParkourSolution& Solution = ParkourComp->GetCachedParkourSolution();
	if (!Solution.bIsValid)
	{
		UE_LOG(LogTemp, Warning, TEXT("Parkour solution invalid"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// Align the character to the wall based on side trace results
	if (Solution.bLeftSideHit)
	{
		LyraChar->SetActorRotation(Solution.LeftSideRotation);
	}
	else if (Solution.bRightSideHit)
	{
		LyraChar->SetActorRotation(Solution.RightSideRotation);
	}

	// Set local bool
	AbilityFalling = Solution.bShouldFall;

	// Validate the Parkour Data
	if (!ParkourData)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParkourData asset not set on ability"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Get and validate montage
	UAnimMontage* MontageToPlay = nullptr;
	for (const FCarbonParkourMove& Move : ParkourData->Moves)
	{
		if (Move.ParkourType == Solution.ParkourType)
		{
			MontageToPlay = Move.Montage;
			break;
		}
	}

	if (!MontageToPlay)
	{
		UE_LOG(LogTemp, Warning, TEXT("No montage found for ParkourType %s"), *UEnum::GetValueAsString(Solution.ParkourType));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Apply warp targets before playing montage
	ApplyWarpTargets(LyraChar, Solution);

	// Set movement mode to flying and stop movement to ensure proper montage play and warping (this is a test, may need adjustments based on actual character setup and desired behavior)
	UCharacterMovementComponent* MoveComp = LyraChar->GetCharacterMovement();
	if (MoveComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("PreMontage MovementMode=%d IsMovingOnGround=%d"), (int32)MoveComp->MovementMode, MoveComp->IsMovingOnGround());

		MoveComp->SetMovementMode(MOVE_Flying);
	}

	// testing
	UE_LOG(LogTemp, Warning, TEXT("Montage: %s  HasRootMotion=%d"), *GetNameSafe(MontageToPlay), MontageToPlay->HasRootMotion());
	if (USkeletalMeshComponent* Mesh = LyraChar->GetMesh())
	{
		if (UAnimInstance* AnimInst = Mesh->GetAnimInstance())
		{
			UE_LOG(LogTemp, Warning, TEXT("AnimInstance=%s  IsAnyMontagePlaying=%d"), *GetNameSafe(AnimInst), AnimInst->IsAnyMontagePlaying());
		}
	}
	if (MoveComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("MovementMode=%d  IsMovingOnGround=%d "), (int32)MoveComp->MovementMode, MoveComp->IsMovingOnGround());
	}
	
	// Play montage and wait
	UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		MontageToPlay,
		1.f,
		NAME_None,
		true
	  );

	Task->OnCompleted.AddDynamic(this, &UCarbonGameplayAbility_Parkour::OnMontageCompleted);
	Task->OnInterrupted.AddDynamic(this, &UCarbonGameplayAbility_Parkour::OnMontageCancelled);
	Task->OnCancelled.AddDynamic(this, &UCarbonGameplayAbility_Parkour::OnMontageCancelled);
	Task->ReadyForActivation();

	UE_LOG(LogTemp, Warning, TEXT("Solution Valid=%d Type=%s"), Solution.bIsValid, *UEnum::GetValueAsString(Solution.ParkourType));

	UE_LOG(LogTemp, Warning, TEXT("Warp Start=%s Loc=%s"), *Solution.WarpTargetStart.ToString(), *Solution.WarpTransformStart.GetLocation().ToString());

	UE_LOG(LogTemp, Warning, TEXT("Warp Mid=%s Loc=%s"), *Solution.WarpTargetMiddle.ToString(), *Solution.WarpTransformMid.GetLocation().ToString());

	UE_LOG(LogTemp, Warning, TEXT("Warp End=%s Loc=%s"), *Solution.WarpTargetEnd.ToString(), *Solution.WarpTransformEnd.GetLocation().ToString());
}


// Callbacks for montage completion and cancellation to end the ability
void UCarbonGameplayAbility_Parkour::OnMontageCompleted()
{
	UE_LOG(LogTemp, Warning, TEXT("Parkour montage completed"));

	if (ALyraCharacter* LyraChar = Cast<ALyraCharacter>(CurrentActorInfo->AvatarActor.Get()))
	{
		if (UCharacterMovementComponent* MoveComp = LyraChar->GetCharacterMovement())
		{
			// Do not force walking here.
			if (!AbilityFalling && MoveComp->MovementMode != MOVE_Falling)
			{
				MoveComp->SetMovementMode(MOVE_Walking);
			}
		}
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UCarbonGameplayAbility_Parkour::OnMontageCancelled()
{
	UE_LOG(LogTemp, Warning, TEXT("Parkour montage cancelled"));

	// Reset movement mode to walking after montage completes or is cancelled
	if (ALyraCharacter* LyraChar = Cast<ALyraCharacter>(CurrentActorInfo->AvatarActor.Get()))
	{
		if (UCharacterMovementComponent* MoveComp = LyraChar->GetCharacterMovement())
		{
			if (AbilityFalling)
			{
				MoveComp->SetMovementMode(MOVE_Falling);
				AbilityFalling = false;
			}
			else
			{
				MoveComp->SetMovementMode(MOVE_Walking);
			}
		}
	}
	
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}


// Function to apply motion warping targets based on the parkour solution
void UCarbonGameplayAbility_Parkour::ApplyWarpTargets(ALyraCharacter* Character, const FCarbonParkourSolution& Solution)
{
	if (!Character) return;

	UMotionWarpingComponent* MotionWarpComp = Character->FindComponentByClass<UMotionWarpingComponent>();

	if (!MotionWarpComp) return;

	// Add start warp target
	MotionWarpComp->AddOrUpdateWarpTarget(FMotionWarpingTarget(
		Solution.WarpTargetStart,
		Solution.WarpTransformStart
	));

	// Add tic-tac warp target if specified
	if (!Solution.WarpTargetTicTac.IsNone())
	{
		MotionWarpComp->AddOrUpdateWarpTarget(FMotionWarpingTarget(
			Solution.WarpTargetTicTac,
			Solution.WarpTransformTicTac
		));
	}

	// Add middle warp target if specified
	if (!Solution.WarpTargetMiddle.IsNone())
	{
		MotionWarpComp->AddOrUpdateWarpTarget(FMotionWarpingTarget(
			Solution.WarpTargetMiddle,
			Solution.WarpTransformMid
		));
	}

	// Add end warp target
	if (!Solution.WarpTargetEnd.IsNone())
	{
		MotionWarpComp->AddOrUpdateWarpTarget(FMotionWarpingTarget(
			Solution.WarpTargetEnd,
			Solution.WarpTransformEnd
		));
	}

	UE_LOG(LogTemp, Warning, TEXT("Applying warp targets. MotionWarp=%s"), MotionWarpComp ? TEXT("YES") : TEXT("NO"));
}

