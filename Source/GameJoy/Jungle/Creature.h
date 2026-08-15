// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CreatureSized.h"
#include "Creature.generated.h"

class UCurveFloat;
class UCreaturePerceptionData;

/**
 *  An AI-controlled creature driven by the shared creature StateTree brain.
 *  Holds a single Size scalar (its currency in the food chain) and derives its
 *  movement speed from Size. All animal species derive from this base; behaviour
 *  differences come purely from data (Size etc.), not from per-species logic.
 *  Mirrors ACombatEnemy.
 */
UCLASS(abstract)
class ACreature : public ACharacter, public ICreatureSized
{
	GENERATED_BODY()

public:

	/** Constructor */
	ACreature();

protected:

	/** This creature's Size scalar. Its sole measure in the food chain (must be > 0). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Creature", meta=(ClampMin="0.01"))
	float Size = 1.0f;

	/** Monotonically decreasing Size -> MaxWalkSpeed curve. Bigger = slower. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Creature")
	TObjectPtr<UCurveFloat> SizeToSpeedCurve;

	/** Shared perception + classification parameters (threshold, sight cone, range). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Creature")
	TObjectPtr<UCreaturePerceptionData> PerceptionData;

public:

	/** Returns the perception/classification data asset configured on this creature. */
	UCreaturePerceptionData* GetPerceptionData() const { return PerceptionData; }

	/** Sets a new Size and immediately updates movement speed (no respawn needed). */
	UFUNCTION(BlueprintCallable, Category="Creature")
	void SetCreatureSize(float NewSize);

	// ~begin ICreatureSized interface

	/** Returns this creature's Size scalar */
	virtual float GetCreatureSize() const override { return Size; }

	// ~end ICreatureSized interface

protected:

	/** Reads the Size -> speed curve and applies the result to CharacterMovement.MaxWalkSpeed. */
	void ApplySizeToMovement();

	/** Gameplay initialization */
	virtual void BeginPlay() override;

#if WITH_EDITOR
	/** Keep movement speed in sync when Size or the curve is edited in the editor. */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
