#if WITH_DEV_AUTOMATION_TESTS

#include "AesirMatchTestGameMode.h"
#include "Abilities/AesirAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Characters/Enemy/AesirEnemyCharacter.h"
#include "Combat/AesirHealthComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Misc/AutomationTest.h"

namespace AesirMatchTests
{
struct FMatchWorld
{
	UGameInstance* Instance = nullptr;
	UWorld* World = nullptr;
	AAesirMatchTestGameMode* Mode = nullptr;
	APawn* Player = nullptr;
	UAesirHealthComponent* PlayerHealth = nullptr;
	AAesirEnemyCharacter* Boss = nullptr;
	int32 Results = 0;
	EAesirMatchResult EventResult = EAesirMatchResult::None;

	FMatchWorld()
	{
		Instance = NewObject<UGameInstance>(GEngine);
		Instance->AddToRoot();
		Instance->InitializeStandalone(MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass(), TEXT("MatchTest")));
		World = Instance->GetWorld();
		if (!World) return;
		World->InitializeActorsForPlay(FURL());
		Mode = World->SpawnActor<AAesirMatchTestGameMode>();
		Player = World->SpawnActor<APawn>();
		if (!Mode || !Player) return;
		PlayerHealth = NewObject<UAesirHealthComponent>(Player);
		Player->AddInstanceComponent(PlayerHealth);
		PlayerHealth->RegisterComponent();
		PlayerHealth->SynchronizeHealthFromGAS(100.0f, 100.0f);
		Boss = SpawnEnemy();
		Mode->OnMatchFinished.AddLambda([this](EAesirMatchResult Result)
		{
			++Results;
			EventResult = Result;
		});
	}

	~FMatchWorld()
	{
		if (World)
		{
			for (AActor* Actor : FActorRange(World))
			{
				Actor->RouteEndPlay(EEndPlayReason::LevelTransition);
			}
		}
		Instance->Shutdown();
		if (World)
		{
			World->DestroyWorld(false);
			GEngine->DestroyWorldContext(World);
		}
		Instance->RemoveFromRoot();
	}

	AAesirEnemyCharacter* SpawnEnemy() const
	{
		FActorSpawnParameters Spawn;
		Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Spawn.bDeferConstruction = true;
		AAesirEnemyCharacter* Enemy = World->SpawnActor<AAesirEnemyCharacter>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
		if (Enemy)
		{
			Enemy->AutoPossessAI = EAutoPossessAI::Disabled;
			Enemy->FinishSpawning(Enemy->GetActorTransform());
			Enemy->DispatchBeginPlay();
		}
		return Enemy;
	}

	static void KillEnemy(AAesirEnemyCharacter* Enemy)
	{
		Enemy->GetAbilitySystemComponent()->SetNumericAttributeBase(UAesirAttributeSet::GetHealthAttribute(), 0.0f);
	}

	void KillPlayer() const
	{
		PlayerHealth->SynchronizeHealthFromGAS(0.0f, 100.0f);
	}
};

class FVerifyResult final : public IAutomationLatentCommand
{
public:
	FVerifyResult(FAutomationTestBase* InTest, TSharedRef<FMatchWorld> InFixture, EAesirMatchResult InExpected)
		: Test(InTest), Fixture(MoveTemp(InFixture)), Expected(InExpected), DeathFrame(GFrameCounter)
	{
	}

	bool Update() override
	{
		Fixture->World->GetTimerManager().Tick(0.1f);
		if (GFrameCounter <= DeathFrame) return false;
		if (Fixture->Results == 0 && Expected != EAesirMatchResult::None && GFrameCounter <= DeathFrame + 3)
		{
			return false;
		}
		Test->TestEqual(TEXT("Correct match result after the death frame"), Fixture->Mode->GetMatchResult(), Expected);
		Test->TestEqual(TEXT("Match completion publishes at most once"), Fixture->Results, Expected == EAesirMatchResult::None ? 0 : 1);
		Test->TestEqual(TEXT("The event publishes the same result"), Fixture->EventResult, Expected);
		if (Expected != EAesirMatchResult::None)
		{
			Fixture->KillPlayer();
			FMatchWorld::KillEnemy(Fixture->Boss);
			Fixture->PlayerHealth->OnDeath.Broadcast(nullptr, nullptr);
			Test->TestFalse(TEXT("Terminal match cannot register another boss"), Fixture->Mode->RegisterMatchBoss(Fixture->Boss));
			Test->TestEqual(TEXT("Late death leaves the result unchanged"), Fixture->Mode->GetMatchResult(), Expected);
			Test->TestEqual(TEXT("Late and duplicate callbacks leave a single finish event"), Fixture->Results, 1);
		}
		return true;
	}

private:
	FAutomationTestBase* Test;
	TSharedRef<FMatchWorld> Fixture;
	EAesirMatchResult Expected;
	uint64 DeathFrame;
};
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FAesirMatchResultRulesTest, "Aesir.Match.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

void FAesirMatchResultRulesTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	for (const TCHAR* Case : { TEXT("ObjectiveVictory"), TEXT("OrdinaryEnemyIgnored"), TEXT("PlayerDefeat"),
		TEXT("SimultaneousBossFirst"), TEXT("SimultaneousPlayerFirst"), TEXT("ReplaceDynamicObjective") })
	{
		Names.Add(Case);
		Commands.Add(Case);
	}
}

bool FAesirMatchResultRulesTest::RunTest(const FString& Parameters)
{
	using namespace AesirMatchTests;
	const auto Fixture = MakeShared<FMatchWorld>();
	if (!TestTrue(TEXT("World, player health and objective are ready"),
		Fixture->Mode && Fixture->PlayerHealth && Fixture->Boss)) return false;
	TestTrue(TEXT("Player can be registered after spawning"), Fixture->Mode->RegisterMatchPlayer(Fixture->Player));
	TestFalse(TEXT("An ordinary pawn cannot be registered as the Boss"), Fixture->Mode->RegisterMatchBoss(Fixture->Player));
	if (!TestTrue(TEXT("The spawned objective can be registered"), Fixture->Mode->RegisterMatchBoss(Fixture->Boss))) return false;
	TestTrue(TEXT("Registration of the same objective is idempotent"), Fixture->Mode->RegisterMatchBoss(Fixture->Boss));

	EAesirMatchResult Expected = EAesirMatchResult::Victory;
	auto* OrdinaryEnemy = Fixture->SpawnEnemy();
	if (!TestNotNull(TEXT("A second enemy is present"), OrdinaryEnemy)) return false;
	if (Parameters == TEXT("OrdinaryEnemyIgnored"))
	{
		FMatchWorld::KillEnemy(OrdinaryEnemy);
		Expected = EAesirMatchResult::None;
	}
	else if (Parameters == TEXT("ReplaceDynamicObjective"))
	{
		TestFalse(TEXT("A live objective cannot be silently replaced"), Fixture->Mode->RegisterMatchBoss(OrdinaryEnemy));
		TestTrue(TEXT("The old objective can be explicitly unregistered"), Fixture->Mode->UnregisterMatchBoss(Fixture->Boss));
		TestTrue(TEXT("A newly spawned objective can be registered"), Fixture->Mode->RegisterMatchBoss(OrdinaryEnemy));
		FMatchWorld::KillEnemy(Fixture->Boss);
		TestTrue(TEXT("Unregistered Boss death cannot block a subsequent registration"), Fixture->Mode->UnregisterMatchBoss(OrdinaryEnemy));
		TestTrue(TEXT("The new objective can be registered again"), Fixture->Mode->RegisterMatchBoss(OrdinaryEnemy));
		FMatchWorld::KillEnemy(OrdinaryEnemy);
	}
	else if (Parameters == TEXT("PlayerDefeat"))
	{
		Fixture->KillPlayer();
		Expected = EAesirMatchResult::Defeat;
	}
	else if (Parameters == TEXT("SimultaneousBossFirst"))
	{
		FMatchWorld::KillEnemy(Fixture->Boss);
		Fixture->KillPlayer();
		Expected = EAesirMatchResult::Defeat;
	}
	else if (Parameters == TEXT("SimultaneousPlayerFirst"))
	{
		Fixture->KillPlayer();
		FMatchWorld::KillEnemy(Fixture->Boss);
		Expected = EAesirMatchResult::Defeat;
	}
	else
	{
		FMatchWorld::KillEnemy(Fixture->Boss);
		TestFalse(TEXT("The other enemy remains alive"), OrdinaryEnemy->IsDead());
		Fixture->Boss->FindComponentByClass<UAesirHealthComponent>()->OnDeath.Broadcast(nullptr, nullptr);
	}
	TestEqual(TEXT("Death callbacks do not publish a result during their frame"), Fixture->Mode->GetMatchResult(), EAesirMatchResult::None);
	ADD_LATENT_AUTOMATION_COMMAND(FVerifyResult(this, Fixture, Expected));
	return true;
}

#endif
