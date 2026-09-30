// Copyright (c) 2026 YYchainsAw. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AesirLevelFlowTypes.generated.h"

/** Level presentation and gameplay phases. Boss/result phases are reserved for later integration. */
UENUM(BlueprintType)
enum class EAesirLevelFlowState : uint8
{
	Uninitialized,
	MainMenu,
	OpeningCinematic,
	Exploration,
	BossIntro,
	BossCombat,
	EndingCinematic,
	Results
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FAesirLevelFlowStateChanged,
	EAesirLevelFlowState, PreviousState,
	EAesirLevelFlowState, NewState);
