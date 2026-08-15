// Copyright Epic Games, Inc. All Rights Reserved.


#include "EnvQueryContext_Creatures.h"
#include "CreatureSized.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"
#include "Kismet/GameplayStatics.h"

void UEnvQueryContext_Creatures::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	// resolve the world from the querying object
	UObject* Owner = QueryInstance.Owner.Get();
	if (!Owner)
	{
		return;
	}

	AActor* OwnerActor = Cast<AActor>(Owner);
	UWorld* World = OwnerActor ? OwnerActor->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}

	// gather every entity that participates in the food chain
	TArray<AActor*> Creatures;
	UGameplayStatics::GetAllActorsWithInterface(World, UCreatureSized::StaticClass(), Creatures);

	// never treat the querier as its own candidate
	if (OwnerActor)
	{
		Creatures.Remove(OwnerActor);
	}

	// add the candidate actors to the context
	UEnvQueryItemType_Actor::SetContextHelper(ContextData, Creatures);
}
