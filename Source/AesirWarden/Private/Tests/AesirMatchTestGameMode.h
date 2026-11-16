#pragma once

#include "Framework/AesirCombatPrototypeGameMode.h"
#include "AesirMatchTestGameMode.generated.h"

/** Concrete native match mode for isolated gameplay tests. */
UCLASS(NotBlueprintable, Transient)
class AAesirMatchTestGameMode : public AAesirCombatPrototypeGameMode
{
	GENERATED_BODY()
};
