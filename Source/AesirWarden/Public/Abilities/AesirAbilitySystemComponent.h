#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AesirAbilitySystemComponent.generated.h"

UCLASS(ClassGroup = (Aesir), meta = (BlueprintSpawnableComponent))
class AESIRWARDEN_API UAesirAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()
	
public:
	UAesirAbilitySystemComponent();
};
