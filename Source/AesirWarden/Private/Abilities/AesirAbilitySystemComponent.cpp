#include "Abilities/AesirAbilitySystemComponent.h"

UAesirAbilitySystemComponent::UAesirAbilitySystemComponent()
{
	SetIsReplicatedByDefault(true);
	SetReplicationMode(EGameplayEffectReplicationMode::Full);
}
