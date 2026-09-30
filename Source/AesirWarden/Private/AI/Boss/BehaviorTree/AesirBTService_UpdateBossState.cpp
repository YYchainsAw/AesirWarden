#include "AI/Boss/BehaviorTree/AesirBTService_UpdateBossState.h"
#include "AI/Boss/AesirBossObservationComponent.h"
#include "AI/Boss/AesirBossObservationTypes.h"
#include "AI/Boss/AesirBossAIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/Enemy/AesirEnemyCharacter.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	const FName TargetActorKey(TEXT("TargetActor"));
	const FName IsStunnedKey(TEXT("IsStunned"));
	const FName IsDeadKey(TEXT("IsDead"));
	const FName DistanceToTargetKey(TEXT("DistanceToTarget"));
	const FName BossHealthRatioKey(TEXT("BossHealthRatio"));
	const FName TargetHealthRatioKey(TEXT("TargetHealthRatio"));
	const FName HasLineOfSightKey(TEXT("HasLineOfSight"));
	const FName TargetIsBlockingKey(TEXT("TargetIsBlocking"));
	const FName TargetIsAttackingKey(TEXT("TargetIsAttacking"));
	const FName TargetAttackJustEndedKey(TEXT("TargetAttackJustEnded"));
	const FName BossJustRecoveredFromStunKey(
		TEXT("BossJustRecoveredFromStun"));
}

UAesirBTService_UpdateBossState::UAesirBTService_UpdateBossState()
{
	NodeName = TEXT("Update Boss State");
	Interval = 0.2f;
	RandomDeviation = 0.0f;
	bCreateNodeInstance = true;

	INIT_SERVICE_NODE_NOTIFY_FLAGS();
}

void UAesirBTService_UpdateBossState::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
	
	AAesirBossAIController* AIController =
		Cast<AAesirBossAIController>(OwnerComp.GetAIOwner());
	AAesirEnemyCharacter* Boss =
		AIController ? Cast<AAesirEnemyCharacter>(AIController->GetPawn()) : nullptr;

	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();

	if (!IsValid(AIController) ||
		!IsValid(Boss) ||
		!IsValid(Blackboard))
	{
		return;
	}

	AActor* Target = UGameplayStatics::GetPlayerPawn(Boss, 0);

	Blackboard->SetValueAsObject(
		TargetActorKey,
		Target);

	const float WorldTimeSeconds = Boss->GetWorld()
		? Boss->GetWorld()->GetTimeSeconds()
		: 0.0f;
	const bool bBossStunned = Boss->IsStunned();
	if (bHasBossStunSample && bWasBossStunned && !bBossStunned)
	{
		StunRecoverySignalUntil =
			WorldTimeSeconds + StunRecoverySignalDuration;
	}

	Blackboard->SetValueAsBool(IsStunnedKey, bBossStunned);
	Blackboard->SetValueAsBool(IsDeadKey, Boss->IsDead());
	Blackboard->SetValueAsBool(
		BossJustRecoveredFromStunKey,
		!bBossStunned &&
		WorldTimeSeconds < StunRecoverySignalUntil);
	bWasBossStunned = bBossStunned;
	bHasBossStunSample = true;

	UAesirBossObservationComponent* ObservationComponent =
		AIController->GetBossObservationComponent();
	if (!IsValid(ObservationComponent) || !IsValid(Target))
	{
		return;
	}

	const FAesirBossObservation Observation =
		ObservationComponent->CaptureObservation(Target);

	Blackboard->SetValueAsFloat(
		DistanceToTargetKey,
		Observation.DistanceToTarget);
	Blackboard->SetValueAsFloat(
		BossHealthRatioKey,
		Observation.BossHealthRatio);
	Blackboard->SetValueAsFloat(
		TargetHealthRatioKey,
		Observation.TargetHealthRatio);
	Blackboard->SetValueAsBool(
		HasLineOfSightKey,
		Observation.bHasLineOfSight);
	Blackboard->SetValueAsBool(
		TargetIsBlockingKey,
		Observation.bTargetBlocking);
	Blackboard->SetValueAsBool(
		TargetIsAttackingKey,
		Observation.bTargetAttacking);

	if (bHasTargetAttackSample &&
		bWasTargetAttacking &&
		!Observation.bTargetAttacking)
	{
		AttackEndedSignalUntil =
			WorldTimeSeconds + AttackEndedSignalDuration;
	}

	Blackboard->SetValueAsBool(
		TargetAttackJustEndedKey,
		WorldTimeSeconds < AttackEndedSignalUntil);
	bWasTargetAttacking = Observation.bTargetAttacking;
	bHasTargetAttackSample = true;
}
