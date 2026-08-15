// Copyright Epic Games, Inc. All Rights Reserved.


#include "CreatureAIController.h"
#include "Components/StateTreeAIComponent.h"

ACreatureAIController::ACreatureAIController()
{
	// create the StateTree AI Component
	StateTreeAI = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeAI"));
	check(StateTreeAI);

	// ensure we start the StateTree when we possess the pawn
	bStartAILogicOnPossess = false;
	StateTreeAI->SetStartLogicAutomatically(false);

	// ensure we're attached to the possessed character.
	// this is necessary for EnvQueries to work correctly
	bAttachToPawn = true;
}

void ACreatureAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// start AI logic
	StateTreeAI->StartLogic();
}
