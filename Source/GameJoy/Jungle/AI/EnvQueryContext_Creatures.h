// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "EnvQueryContext_Creatures.generated.h"

/**
 *  UEnvQueryContext_Creatures
 *  EnvQuery Context that returns every entity implementing ICreatureSized
 *  (animals and the player) except the querier itself. Used as the candidate
 *  set for cone + line-of-sight perception queries.
 */
UCLASS()
class UEnvQueryContext_Creatures : public UEnvQueryContext
{
	GENERATED_BODY()

public:

	/** Provides the context actors for this EnvQuery */
	virtual void ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const override;
};
