#include "Abilities/AesirAttributeSet.h"

#include "GameplayEffectExtension.h"

UAesirAttributeSet::UAesirAttributeSet()
{
	InitMaxHealth(100.0f);
	InitHealth(100.0f);
	InitDamage(0.0f);
}

void UAesirAttributeSet::PreAttributeChange(
	const FGameplayAttribute& Attribute,
	float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetMaxHealthAttribute())
		NewValue = FMath::Max(NewValue, 1.0f);
	else if (Attribute == GetHealthAttribute())
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
}

void UAesirAttributeSet::PostGameplayEffectExecute(
	const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	SetMaxHealth(FMath::Max(GetMaxHealth(), 1.0f));
	
	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		const float LocalDamage = FMath::Max(GetDamage(), 0.0f);
		SetDamage(0.0f);

		if (LocalDamage > 0.0f)
		{
			SetHealth(FMath::Clamp(
				GetHealth() - LocalDamage,
				0.0f,
				GetMaxHealth()));
		}
	}
	
	SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
}