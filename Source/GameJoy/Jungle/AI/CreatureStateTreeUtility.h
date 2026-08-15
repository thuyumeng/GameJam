// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "StateTreeConditionBase.h"

#include "CreatureStateTreeUtility.generated.h"

class ACreature;
class AAIController;
class UCreaturePerceptionData;

////////////////////////////////////////////////////////////////////

/**
 *  Instance data for the Creature Perception task.
 *  Inputs are the perceiving creature (+ optional parameter override).
 *  Outputs are the classified targets the brain reads to drive transitions.
 */
USTRUCT()
struct FStateTreeCreaturePerceptionInstanceData
{
	GENERATED_BODY()

	/** The perceiving creature */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<ACreature> Creature;

	/**
	 *  Optional perception/classification parameters. If left empty, the data asset
	 *  configured on the Creature is used; if that is also empty, defaults apply.
	 */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TObjectPtr<UCreaturePerceptionData> PerceptionDataOverride;

	/**
	 *  Targets closer than this are sensed even outside the sight cone (contact
	 *  awareness), so something right beside/behind us at melee range isn't lost.
	 */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = 0, Units = "cm"))
	float CloseRangeRadius = 250.0f;

	/**
	 *  Perception memory: how long a prey/predator stays "perceived" after we lose
	 *  sight of it, so chasing/fleeing doesn't give up the instant sight breaks.
	 */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = 0, Units = "s"))
	float MemoryDuration = 3.0f;

	/**
	 *  Draws the sight cone, close-range ring, and a colored line to every
	 *  candidate each tick (green=seen, orange=out of cone, red=blocked by LOS).
	 *  Turn on to debug why a target is or isn't being perceived.
	 */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bDrawDebug = false;

	// --- Outputs ---

	/** Nearest perceived entity classified as prey (this creature is clearly bigger) */
	UPROPERTY(VisibleAnywhere, Category = "Output")
	TObjectPtr<AActor> PreyTarget;

	/** Nearest perceived entity classified as a predator (this creature is clearly smaller) */
	UPROPERTY(VisibleAnywhere, Category = "Output")
	TObjectPtr<AActor> PredatorTarget;

	/** Nearest perceived entity classified as an evenly-matched peer (grey zone) */
	UPROPERTY(VisibleAnywhere, Category = "Output")
	TObjectPtr<AActor> PeerTarget;

	/** True if a prey entity is currently perceived */
	UPROPERTY(VisibleAnywhere, Category = "Output")
	bool bHasPrey = false;

	/** True if a predator entity is currently perceived */
	UPROPERTY(VisibleAnywhere, Category = "Output")
	bool bHasPredator = false;

	/** True if an evenly-matched peer is currently perceived */
	UPROPERTY(VisibleAnywhere, Category = "Output")
	bool bHasPeer = false;

	/** Last known location of the prey target. Persists after line of sight is lost. */
	UPROPERTY(VisibleAnywhere, Category = "Output")
	FVector LastKnownPreyLocation = FVector::ZeroVector;

	/** Last known location of the predator target. Persists after line of sight is lost. */
	UPROPERTY(VisibleAnywhere, Category = "Output")
	FVector LastKnownPredatorLocation = FVector::ZeroVector;

	// --- runtime state (perception memory timers) ---

	/** Seconds since the prey was last directly seen; drives the memory window */
	UPROPERTY()
	float TimeSincePreySeen = 0.0f;

	/** Seconds since the predator was last directly seen; drives the memory window */
	UPROPERTY()
	float TimeSincePredatorSeen = 0.0f;
};

/**
 *  StateTree task that continuously perceives nearby creatures through a forward
 *  sight cone + line-of-sight trace, classifies each by size (B-rule), and exposes
 *  the current prey / predator / peer targets for the brain to act on.
 */
USTRUCT(meta=(DisplayName="Creature Perception", Category="Creature"))
struct FStateTreeCreaturePerceptionTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	/** Constructor */
	FStateTreeCreaturePerceptionTask()
	{
		// perception runs every tick while the owning state is active
		bShouldCallTick = true;

		// keep running across reselection without restarting
		bShouldStateChangeOnReselect = false;
	}

	/* Ensure we're using the correct instance data struct */
	using FInstanceDataType = FStateTreeCreaturePerceptionInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	/** Runs an initial perception pass when the owning state is entered */
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

	/** Refreshes perception each tick */
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif // WITH_EDITOR
};

////////////////////////////////////////////////////////////////////

/**
 *  Instance data for the Within Distance condition.
 */
USTRUCT()
struct FStateTreeWithinDistanceConditionInstanceData
{
	GENERATED_BODY()

