#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AesirCompanionCharacter.generated.h"

UCLASS()
class AESIRWARDEN_API AAesirCompanionCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AAesirCompanionCharacter();

	UFUNCTION(BlueprintPure, Category = "Aesir|Companion")
	FName GetCompanionId() const
	{
		return CompanionId;
	}
protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
	Category = "Aesir|Companion",
	meta = (AllowPrivateAccess = "true"))
	FName CompanionId = TEXT("companion.alice");
public:	
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
