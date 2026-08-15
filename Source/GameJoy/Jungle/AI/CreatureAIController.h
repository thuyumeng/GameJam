// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "CreatureAIController.generated.h"

class UStateTreeAIComponent;

/**
 *	AI Controller for creatures. Runs the shared creature StateTree brain.
 *	Mirrors ACombatAIController.
 */
UCLASS(abstract)
class ACreatureAIController : public AAIController
{
	GENERATED_BODY()

	/** StateTree Component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStateTreeAIComponent* StateTreeAI;

public:

	/** Constructor */
	ACreatureAIController();

protected:

	/** Pawn Initialization */
	virtual void OnPossess(APawn* InPawn) override;
};
