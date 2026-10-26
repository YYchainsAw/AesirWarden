#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Framework/Flow/AesirLevelFlowTypes.h"
#include "TimerManager.h"
#include "AesirCompanionAIController.generated.h"

struct FCompanionAgentDirective;
class AAesirLevelFlowDirector;
class UAesirHealthComponent;

UCLASS()
class AESIRWARDEN_API AAesirCompanionAIController : public AAIController
{
	GENERATED_BODY()
	
public:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintPure, Category = "Aesir|Companion AI")
	bool IsWaitingForLevelStart() const { return bWaitingForLevelStart; }

	UFUNCTION(BlueprintPure, Category = "Aesir|Companion AI")
	bool IsWaitingForPlayerCommand() const { return bWaitingForPlayerCommand; }

	UFUNCTION(BlueprintPure, Category = "Aesir|Companion AI")
	bool IsFollowingPlayerCommand() const { return bFollowingPlayerCommand; }

	UFUNCTION(BlueprintPure, Category = "Aesir|Companion AI")
	FString GetActiveExplorationDirectiveId() const { return ActiveExplorationDirectiveId; }

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aesir|Companion AI")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

private:
	friend class FCompanionExplorationExecutionTest;

	UFUNCTION()
	void HandleAgentDirective(const FCompanionAgentDirective& Directive);

	UFUNCTION()
	void HandleConversationReset();

	UFUNCTION()
	void HandleFlowStateChanged(EAesirLevelFlowState PreviousState, EAesirLevelFlowState NewState);

	UFUNCTION()
	void HandleCompanionDeath(AController* InstigatedBy, AActor* DamageCauser);

	bool IsExplorationAllowed() const;
	void UpdateExplorationCommand();
	void ResetExplorationCommand(bool bResumeDefaultBehavior);
	void ReleaseBindings();
	void PauseDefaultBehavior();
	void StartDefaultBehavior();

	bool bWaitingForPlayerCommand = false;
	bool bWaitingForLevelStart = false;
	bool bFollowingPlayerCommand = false;
	bool bOwnsExplorationBrainPause = false;
	FString ActiveExplorationDirectiveId;
	TArray<FString> AppliedDirectiveIds;
	TWeakObjectPtr<APawn> CommandPlayer;
	TWeakObjectPtr<AAesirLevelFlowDirector> LevelFlowDirector;
	TWeakObjectPtr<UAesirHealthComponent> CompanionHealth;
	FTimerHandle ExplorationCommandTimer;
};