	/** Source actor */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> Source;

	/** Target actor to measure the distance to */
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<AActor> Target;

	/** Distance threshold, in cm (e.g. personal space or contact radius) */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = 0, Units = "cm"))
	float Distance = 200.0f;

	/** If true, the condition passes when the target is beyond the distance instead */
	UPROPERTY(EditAnywhere, Category = "Condition")
	bool bInvert = false;
};

/**
 *  StateTree condition that passes when Target is within Distance of Source.
 *  Used for contact/predation checks and standoff personal-space breaches.
 */
USTRUCT(DisplayName = "Actor Within Distance")
struct FStateTreeWithinDistanceCondition : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	/** Set the instance data type */
	using FInstanceDataType = FStateTreeWithinDistanceConditionInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	/** Default constructor */
	FStateTreeWithinDistanceCondition() = default;

	/** Tests the StateTree condition */
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

////////////////////////////////////////////////////////////////////

/**
 *  Instance data for the Wander task.
 */
USTRUCT()
struct FStateTreeWanderInstanceData
{
	GENERATED_BODY()

	/** AI Controller driving the wandering pawn */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> Controller;

	/** Radius around the current location to pick the next wander point */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = 0, Units = "cm"))
	float WanderRadius = 1500.0f;

	/** How close to a wander point counts as "arrived" */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = 0, Units = "cm"))
	float AcceptanceRadius = 100.0f;

	/** Minimum idle time spent pausing at each reached point */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = 0, Units = "s"))
	float MinPauseTime = 1.0f;

	/** Maximum idle time spent pausing at each reached point */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = 0, Units = "s"))
	float MaxPauseTime = 3.0f;

	// --- runtime state ---

	/** True while idling at a reached point; false while moving toward one */
	UPROPERTY()
	bool bIsPausing = false;

	/** Remaining pause time, counted down while idling */
	UPROPERTY()
	float PauseTimeRemaining = 0.0f;
};

/**
 *  StateTree task that makes an AI pawn wander: it walks to a random reachable
 *  point, pauses for a randomized rest, then picks the next point and repeats.
 *  Stays Running forever; the brain leaves WANDER via perception-driven transitions.
 */
USTRUCT(meta=(DisplayName="Wander", Category="Creature"))
struct FStateTreeWanderTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	/** Constructor */
	FStateTreeWanderTask()
	{
		// wandering advances over time (move -> pause -> move)
		bShouldCallTick = true;

		// keep running across reselection without restarting the walk
		bShouldStateChangeOnReselect = false;
	}

	/* Ensure we're using the correct instance data struct */
	using FInstanceDataType = FStateTreeWanderInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	/** Kicks off the first wander move when the state is entered */
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

	/** Advances the move / pause cycle each tick */
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

	/** Stops any in-flight movement when the state is left */
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif // WITH_EDITOR
};

////////////////////////////////////////////////////////////////////

/**
 *  Instance data for the Flee task.
 */
USTRUCT()
struct FStateTreeFleeInstanceData
{
	GENERATED_BODY()

	/** AI Controller driving the fleeing pawn */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> Controller;

	/** The threat to run away from (bind to the perception PredatorTarget) */
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<AActor> Threat;

	/** How far to try to run away from the threat on each repath */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = 0, Units = "cm"))
	float FleeDistance = 1500.0f;

	/** How often to recompute the escape point while the threat keeps moving */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = 0, Units = "s"))
	float RepathInterval = 0.5f;

	/** How close to an escape point counts as "arrived" */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = 0, Units = "cm"))
	float AcceptanceRadius = 100.0f;

	// --- runtime state ---

	/** Time accumulated since the last repath */
	UPROPERTY()
	float TimeSinceRepath = 0.0f;
};

/**
 *  StateTree task that makes an AI pawn flee from a threat: it repeatedly heads
 *  to a navmesh point directly away from the threat, repathing as the threat
 *  moves. Stays Running; the brain leaves FLEE when the predator is lost.
 */
USTRUCT(meta=(DisplayName="Flee", Category="Creature"))
struct FStateTreeFleeTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	/** Constructor */
	FStateTreeFleeTask()
	{
		// fleeing re-evaluates the escape point over time
		bShouldCallTick = true;

		// keep running across reselection without restarting the escape
		bShouldStateChangeOnReselect = false;
	}

	/* Ensure we're using the correct instance data struct */
	using FInstanceDataType = FStateTreeFleeInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	/** Runs away from the threat when the state is entered */
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

	/** Repaths away from the (moving) threat each tick */
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

	/** Stops any in-flight movement when the state is left */
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif // WITH_EDITOR
};
