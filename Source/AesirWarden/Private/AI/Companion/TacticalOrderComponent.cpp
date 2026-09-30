#include "AI/Companion/TacticalOrderComponent.h"
#include "Characters/Companion/AesirCompanionCharacter.h"
#include "Combat/AesirHealthComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Combat/AesirCombatStateComponent.h"

UTacticalOrderComponent::UTacticalOrderComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	SupportedAbilityIds = {
		TEXT("ability.alice.explosion"),
		TEXT("ability.alice.basic_attack")
	};
}

void UTacticalOrderComponent::BeginPlay()
{
	Super::BeginPlay();
}

bool UTacticalOrderComponent::ValidateOrder(
	FTacticalOrder Order,
	FString& FailureReason) const
{
	const AAesirCompanionCharacter* Companion =
		Cast<AAesirCompanionCharacter>(GetOwner());

	if (!IsValid(Companion))
	{
		FailureReason = TEXT("组件 Owner 不是有效队友。");
		return false;
	}

	const FName CompanionAgentId = Companion->GetCompanionId();
	const FName OrderAgentId(*Order.AgentId.TrimStartAndEnd());

	if (CompanionAgentId != OrderAgentId)
	{
		FailureReason = FString::Printf(
			TEXT("agent_id 不匹配：order='%s' companion='%s' owner='%s'"),
			*Order.AgentId,
			*CompanionAgentId.ToString(),
			*GetNameSafe(Companion));
		return false;
	}

	const APawn* CompanionPawn = Cast<APawn>(Companion);
	if (!CompanionPawn || !IsValid(CompanionPawn->GetController()))
	{
		FailureReason = TEXT("队友当前没有有效 AI Controller。");
		return false;
	}

	const UAesirHealthComponent* Health =
		Companion->FindComponentByClass<UAesirHealthComponent>();

	if (!IsValid(Health) || Health->IsDead())
	{
		FailureReason = TEXT("队友不存在生命组件或已经死亡。");
		return false;
	}

	if (Order.Intent == ETacticalOrderIntent::Unknown ||
		Order.Then.Type == ETacticalThenType::Unknown ||
		Order.Expires.Type != ETacticalExpiresType::EncounterEnd)
	{
		FailureReason = TEXT("命令包含未实现的类型。");
		return false;
	}

	const bool bNeedsAbility =
		Order.Then.Type == ETacticalThenType::CastAbility ||
		Order.Then.Type == ETacticalThenType::HoldAbility;

	if (bNeedsAbility &&
		!SupportedAbilityIds.Contains(FName(*Order.Then.AbilityId)))
	{
		FailureReason = FString::Printf(
			TEXT("队友能力目录不包含：%s"),
			*Order.Then.AbilityId);
		return false;
	}
	
	AActor* ResolvedSubject = nullptr;

	if (Order.bHasWhen)
	{
		if (Order.When.Type != ETacticalWhenType::StateEntered ||
			Order.When.Subject.IsEmpty())
		{
			FailureReason = TEXT("when.subject 结构不合法。");
			return false;
		}

		ResolvedSubject =
			ResolveTargetSelector(Order.When.Subject);

		if (!IsValid(ResolvedSubject))
		{
			FailureReason = FString::Printf(
				TEXT("无法解析 subject：%s"),
				*Order.When.Subject);
			return false;
		}
	}

	const bool bNeedsTarget =
		Order.Then.Type == ETacticalThenType::CastAbility ||
		Order.Then.Type == ETacticalThenType::Follow;

	if (bNeedsTarget)
	{
		if (Order.Then.Target.Type == ETacticalTargetType::Selector)
		{
			if (!IsValid(ResolveTargetSelector(
				Order.Then.Target.Selector)))
			{
				FailureReason = FString::Printf(
					TEXT("无法解析 target：%s"),
					*Order.Then.Target.Selector);
				return false;
			}
		}
		else if (Order.Then.Target.Type ==
			ETacticalTargetType::Reference)
		{
			if (Order.Then.Target.Reference != TEXT("when.subject") ||
				!IsValid(ResolvedSubject))
			{
				FailureReason = TEXT("target 对 when.subject 的引用无效。");
				return false;
			}
		}
		else
		{
			FailureReason = TEXT("需要目标的命令没有提供 target。");
			return false;
		}
	}

	FailureReason = TEXT("战术命令本地校验通过。");
	return true;
}

void UTacticalOrderComponent::BeginEncounter()
{
	bEncounterActive = true;
	bHasActiveOrder = false;
	ActiveOrder = FTacticalOrder{};
}

void UTacticalOrderComponent::EndEncounter()
{
	bEncounterActive = false;
	bHasActiveOrder = false;
	ActiveOrder = FTacticalOrder{};
}

bool UTacticalOrderComponent::TryAcceptOrder(
	FTacticalOrder Order,
	FString& FailureReason)
{
	if (!ValidateOrder(Order, FailureReason))
	{
		return false;
	}

	if (!bEncounterActive)
	{
		FailureReason = TEXT("命令已过期：当前没有进行中的遭遇。");
		return false;
	}

	if (bHasActiveOrder)
	{
		if (Order.OrderId == ActiveOrder.OrderId)
		{
			FailureReason = TEXT("重复的 order_id。");
			return false;
		}

		if (Order.Priority <= ActiveOrder.Priority)
		{
			FailureReason = FString::Printf(
				TEXT("命令优先级 %d 未高于当前优先级 %d。"),
				Order.Priority,
				ActiveOrder.Priority);
			return false;
		}
	}

	ActiveOrder = MoveTemp(Order);
	bHasActiveOrder = true;

	FailureReason = FString::Printf(
		TEXT("命令已接受，当前优先级：%d。"),
		ActiveOrder.Priority);
	return true;
}

