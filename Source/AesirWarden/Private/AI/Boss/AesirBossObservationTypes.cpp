#include "AI/Boss/AesirBossObservationTypes.h"

TArray<float> FAesirBossObservation::ToFeatureVector() const
{
	return {
		BossHealthRatio,
		TargetHealthRatio,
		NormalizedDistance,
		FacingAlignment,
		bHasLineOfSight ? 1.0f : 0.0f,
		bBossStunned ? 1.0f : 0.0f,
		bBossAttacking ? 1.0f : 0.0f,
		bTargetBlocking ? 1.0f : 0.0f,
		bTargetAttacking ? 1.0f : 0.0f,
		bTargetDead ? 1.0f : 0.0f,
		BossPoiseRatio,
		TargetGuardPressureRatio,
		bTargetDodging ? 1.0f : 0.0f,
		RecentTargetAttackRate,
		RecentTargetBlockRate,
		RecentTargetDodgeRate,
		DistanceTrend,
		bLightAttackAvailable ? 1.0f : 0.0f,
		bHeavyAttackAvailable ? 1.0f : 0.0f,
		bDefendAvailable ? 1.0f : 0.0f,
		bDodgeAvailable ? 1.0f : 0.0f,
		bPursueAvailable ? 1.0f : 0.0f,
		bDisengageAvailable ? 1.0f : 0.0f,
		bUseAbilityAvailable ? 1.0f : 0.0f,
		bGapCloserSkillAvailable ? 1.0f : 0.0f,
		bUnblockableAreaSkillAvailable ? 1.0f : 0.0f
	};
}
