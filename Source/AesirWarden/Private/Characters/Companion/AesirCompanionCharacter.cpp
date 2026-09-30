#include "Characters/Companion/AesirCompanionCharacter.h"

#include "Components/PrimitiveComponent.h"

AAesirCompanionCharacter::AAesirCompanionCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

}

void AAesirCompanionCharacter::BeginPlay()
{
	Super::BeginPlay();

	TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents;
	GetComponents(PrimitiveComponents);
	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (IsValid(PrimitiveComponent))
		{
			PrimitiveComponent->SetCollisionResponseToChannel(
				ECC_Camera,
				ECR_Ignore);
		}
	}
	
}

void AAesirCompanionCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

