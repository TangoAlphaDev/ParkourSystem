// Copyright Tango Alpha LLC. All Rights Reserved.

#include "CarbonAnimNotify_TransitionToFalling.h"

#include "Character/LyraCharacter.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Character/CarbonParkourComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

FString UCarbonAnimNotify_TransitionToFalling::GetNotifyName_Implementation() const
{
	return TEXT("TransitionToFalling");
}

void UCarbonAnimNotify_TransitionToFalling::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	UE_LOG(LogTemp, Warning, TEXT("Transition Called."));
	if (!MeshComp)
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		return;
	}

	// Preferred: route through your character/component, not directly through anim BP.
	if (ALyraCharacter* LyraChar = Cast<ALyraCharacter>(Owner))
	{
		if (UCarbonParkourComponent* ParkourComp = LyraChar->FindComponentByClass<UCarbonParkourComponent>())
		{
			if (UCharacterMovementComponent* MoveComp = LyraChar->GetCharacterMovement())
			{
				// Only transition once
				if (MoveComp->MovementMode != MOVE_Falling)
				{
					MoveComp->SetMovementMode(MOVE_Falling); // Let the character actually re-enter the air state immediately
					UE_LOG(LogTemp, Warning, TEXT("Movement mode set to Falling successfully."));
				}
			}
		}
	}
}

