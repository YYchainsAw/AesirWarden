#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "AesirAttributeSet.generated.h"

#define AESIR_ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
		GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
		GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
		GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
		GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

struct FGameplayEffectModCallbackData;

UCLASS()
class AESIRWARDEN_API UAesirAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UAesirAttributeSet();

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|Attributes|Health")
	FGameplayAttributeData Health;
	AESIR_ATTRIBUTE_ACCESSORS(UAesirAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|Attributes|Health")
	FGameplayAttributeData MaxHealth;
	AESIR_ATTRIBUTE_ACCESSORS(UAesirAttributeSet, MaxHealth)
	
	UPROPERTY(BlueprintReadOnly, Category = "Aesir|Attributes|Health")
	FGameplayAttributeData Damage;
	AESIR_ATTRIBUTE_ACCESSORS(UAesirAttributeSet, Damage)

protected:
	virtual void PreAttributeChange(
		const FGameplayAttribute& Attribute,
		float& NewValue) override;

	virtual void PostGameplayEffectExecute(
		const FGameplayEffectModCallbackData& Data) override;
};

#undef AESIR_ATTRIBUTE_ACCESSORS