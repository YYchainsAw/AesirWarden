#if WITH_DEV_AUTOMATION_TESTS

#include "AI/Companion/AesirCompanionAIController.h"
#include "Characters/Companion/AesirCompanionCharacter.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Services/Companion/CompanionChatSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompanionExplorationExecutionTest,
	"Aesir.Companion.ExplorationExecution",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompanionExplorationExecutionTest::RunTest(const FString&)
{
	// InitializeStandalone creates a transient game World and its context, with no map/assets or navigation.
	UGameInstance* Instance = NewObject<UGameInstance>(GEngine);
	Instance->AddToRoot();
	Instance->InitializeStandalone(FName(*FString::Printf(TEXT("CompanionTest_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))));
	UWorld* World = Instance->GetWorld();
	if (!TestNotNull(TEXT("Transient game world"), World))
	{
		Instance->Shutdown();
		Instance->RemoveFromRoot();
		return false;
	}
	World->CreateAISystem();
	World->InitializeActorsForPlay(FURL());
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	APlayerController* PlayerController = World->SpawnActor<APlayerController>(Spawn);
	ACharacter* PlayerPawn = World->SpawnActor<ACharacter>(FVector(1000.0f, 0.0f, 100.0f), FRotator::ZeroRotator, Spawn);
	AAesirCompanionCharacter* Companion = World->SpawnActor<AAesirCompanionCharacter>(FVector(0.0f, 0.0f, 100.0f), FRotator::ZeroRotator, Spawn);
	AAesirCompanionAIController* Controller = World->SpawnActor<AAesirCompanionAIController>(Spawn);
	UCompanionChatSubsystem* Chat = Instance->GetSubsystem<UCompanionChatSubsystem>();
	const bool bSpawned = TestNotNull(TEXT("Player controller"), PlayerController) &&
		TestNotNull(TEXT("Player pawn"), PlayerPawn) && TestNotNull(TEXT("Companion pawn"), Companion) &&
		TestNotNull(TEXT("Companion controller"), Controller) && TestNotNull(TEXT("Chat subsystem"), Chat);
	if (bSpawned)
	{
		PlayerController->Possess(PlayerPawn);
		Controller->Possess(Companion);
		Chat->SelectedCompanionId = Companion->GetCompanionId().ToString();
		Chat->Binding.RunId = TEXT("33333333-3333-4333-8333-333333333333");
		Chat->ContextGeneration = 7;
		Chat->bBindingReady = true;
		Chat->bContextActive = true;
		Chat->SessionWorld = World;
		Chat->SessionPlayer = PlayerPawn;
		Chat->SessionCompanion = Companion;

		FCompanionAgentDirective Directive;
		Directive.RequestId = TEXT("11111111-1111-4111-8111-111111111111");
		Directive.DirectiveId = TEXT("bbbbbbbb-bbbb-4bbb-8bbb-bbbbbbbbbbbb");
		Directive.RunId = Chat->Binding.RunId;
		Directive.ContextGeneration = Chat->ContextGeneration;
		Directive.ValidUntilSeconds = FPlatformTime::Seconds() + 30.0;
		Directive.AgentId = Companion->GetCompanionId().ToString();
		Directive.Domain = TEXT("exploration");
		Directive.Source = TEXT("player_command");
		Directive.ActionType = TEXT("wait");
		TestEqual(TEXT("Fixture player is registered in the game world"), UGameplayStatics::GetPlayerPawn(World, 0), static_cast<APawn*>(PlayerPawn));
		TestTrue(TEXT("AI possesses the companion"), Controller->GetPawn() == Companion);
		TestTrue(TEXT("Synthetic binding passes the production scope/freshness gate"), Chat->IsDirectiveCurrent(Directive));
		TestTrue(TEXT("Fixture is an exploration sandbox"), Controller->IsExplorationAllowed());
		Companion->GetCharacterMovement()->Velocity = FVector(300.0f, 0.0f, 0.0f);
		Chat->OnDirectiveReceived.Broadcast(Directive);
		TestTrue(TEXT("A valid wait actually stops character velocity"), Companion->GetVelocity().IsNearlyZero());
		TestTrue(TEXT("Wait becomes the active command"), Controller->IsWaitingForPlayerCommand());
		TestEqual(TEXT("Local action retains the service directive ID"), Controller->GetActiveExplorationDirectiveId(), Directive.DirectiveId);

		// If duplicate processing ran StopMovementImmediately again, this externally set velocity would be cleared.
		Companion->GetCharacterMovement()->Velocity = FVector(10.0f, 0.0f, 0.0f);
		Chat->OnDirectiveReceived.Broadcast(Directive);
		TestEqual(TEXT("A duplicate directive has no second physical side effect"), Companion->GetVelocity(), FVector(10.0f, 0.0f, 0.0f));

		FCompanionAgentDirective Old = Directive;
		Old.DirectiveId = TEXT("cccccccc-cccc-4ccc-8ccc-cccccccccccc");
		Old.ContextGeneration--;
		Chat->OnDirectiveReceived.Broadcast(Old);
		TestEqual(TEXT("Old generation cannot stop movement or replace the active command"), Companion->GetVelocity(), FVector(10.0f, 0.0f, 0.0f));
		TestEqual(TEXT("Old generation leaves current directive identity intact"), Controller->GetActiveExplorationDirectiveId(), Directive.DirectiveId);

		FCompanionAgentDirective Expired = Old;
		Expired.ContextGeneration = Chat->ContextGeneration;
		Expired.ValidUntilSeconds = FPlatformTime::Seconds() - 0.01;
		Chat->OnDirectiveReceived.Broadcast(Expired);
		TestEqual(TEXT("Expired response cannot mutate local movement"), Companion->GetVelocity(), FVector(10.0f, 0.0f, 0.0f));

		FCompanionAgentDirective Follow = Directive;
		Follow.DirectiveId = TEXT("dddddddd-dddd-4ddd-8ddd-dddddddddddd");
		Follow.ActionType = TEXT("follow");
		Follow.TargetId = TEXT("party.player");
		TestNull(TEXT("This fixture intentionally has no navigation system"), World->GetNavigationSystem());
		AddExpectedMessage(TEXT("Companion follow failed:"), ELogVerbosity::Warning);
		Chat->OnDirectiveReceived.Broadcast(Follow);
		TestFalse(TEXT("A failed navigation request is not reported as following"), Controller->IsFollowingPlayerCommand());
		TestEqual(TEXT("Failed follow does not claim its directive was applied"), Controller->GetActiveExplorationDirectiveId(), Directive.DirectiveId);

		++Chat->ContextGeneration;
		Chat->OnConversationReset.Broadcast();
		TestFalse(TEXT("Context reset clears wait"), Controller->IsWaitingForPlayerCommand());
		TestTrue(TEXT("Context reset clears directive identity"), Controller->GetActiveExplorationDirectiveId().IsEmpty());
		Chat->OnDirectiveReceived.Broadcast(Directive);
		TestFalse(TEXT("A late old-generation callback cannot revive wait"), Controller->IsWaitingForPlayerCommand());

		// Prevent shutdown from making HTTP calls for this synthetic binding.
		Chat->bBindingReady = false;
		Chat->bContextActive = false;
		Chat->Binding = CompanionScoped::FBinding{};
	}
	Instance->Shutdown();
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	Instance->RemoveFromRoot();
	return bSpawned;
}

#endif
