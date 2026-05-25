// Copyright Tango Alpha LLC. All Rights Reserved.

#include "CarbonGameplayAbility_Parkour.h"

#include "CarbonParkourDataAsset.h"
#include "Character/LyraCharacter.h"
#include "Character/CarbonParkourComponent.h"
#include "MotionWarpingComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GameFramework/CharacterMovementComponent.h"


// The constructor
UCarbonGameplayAbility_Parkour::UCarbonGameplayAbility_Parkour()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted; // ability runs on the client and the server
	NetSecurityPolicy  = EGameplayAbilityNetSecurityPolicy::ClientOrServer; // either side can trigger the ability
	InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor; //each actor gets their own instance
}


//  Helper function to determine if we should change movement mode
bool UCarbonGameplayAbility_Parkour::ShouldChangeMovementMode(const FGameplayAbilityActorInfo* ActorInfo)
{
	if (!ActorInfo)
	{
		return false;
	}

	const AActor* Avatar = ActorInfo->AvatarActor.Get();
	if (!Avatar)
	{
		return false;
	}

	// Authority always sets the mode
	if (Avatar->HasAuthority())
	{
		return true;
	}

	// Locally controlled autonomous proxy sets it for prediction
	if (const APawn* Pawn = Cast<APawn>(Avatar))
	{
		if (Pawn->IsLocallyControlled())
		{
			return true;
		}
	}

	return false;
}


// Helper function to select the montage to play
UAnimMontage* UCarbonGameplayAbility_Parkour::SelectMontageForParkourType(
	ECarbonParkourType ParkourType,
	const FGameplayAbilityActivationInfo& ActivationInfo) const
{
	if (!ParkourData)
	{
		return nullptr;
	}

	const FCarbonParkourMove* MatchingMove = ParkourData->Moves.FindByPredicate(
		[ParkourType](const FCarbonParkourMove& Move)
		{
			return Move.ParkourType == ParkourType;
		});

	if (!MatchingMove || MatchingMove->Montages.Num() == 0)
	{
		return nullptr;
	}

	if (MatchingMove->Montages.Num() == 1)
	{
		return MatchingMove->Montages[0];
	}

	// Seed an random stream with the prediction key so the client and server pick same
	const int32 PredictionSeed = (int32)ActivationInfo.GetActivationPredictionKey().Current;
	const int32 Seed = (PredictionSeed != 0)
		? PredictionSeed
		: (int32)(uint8)ParkourType; // fallback: stable per parkour type

	FRandomStream Stream(Seed);
	const int32 Index = Stream.RandRange(0, MatchingMove->Montages.Num() - 1);
	return MatchingMove->Montages[Index];
}


// The main function where the ability is activated
void UCarbonGameplayAbility_Parkour::ActivateAbility(
  const FGameplayAbilitySpecHandle Handle,
  const FGameplayAbilityActorInfo* ActorInfo,
  const FGameplayAbilityActivationInfo ActivationInfo,
  const FGameplayEventData* TriggerEventData)
{
	UE_LOG(LogTemp, Warning, TEXT("Parkour Ability ActivateAbility Called (NetMode=%d HasAuthority=%d)"),
		ActorInfo && ActorInfo->AvatarActor.IsValid() ? (int32)ActorInfo->AvatarActor->GetNetMode() : -1,
		ActorInfo && ActorInfo->AvatarActor.IsValid() ? (int32)ActorInfo->AvatarActor->HasAuthority() : -1);

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

	// Run traces and build a solution
	ParkourComp->VaultSolution();

	// Get the cached parkour solution and validate
	const FCarbonParkourSolution& Solution = ParkourComp->GetCachedParkourSolution();
	if (!Solution.bIsValid)
	{
		UE_LOG(LogTemp, Warning, TEXT("Parkour solution invalid (likely no obstacle in range)"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
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

	// Select montage. Client and server pick the same one for a given activation
	UAnimMontage* MontageToPlay = SelectMontageForParkourType(Solution.ParkourType, ActivationInfo);
	if (!MontageToPlay)
	{
		UE_LOG(LogTemp, Warning, TEXT("No montage found for ParkourType %s"), *UEnum::GetValueAsString(Solution.ParkourType));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Apply warp targets before playing montage
	if (ParkourComp->bEnableMotionWarping)
	{
		UE_LOG(LogTemp, Warning, TEXT("Applying warp targets before montage play"));
		ApplyWarpTargets(LyraChar, Solution);
	}

	// Set movement mode to flying
	UCharacterMovementComponent* MoveComp = LyraChar->GetCharacterMovement();
	if (MoveComp && ShouldChangeMovementMode(ActorInfo))
	{
		UE_LOG(LogTemp, Warning, TEXT("PreMontage MovementMode=%d IsMovingOnGround=%d"),
			(int32)MoveComp->MovementMode, MoveComp->IsMovingOnGround());

		MoveComp->SetMovementMode(MOVE_Flying);
	}

	// Diagnostic logging
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
}


// Callbacks for montage completion and cancellation to end the ability
void UCarbonGameplayAbility_Parkour::OnMontageCompleted()
{
	UE_LOG(LogTemp, Warning, TEXT("Parkour montage completed"));

	if (ALyraCharacter* LyraChar = Cast<ALyraCharacter>(CurrentActorInfo->AvatarActor.Get()))
	{
		if (UCharacterMovementComponent* MoveComp = LyraChar->GetCharacterMovement())
		{
			// Only the authority + local proxy change the movement mode
			// remote simulated proxies receive it via CMC replication
			if (ShouldChangeMovementMode(CurrentActorInfo))
			{
				// Do not force walking if we were already in another mode
				if (!AbilityFalling && MoveComp->MovementMode != MOVE_Falling)
				{
					MoveComp->SetMovementMode(MOVE_Walking);
				}
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
			if (ShouldChangeMovementMode(CurrentActorInfo))
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
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}


// Function to apply motion warping targets based on the parkour solution
void UCarbonGameplayAbility_Parkour::ApplyWarpTargets(ALyraCharacter* Character, const FCarbonParkourSolution& Solution)
{
	UE_LOG(LogTemp, Warning, TEXT("Applying warp targets."));

	if (!Character) return;

	UMotionWarpingComponent* MotionWarpComp = Character->FindComponentByClass<UMotionWarpingComponent>();

	UE_LOG(LogTemp, Warning, TEXT("Applying warp targets. MotionWarp=%s"), MotionWarpComp ? TEXT("YES") : TEXT("NO"));

	if (!MotionWarpComp) return;

	// Add warp targets if specified
	if (!Solution.WarpTargetTicTac.IsNone())
	{
		MotionWarpComp->AddOrUpdateWarpTarget(FMotionWarpingTarget(
			Solution.WarpTargetTicTac,
			Solution.WarpTransformTicTac
		));
	}

	if (!Solution.WarpTargetLedge.IsNone())
	{
		MotionWarpComp->AddOrUpdateWarpTarget(FMotionWarpingTarget(
			Solution.WarpTargetLedge,
			Solution.WarpTransformLedge
		));
	}

	if (!Solution.WarpTargetWallRun.IsNone())
	{
		MotionWarpComp->AddOrUpdateWarpTarget(FMotionWarpingTarget(
			Solution.WarpTargetWallRun,
			Solution.WarpTransformWallRun
		));
	}

	UE_LOG(LogTemp, Warning, TEXT("Applying warp targets. MotionWarp=%s"), MotionWarpComp ? TEXT("YES") : TEXT("NO"));
}
