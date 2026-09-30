// Copyright © 2026 YYchainsAw. All Rights Reserved.

#include "AI/Boss/BehaviorTree/AesirBTTask_BossAttack.h"

#include "AI/Boss/AesirBossActionComponent.h"
#include "AI/Boss/AesirBossAIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/Enemy/AesirEnemyCharacter.h"

namespace
{
	const FName BossAttackTargetActorKey(TEXT("TargetActor"));
}

UAesirBTTask_BossAttack::UAesirBTTask_BossAttack()
{
	NodeName = TEXT("Execute Boss GAS Action");
	bNotifyTick = true;
}

EBTNodeResult::Type UAesirBTTask_BossAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAesirBossAIController* BossController =
		Cast<AAesirBossAIController>(OwnerComp.GetAIOwner());
	
	UBlackboardComponent* Blackboard =
		OwnerComp.GetBlackboardComponent();
	
	AActor* Target = Blackboard
		? Cast<AActor>(Blackboard->GetValueAsObject(
			BossAttackTargetActorKey))
		: nullptr;

	if (!IsValid(BossController))
		return EBTNodeResult::Failed;

	const AAesirEnemyCharacter* BossCharacter =
		Cast<AAesirEnemyCharacter>(BossController->GetPawn());
	if (IsValid(BossCharacter) &&
		(BossCharacter->IsDead() ||
		 BossCharacter->IsStunned() ||
		 BossCharacter->IsAttacking() ||
		 BossCharacter->IsDodging() ||
		 BossCharacter->IsDefending() ||
		 BossCharacter->IsExecutingMovementAction()))
	{
		return EBTNodeResult::Succeeded;
	}

	const FAesirBossActionOutcome Outcome =
		BossController->ExecuteBossDecision(Action, Target);

	if (!Outcome.WasAccepted())
	{
		if (Outcome.Result == EAesirBossActionResult::Dead ||
			Outcome.Result == EAesirBossActionResult::Stunned ||
			Outcome.Result == EAesirBossActionResult::Busy)
		{
			return EBTNodeResult::Succeeded;
		}

		return EBTNodeResult::Failed;
	}

	UAesirBossActionComponent* ActionComponent =
		BossController->GetBossActionComponent();
	return IsValid(ActionComponent) &&
		ActionComponent->IsActionActive(Action)
		? EBTNodeResult::InProgress
		: EBTNodeResult::Succeeded;
}

void UAesirBTTask_BossAttack::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);
	
	AAesirBossAIController* BossController =
		Cast<AAesirBossAIController>(OwnerComp.GetAIOwner());
	UAesirBossActionComponent* ActionComponent = BossController
		? BossController->GetBossActionComponent()
		: nullptr;

	if (!IsValid(ActionComponent) ||
		!IsValid(ActionComponent->GetAvatar()))
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	if (!ActionComponent->IsActionActive(Action))
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
}
