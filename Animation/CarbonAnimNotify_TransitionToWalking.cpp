// Copyright Tango Alpha LLC. All Rights Reserved.

#include "CarbonAnimNotify_TransitionToWalking.h"

#include "Character/LyraCharacter.h"
#include "Character/CarbonParkourComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

FString UCarbonAnimNotify_TransitionToWalking::GetNotifyName_Implementation() const
{
	return TEXT("TransitionToWalking");
}

void UCarbonAnimNotify_TransitionToWalking::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (!MeshComp)
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		return;
	}

	if (ALyraCharacter* LyraChar = Cast<ALyraCharacter>(Owner))
	{
		if (UCarbonParkourComponent* ParkourComp = LyraChar->FindComponentByClass<UCarbonParkourComponent>())
		{
			if (UCharacterMovementComponent* MoveComp = LyraChar->GetCharacterMovement())
			{
				// Only transition once
				if (MoveComp->MovementMode != MOVE_Walking)
				{
					MoveComp->SetMovementMode(MOVE_Walking); // Let the character actually re-enter walking state
					UE_LOG(LogTemp, Warning, TEXT("Movement mode set to Walking successfully."));
				}
			}
		}
	}
}

