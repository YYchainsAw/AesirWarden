#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Framework/Flow/AesirLevelFlowTypes.h"
#include "AesirCompanionAIController.generated.h"

struct FCompanionAgentDirective;
class AAesirLevelFlowDirector;

UCLASS()
class AESIRWARDEN_API AAesirCompanionAIController : public AAIController
{
	GENERATED_BODY()
	
public:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aesir|Companion AI")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

private:
	void ClearLevelStartBinding();
	void UnsubscribeFromChat();

	UFUNCTION()
	void HandleLevelFlowStateChanged(EAesirLevelFlowState PreviousState, EAesirLevelFlowState NewState);

	UFUNCTION()
	void HandleAgentDirective(const FCompanionAgentDirective& Directive);

	bool bWaitingForPlayerCommand = false;
	bool bWaitingForLevelStart = false;
	TWeakObjectPtr<AAesirLevelFlowDirector> StartupFlowDirector;
};