bool UTacticalOrderComponent::ValidateActiveOrderAtTrigger(
	bool bHasEnoughResource,
	bool bAbilityOffCooldown,
	float MaxExecutionDistance,
	FString& FailureReason) const
{
	if (!bEncounterActive)
	{
		FailureReason = TEXT("触发失败：当前没有进行中的遭遇。");
		return false;
	}

	if (!bHasActiveOrder)
	{
		FailureReason = TEXT("触发失败：当前没有活动命令。");
		return false;
	}

	if (!ValidateOrder(ActiveOrder, FailureReason))
	{
		return false;
	}

	if (ActiveOrder.Intent != ETacticalOrderIntent::ConditionalCast ||
		ActiveOrder.Then.Type != ETacticalThenType::CastAbility)
	{
		FailureReason = TEXT("触发失败：活动命令不是条件施法命令。");
		return false;
	}

	AActor* ResolvedTarget = nullptr;

	if (ActiveOrder.Then.Target.Type == ETacticalTargetType::Selector)
	{
		ResolvedTarget = ResolveTargetSelector(
			ActiveOrder.Then.Target.Selector);
	}
	else if (
		ActiveOrder.Then.Target.Type == ETacticalTargetType::Reference &&
		ActiveOrder.Then.Target.Reference == TEXT("when.subject"))
	{
		ResolvedTarget = ResolveTargetSelector(
			ActiveOrder.When.Subject);
	}

	if (!IsValid(ResolvedTarget))
	{
		FailureReason = TEXT("触发失败：目标已经失效。");
		return false;
	}

	const UAesirHealthComponent* TargetHealth =
		ResolvedTarget->FindComponentByClass<UAesirHealthComponent>();

	if (!IsValid(TargetHealth) || TargetHealth->IsDead())
	{
		FailureReason = TEXT("触发失败：目标不存在生命组件或已经死亡。");
		return false;
	}

	if (!bHasEnoughResource)
	{
		FailureReason = TEXT("触发失败：技能资源不足。");
		return false;
	}

	if (!bAbilityOffCooldown)
	{
		FailureReason = TEXT("触发失败：技能仍在冷却中。");
		return false;
	}

	if (MaxExecutionDistance <= 0.0f)
	{
		FailureReason = TEXT("触发失败：施法距离配置无效。");
		return false;
	}

	const float DistanceSquared = FVector::DistSquared(
		GetOwner()->GetActorLocation(),
		ResolvedTarget->GetActorLocation());

	if (DistanceSquared > FMath::Square(MaxExecutionDistance))
	{
		FailureReason = FString::Printf(
			TEXT("触发失败：目标距离过远，当前 %.0f，最大 %.0f。"),
			FMath::Sqrt(DistanceSquared),
			MaxExecutionDistance);
		return false;
	}

	const UAesirCombatStateComponent* CombatState =
		GetOwner()->FindComponentByClass<UAesirCombatStateComponent>();

	if (!IsValid(CombatState))
	{
		FailureReason = TEXT("触发失败：队友没有战斗状态组件。");
		return false;
	}

	const EAesirCombatState CurrentState =
		CombatState->GetCombatState();

	if (CurrentState != EAesirCombatState::Idle &&
		CurrentState != EAesirCombatState::Combat)
	{
		FailureReason = TEXT("触发失败：队友当前状态不允许施法。");
		return false;
	}

	FailureReason = TEXT("触发时二次校验通过。");
	return true;
}

bool UTacticalOrderComponent::CompleteActiveOrder(FString& CompletedOrderId, FString& FailureReason)
{
	CompletedOrderId.Reset();

	if (!bHasActiveOrder)
	{
		FailureReason = TEXT("完成失败：当前没有活动命令。");
		return false;
	}

	CompletedOrderId = ActiveOrder.OrderId;

	bHasActiveOrder = false;
	ActiveOrder = FTacticalOrder{};

	FailureReason = FString::Printf(
		TEXT("命令已完成：%s"),
		*CompletedOrderId);

	return true;
}

bool UTacticalOrderComponent::CancelActiveOrder(
	const FString& CancelReason,
	FString& CancelledOrderId,
	FString& ResultMessage)
{
	CancelledOrderId.Reset();

	if (!bHasActiveOrder)
	{
		ResultMessage = TEXT("取消失败：当前没有活动命令。");
		return false;
	}

	CancelledOrderId = ActiveOrder.OrderId;

	const FString EffectiveReason =
		CancelReason.TrimStartAndEnd().IsEmpty()
			? TEXT("未提供取消原因")
			: CancelReason.TrimStartAndEnd();

	bHasActiveOrder = false;
	ActiveOrder = FTacticalOrder{};

	ResultMessage = FString::Printf(
		TEXT("命令已取消：%s，原因：%s"),
		*CancelledOrderId,
		*EffectiveReason);

	return true;
}

AActor* UTacticalOrderComponent::ResolveTargetSelector(const FString& Selector) const
{
	if (!GetWorld())
		return nullptr;

	if (Selector == TEXT("party.player"))
		return UGameplayStatics::GetPlayerPawn(this, 0);

	if (Selector == TEXT("encounter.primary_hostile"))
	{
		TArray<AActor*> MatchingActors;

		UGameplayStatics::GetAllActorsWithTag(
			this,
			FName(TEXT("encounter.primary_hostile")),
			MatchingActors);

		for (AActor* Actor : MatchingActors)
			if (IsValid(Actor))
				return Actor;
	}
	
	return nullptr;
}
