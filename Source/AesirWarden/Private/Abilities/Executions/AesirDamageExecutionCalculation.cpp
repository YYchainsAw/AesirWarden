#include "Abilities/Executions/AesirDamageExecutionCalculation.h"

#include "AbilitySystemComponent.h"
#include "Abilities/AesirAttributeSet.h"
#include "Abilities/AesirPlayerAttributeSet.h"
#include "Tags/AesirGameplayTags.h"
#include "GameplayEffect.h"

void UAesirDamageExecutionCalculation::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	UAbilitySystemComponent* SourceASC =
		ExecutionParams.GetSourceAbilitySystemComponent();
	UAbilitySystemComponent* TargetASC =
		ExecutionParams.GetTargetAbilitySystemComponent();

	if (!TargetASC)
	{
		return;
	}

	const bool bUsesPlayerGuardPressure =
		TargetASC->HasAttributeSetForAttribute(
			UAesirPlayerAttributeSet::GetGuardPressureAttribute());

	if (bUsesPlayerGuardPressure &&
		TargetASC->HasMatchingGameplayTag(
			AesirGameplayTags::State_Blocking))
	{
		return;
	}

	const float BaseDamage = FMath::Max(
		Spec.GetSetByCallerMagnitude(
			AesirGameplayTags::Data_Damage,
			false,
			0.0f),
		0.0f);

	if (BaseDamage <= 0.0f)
		return;

	float PhysicalPower = 1.0f;

	if (SourceASC && SourceASC->HasAttributeSetForAttribute(
		UAesirPlayerAttributeSet::GetPhysicalPowerAttribute()))
	{
		PhysicalPower = SourceASC->GetNumericAttribute(
			UAesirPlayerAttributeSet::GetPhysicalPowerAttribute());
	}

	float DamageReduction = 0.0f;

	if (TargetASC->HasAttributeSetForAttribute(
		UAesirPlayerAttributeSet::GetDamageReductionAttribute()))
	{
		DamageReduction = TargetASC->GetNumericAttribute(
			UAesirPlayerAttributeSet::GetDamageReductionAttribute());
	}

	PhysicalPower = FMath::Max(PhysicalPower, 0.0f);
	DamageReduction = FMath::Clamp(DamageReduction, 0.0f, 0.8f);

	const float FinalDamage = FMath::Max(
		BaseDamage * PhysicalPower * (1.0f - DamageReduction),
		0.0f);

	if (FinalDamage <= 0.0f)
		return;

	OutExecutionOutput.AddOutputModifier(
		FGameplayModifierEvaluatedData(
			UAesirAttributeSet::GetDamageAttribute(),
			EGameplayModOp::Additive,
			FinalDamage));

	UE_LOG(LogTemp, Display,
		TEXT("GAS damage calculation: source=%s target=%s "
			"Base=%.1f PhysicalPower=%.2f "
			"DamageReduction=%.2f Final=%.1f"),
		*GetNameSafe(SourceASC ? SourceASC->GetAvatarActor() : nullptr),
		*GetNameSafe(TargetASC->GetAvatarActor()),
		BaseDamage,
		PhysicalPower,
		DamageReduction,
		FinalDamage);
}
