// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CreatureSizeStatics.generated.h"

/**
 *  How one entity relates to another purely by size (B-rule).
 *  Evaluated from the perceiver's point of view looking at the other entity.
 */
UENUM(BlueprintType)
enum class ECreatureRelation : uint8
{
	/** The other entity is significantly smaller and can be preyed upon */
	Prey,

	/** The other entity is close in size; neither can prey on the other (grey zone) */
	Peer,

	/** The other entity is significantly larger and will prey on us */
	Predator
};

/**
 *  Pure, stateless helpers for the size-based food chain.
 */
UCLASS()
class UCreatureSizeStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/**
	 *  Classifies how an entity of MySize relates to an entity of OtherSize under the B-rule.
	 *  - MySize >= OtherSize * Threshold  -> Prey     (we are clearly bigger)
	 *  - OtherSize >= MySize * Threshold  -> Predator (we are clearly smaller)
	 *  - otherwise                        -> Peer     (grey zone, evenly matched)
	 *
	 *  @param MySize     Size of the perceiving entity (> 0)
	 *  @param OtherSize  Size of the observed entity (> 0)
	 *  @param Threshold  Predation size-ratio threshold T (>= 1, default 1.2)
	 */
	UFUNCTION(BlueprintPure, Category="Creature")
	static ECreatureRelation ClassifyBySize(float MySize, float OtherSize, float Threshold = 1.2f);
};
