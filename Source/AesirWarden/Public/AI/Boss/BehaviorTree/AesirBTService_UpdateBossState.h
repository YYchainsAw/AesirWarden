#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "AesirBTService_UpdateBossState.generated.h"

class UBehaviorTreeComponent;

UCLASS()
class AESIRWARDEN_API UAesirBTService_UpdateBossState
	: public UBTService
{
	GENERATED_BODY()

public:
	UAesirBTService_UpdateBossState();

protected:
	virtual void TickNode(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory,
		float DeltaSeconds) override;

private:
	UPROPERTY(EditAnywhere, Category = "Aesir|AI|Boss|Reaction",
		meta = (ClampMin = "0.0"))
	float AttackEndedSignalDuration = 0.6f;

	/** Brief decision window opened when the Boss exits stun or attack recoil. */
	UPROPERTY(EditAnywhere, Category = "Aesir|AI|Boss|Reaction",
		meta = (ClampMin = "0.0"))
	float StunRecoverySignalDuration = 0.75f;

	float AttackEndedSignalUntil = 0.0f;
	float StunRecoverySignalUntil = 0.0f;
	bool bWasTargetAttacking = false;
	bool bHasTargetAttackSample = false;
	bool bWasBossStunned = false;
	bool bHasBossStunSample = false;
};
