// Copyright Tango Alpha LLC. All Rights Reserved.

#pragma once

#include "CarbonParkourTypes.generated.h"

UENUM(BlueprintType)
enum class ECarbonParkourType : uint8
{
	None				UMETA(DisplayName="None"),

	Step				UMETA(DisplayName="Step"),
	StepOn				UMETA(DisplayName="Step On"),
		
	VaultShort			UMETA(DisplayName="Short Vault"),
	VaultMedium			UMETA(DisplayName="Medium Vault"),
	VaultLong			UMETA(DisplayName="Long Vault"),
	VaultOn				UMETA(DisplayName="Vault On"),

	TicTacVaultLeft		UMETA(DisplayName="Tic-Tac Left Vault"),
	TicTacVaultRight	UMETA(DisplayName="Tic-Tac Right Vault"),
	
	VaultHigh			UMETA(DisplayName="High Vault"),
	VaultHighOn			UMETA(DisplayName="High Vault On"),
	
	TicTacHighLeft		UMETA(DisplayName="Tic-Tac High Left Vault"),
	TicTacHighRight		UMETA(DisplayName="Tic-Tac High Right Vault"),
	TicTacHighLeftOn	UMETA(DisplayName="Tic-Tac High Left On"),
	TicTacHighRightOn	UMETA(DisplayName="Tic-Tac High Right On"),

	ClimbUpLow			UMETA(DisplayName="Climb Up Low"),
	ClimbUpHigh			UMETA(DisplayName="Climb Up High"),
};