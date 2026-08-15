// Copyright Epic Games, Inc. All Rights Reserved.


#include "Creature.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Curves/CurveFloat.h"
#include "AI/CreatureAIController.h"

ACreature::ACreature()
{
	PrimaryActorTick.bCanEverTick = true;

	// set the AI Controller class by default
	AIControllerClass = ACreatureAIController::StaticClass();

	// use an AI Controller regardless of whether we're placed or spawned
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// ignore the controller's yaw rotation; the movement component drives rotation
	bUseControllerRotationYaw = false;

	// rotate towards the controller's desired rotation (set via AI focus)
	GetCharacterMovement()->bUseControllerDesiredRotation = true;
}

void ACreature::SetCreatureSize(float NewSize)
{
	// keep Size strictly positive
	Size = FMath::Max(0.01f, NewSize);

	// bigger creatures move slower; apply the new speed immediately
	ApplySizeToMovement();
}

void ACreature::ApplySizeToMovement()
{
	// derive MaxWalkSpeed from Size via the configured curve
	if (SizeToSpeedCurve)
	{
		GetCharacterMovement()->MaxWalkSpeed = SizeToSpeedCurve->GetFloatValue(Size);
	}
}

void ACreature::BeginPlay()
{
	Super::BeginPlay();

	// initialize movement speed from the starting Size
	ApplySizeToMovement();
}

#if WITH_EDITOR
void ACreature::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// keep speed in sync as Size / the curve are tweaked in the editor
	ApplySizeToMovement();
}
#endif
