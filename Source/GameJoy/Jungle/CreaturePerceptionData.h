// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CreaturePerceptionData.generated.h"

/**
 *  Single, shareable data source for the food-chain classification threshold and
 *  the perception parameters (sight cone, range, personal space). Keeping these in
 *  one DataAsset avoids hard-coding the threshold T in multiple places.
 */
UCLASS(BlueprintType)
class UCreaturePerceptionData : public UDataAsset
{
	GENERATED_BODY()

public:

	/** Predation size-ratio threshold T. Must be >= 1. Default 1.2 (B-rule). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Classification", meta=(ClampMin="1.0"))
	float PredationThreshold = 1.2f;

	/** Half-angle of the forward sight cone, in degrees (total horizontal FOV = 2x this). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Perception", meta=(ClampMin="0.0", ClampMax="180.0", Units="degrees"))
	float SightConeHalfAngle = 60.0f;

	/** Maximum sight distance, in cm. Targets beyond this are not perceived. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Perception", meta=(ClampMin="0.0", Units="cm"))
	float SightRange = 2500.0f;

	/** Personal-space radius, in cm. A peer breaching this during a standoff triggers flight. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Perception", meta=(ClampMin="0.0", Units="cm"))
	float PersonalSpaceRadius = 200.0f;
};
