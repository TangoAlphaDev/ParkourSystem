// Copyright Tango Alpha LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"

#include "CarbonAnimNotify_TransitionToFalling.generated.h"

UCLASS()
class LYRAGAME_API UCarbonAnimNotify_TransitionToFalling : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual FString GetNotifyName_Implementation() const override;

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
};