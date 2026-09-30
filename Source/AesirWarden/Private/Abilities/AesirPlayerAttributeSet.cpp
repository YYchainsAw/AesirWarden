// Copyright © 2026 YYchainsAw. All Rights Reserved.

#include "Abilities/AesirPlayerAttributeSet.h"

#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffectExtension.h"

UAesirPlayerAttributeSet::UAesirPlayerAttributeSet()
{
	InitMaxGuardPressure(100.0f);
	InitGuardPressure(0.0f);

	InitPhysicalPower(1.0f);
	InitDamageReduction(0.0f);
	InitRunePower(1.0f);
	InitCooldownHaste(0.0f);
}

void UAesirPlayerAttributeSet::PreAttributeChange(
	const FGameplayAttribute& Attribute,
	float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetMaxGuardPressureAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.0f);
	}
	else if (Attribute == GetGuardPressureAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxGuardPressure());
	}
	else if (Attribute == GetPhysicalPowerAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetDamageReductionAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, 0.8f);
	}
	else if (Attribute == GetRunePowerAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetCooldownHasteAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, 2.0f);
	}
}

void UAesirPlayerAttributeSet::PostGameplayEffectExecute(
	const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	SetMaxGuardPressure(FMath::Max(
		GetMaxGuardPressure(),
		1.0f));

	SetGuardPressure(FMath::Clamp(
		GetGuardPressure(),
		0.0f,
		GetMaxGuardPressure()));
	
	if (Data.EvaluatedData.Attribute ==
		GetGuardPressureAttribute() &&
		GetGuardPressure() >= GetMaxGuardPressure())
	{
		SetGuardPressure(0.0f);

		if (AActor* AvatarActor = Data.Target.GetAvatarActor())
		{
			static const FGameplayTag GuardBrokenEventTag =
				FGameplayTag::RequestGameplayTag(
					FName(TEXT("Event.Player.Guard.Broken")));

			FGameplayEventData Payload;
			Payload.EventTag = GuardBrokenEventTag;
			Payload.Instigator = AvatarActor;
			Payload.Target = AvatarActor;

			UAbilitySystemBlueprintLibrary::
				SendGameplayEventToActor(
					AvatarActor,
					GuardBrokenEventTag,
					Payload);
		}
	}

	SetPhysicalPower(FMath::Max(
		GetPhysicalPower(),
		0.0f));

	SetDamageReduction(FMath::Clamp(
		GetDamageReduction(),
		0.0f,
		0.8f));

	SetRunePower(FMath::Max(
		GetRunePower(),
		0.0f));

	SetCooldownHaste(FMath::Clamp(
		GetCooldownHaste(),
		0.0f,
		2.0f));
}
