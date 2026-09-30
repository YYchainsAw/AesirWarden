#pragma once

#include "CoreMinimal.h"
#include "HttpFwd.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Services/Companion/TacticalOrderTypes.h"
#include "Services/Companion/CombatContextTypes.h"
#include "CommandServiceSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnCommandServiceHealthChecked,
	bool, bSuccess,
	const FString&, Message);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
	FOnCommandParsed,
	bool, bSuccess,
	bool, bRecognized,
	const FString&, Message,
	FTacticalOrder, Order);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnTacticalResolved,
	bool, bSuccess,
	const FString&, ResponseJson);

UCLASS()
class AESIRWARDEN_API UCommandServiceSubsystem
	: public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Aesir|Command Service")
	void CheckHealth();
	
	UFUNCTION(BlueprintCallable, Category = "Aesir|Command Service")
	void ParseCommand(const FString& Text);

	UPROPERTY(BlueprintAssignable, Category = "Aesir|Command Service")
	FOnCommandServiceHealthChecked OnHealthChecked;
	
	UPROPERTY(BlueprintAssignable, Category = "Aesir|Command Service")
	FOnCommandParsed OnCommandParsed;	
	
	UFUNCTION(BlueprintCallable, Category = "Aesir|Command Service",
		meta = (DevelopmentOnly))
	void SendRawCommandForTest(const FString& JsonBody);
	
	UFUNCTION(BlueprintCallable, Category="Aesir|Command Service")
	void ParseVoiceCommand(const TArray<uint8>& WavData);
	
	UFUNCTION(BlueprintCallable, Category = "Aesir|Tactical Service")
	void ResolveTacticalIntentForTest(
		const FString& IntentId,
		const FString& TargetId,
		const FString& Timing,
		const FString& NormalizedText,
		const FCombatContext& CombatContext);

	UPROPERTY(BlueprintAssignable, Category = "Aesir|Tactical Service")
	FOnTacticalResolved OnTacticalResolved;

private:
	void HandleHealthResponse(
		FHttpRequestPtr Request,
		FHttpResponsePtr Response,
		bool bSucceeded);
	
	void HandleParseCommandResponse(
		FHttpRequestPtr Request,
		FHttpResponsePtr Response,
		bool bSucceeded,
		FString ExpectedRequestId);

	FString ServiceBaseUrl = TEXT("http://127.0.0.1:8011");
	
	void HandleResolveTacticalResponse(
		FHttpRequestPtr Request,
		FHttpResponsePtr Response,
		bool bSucceeded,
		FString ExpectedRequestId);
};