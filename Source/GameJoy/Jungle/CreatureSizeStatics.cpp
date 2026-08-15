// Copyright Epic Games, Inc. All Rights Reserved.


#include "CreatureSizeStatics.h"

ECreatureRelation UCreatureSizeStatics::ClassifyBySize(float MySize, float OtherSize, float Threshold)
{
	// guard against invalid data; treat as evenly matched
	if (MySize <= 0.0f || OtherSize <= 0.0f || Threshold < 1.0f)
	{
		return ECreatureRelation::Peer;
	}

	// clearly bigger than the other -> the other is prey
	if (MySize >= OtherSize * Threshold)
	{
		return ECreatureRelation::Prey;
	}

	// clearly smaller than the other -> the other is a predator
	if (OtherSize >= MySize * Threshold)
	{
		return ECreatureRelation::Predator;
	}

	// grey zone -> evenly matched, neither can prey on the other
	return ECreatureRelation::Peer;
}
