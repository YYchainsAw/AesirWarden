#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AesirCompanionAIController.generated.h"

struct FCompanionAgentDirective;

UCLASS()
class AESIRWARDEN_API AAesirCompanionAIController : public AAIController
{
	GENERATED_BODY()
	
public:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aesir|Companion AI")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

private:
	UFUNCTION()
	void HandleAgentDirective(const FCompanionAgentDirective& Directive);

	bool bWaitingForPlayerCommand = false;
};
