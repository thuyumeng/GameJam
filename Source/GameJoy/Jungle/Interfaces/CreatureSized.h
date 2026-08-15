// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CreatureSized.generated.h"

/**
 *  CreatureSized interface
 *  Exposes an entity's Size scalar so any actor (animal or player) can
 *  participate in the size-based food chain and be classified by others.
 *  Mirrors the ICombatDamageable interface pattern.
 */
UINTERFACE(MinimalAPI, NotBlueprintable)
class UCreatureSized : public UInterface
{
	GENERATED_BODY()
};

class ICreatureSized
{
	GENERATED_BODY()

public:

	/** Returns this entity's Size scalar (must be > 0), used for food-chain comparisons */
	UFUNCTION(BlueprintCallable, Category="Creature")
	virtual float GetCreatureSize() const = 0;
};
