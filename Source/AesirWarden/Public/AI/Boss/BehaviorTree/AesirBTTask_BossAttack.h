// Copyright © 2026 YYchainsAw. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AI/Boss/AesirBossDecisionTypes.h"
#include "BehaviorTree/BTTaskNode.h"
#include "AesirBTTask_BossAttack.generated.h"

UCLASS()
class AESIRWARDEN_API UAesirBTTask_BossAttack : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UAesirBTTask_BossAttack();

protected:
	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory) override;

	virtual void TickTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory,
		float DeltaSeconds) override;

private:
	UPROPERTY(EditAnywhere, Category = "Aesir|AI|Boss")
	EAesirBossAction Action = EAesirBossAction::LightAttack;
};
