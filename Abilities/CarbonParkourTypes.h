// Copyright Tango Alpha LLC. All Rights Reserved.

#pragma once

#include "CarbonParkourTypes.generated.h"

UENUM(BlueprintType)
enum class ECarbonParkourType : uint8
{
	None				UMETA(DisplayName="None"),

	VaultXShort			UMETA(DisplayName="Vault X Short"),
	VaultShort			UMETA(DisplayName="Vault Short"),
	VaultMedium			UMETA(DisplayName="Vault Medium"),
	VaultLong			UMETA(DisplayName="Vault Long "),
	VaultOn				UMETA(DisplayName="Vault On"),

	VaultTicTacLeft		UMETA(DisplayName="Vault Tic-Tac Left"),
	VaultTicTacRight	UMETA(DisplayName="Vault Tic-Tac Right"),

	VaultHighXShort		UMETA(DisplayName="Vault High X Short"),
	VaultHighShort		UMETA(DisplayName="Vault High Short"),
	VaultHighOn			UMETA(DisplayName="High Vault On"),
	
	TicTacLeft			UMETA(DisplayName="Tic-Tac Left"),
	TicTacRight			UMETA(DisplayName="Tic-Tac Right"),
	TicTacLeftOn		UMETA(DisplayName="Tic-Tac Left On"),
	TicTacRightOn		UMETA(DisplayName="Tic-Tac Right On"),

	WallRunLeft			UMETA(DisplayName="Wall Run Left"),
	WallRunRight		UMETA(DisplayName="Wall Run Right"),

	WallRunOver			UMETA(DisplayName="Wall Run Over"),
	WallRunOn			UMETA(DisplayName="Wall Run On"),
};