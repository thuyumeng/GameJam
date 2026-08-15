// Copyright Epic Games, Inc. All Rights Reserved.


#include "CreatureStateTreeUtility.h"
#include "StateTreeExecutionContext.h"
#include "Engine/World.h"
#include "Creature.h"
#include "CreatureSized.h"
#include "CreatureSizeStatics.h"
#include "CreaturePerceptionData.h"
#include "Kismet/GameplayStatics.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/Pawn.h"
#include "DrawDebugHelpers.h"

namespace
{
	// Default perception parameters, used when no data asset is available.
	constexpr float DefaultThreshold = 1.2f;
	constexpr float DefaultSightRange = 2500.0f;
	constexpr float DefaultSightConeHalfAngle = 60.0f;

	/**
	 *  Runs one cone + line-of-sight perception pass and fills the classified outputs.
	 *  The last known prey location is only overwritten while a prey is actually seen,
	 *  so it persists briefly after line of sight is lost (basic memory).
	 */
	void PerformPerception(FStateTreeCreaturePerceptionInstanceData& Data, const float DeltaTime)
	{
		// peer has no memory, so clear it every pass
		Data.PeerTarget = nullptr;
		Data.bHasPeer = false;

		ACreature* Self = Data.Creature;
		if (!Self)
		{
			// no perceiver: drop everything, memory included
			Data.PreyTarget = nullptr;
			Data.PredatorTarget = nullptr;
			Data.bHasPrey = false;
			Data.bHasPredator = false;
			return;
		}

		UWorld* World = Self->GetWorld();
		if (!World)
		{
			return;
		}

		// resolve parameters: explicit override -> creature's data asset -> defaults
		const UCreaturePerceptionData* Params = Data.PerceptionDataOverride ? Data.PerceptionDataOverride.Get() : Self->GetPerceptionData();
		const float Threshold = Params ? Params->PredationThreshold : DefaultThreshold;
		const float SightRange = Params ? Params->SightRange : DefaultSightRange;
		const float ConeHalfAngle = Params ? Params->SightConeHalfAngle : DefaultSightConeHalfAngle;
		const float ConeCos = FMath::Cos(FMath::DegreesToRadians(ConeHalfAngle));

		const float MySize = Self->GetCreatureSize();
		const FVector SelfLoc = Self->GetActorLocation();
		const FVector Forward = Self->GetActorForwardVector();

		// trace from roughly eye height so low walls block sight
		const FVector EyeLoc = SelfLoc + FVector(0.0f, 0.0f, Self->BaseEyeHeight);

		// gather every food-chain participant in the world
		TArray<AActor*> Candidates;
		UGameplayStatics::GetAllActorsWithInterface(World, UCreatureSized::StaticClass(), Candidates);

		const float SightRangeSq = FMath::Square(SightRange);
		const float CloseRangeSq = FMath::Square(Data.CloseRangeRadius);

#if ENABLE_DRAW_DEBUG
		// draw the sight cone edges + close-range ring so we can see what it covers
		if (Data.bDrawDebug)
		{
			const FVector ConeEdgeL = Forward.RotateAngleAxis(ConeHalfAngle, FVector::UpVector) * SightRange;
			const FVector ConeEdgeR = Forward.RotateAngleAxis(-ConeHalfAngle, FVector::UpVector) * SightRange;
			DrawDebugLine(World, EyeLoc, EyeLoc + ConeEdgeL, FColor::Yellow, false, -1.0f, 0, 1.5f);
			DrawDebugLine(World, EyeLoc, EyeLoc + ConeEdgeR, FColor::Yellow, false, -1.0f, 0, 1.5f);
			DrawDebugCircle(World, SelfLoc, Data.CloseRangeRadius, 24, FColor(0, 128, 255), false, -1.0f, 0, 1.0f, FVector(1, 0, 0), FVector(0, 1, 0), false);
		}
#endif

		// nearest target actually SEEN this pass, per category
		AActor* SeenPrey = nullptr;
		AActor* SeenPredator = nullptr;
		AActor* SeenPeer = nullptr;
		float BestPreyDistSq = TNumericLimits<float>::Max();
		float BestPredatorDistSq = TNumericLimits<float>::Max();
		float BestPeerDistSq = TNumericLimits<float>::Max();

		for (AActor* Actor : Candidates)
		{
			// skip invalid actors and ourselves
			if (!Actor || Actor == Self)
			{
				continue;
			}

			ICreatureSized* Other = Cast<ICreatureSized>(Actor);
			if (!Other)
			{
				continue;
			}

			const FVector TargetLoc = Actor->GetActorLocation();
			const FVector ToTarget = TargetLoc - SelfLoc;

			// outside max sight distance?
			const float DistSq = ToTarget.SizeSquared();
			if (DistSq > SightRangeSq)
			{
				continue;
			}

			// close targets are sensed regardless of facing (contact awareness);
			// farther ones must fall inside the forward sight cone (rear = blind)
			if (DistSq > CloseRangeSq)
			{
				const FVector Dir = ToTarget.GetSafeNormal2D();
				if (FVector::DotProduct(Dir, Forward) < ConeCos)
				{
#if ENABLE_DRAW_DEBUG
					// orange: in range but outside the sight cone (a blind angle)
					if (Data.bDrawDebug)
					{
						DrawDebugLine(World, EyeLoc, TargetLoc, FColor::Orange, false, -1.0f, 0, 1.0f);
					}
#endif
					continue;
				}
			}

			// line-of-sight check: an occluder between us and the target hides it
			FHitResult Hit;
			FCollisionQueryParams QueryParams;
			QueryParams.AddIgnoredActor(Self);
			QueryParams.AddIgnoredActor(Actor);

			const FVector TargetEye = TargetLoc + FVector(0.0f, 0.0f, 50.0f);
			const bool bBlocked = World->LineTraceSingleByChannel(Hit, EyeLoc, TargetEye, ECC_Visibility, QueryParams);
			if (bBlocked)
			{
#if ENABLE_DRAW_DEBUG
				// red: in the cone but an occluder blocks line of sight
				if (Data.bDrawDebug)
				{
					DrawDebugLine(World, EyeLoc, Hit.ImpactPoint, FColor::Red, false, -1.0f, 0, 1.0f);
					DrawDebugPoint(World, Hit.ImpactPoint, 8.0f, FColor::Red, false, -1.0f);
				}
#endif
				continue;
			}

#if ENABLE_DRAW_DEBUG
			// green: fully perceived this pass (in range, in cone, clear line of sight)
			if (Data.bDrawDebug)
			{
				DrawDebugLine(World, EyeLoc, TargetEye, FColor::Green, false, -1.0f, 0, 2.0f);
			}
#endif

			// classify this visible target by size and keep the nearest per category
			const ECreatureRelation Relation = UCreatureSizeStatics::ClassifyBySize(MySize, Other->GetCreatureSize(), Threshold);
			switch (Relation)
			{
			case ECreatureRelation::Prey:
				if (DistSq < BestPreyDistSq)
				{
					BestPreyDistSq = DistSq;
					SeenPrey = Actor;
				}
				break;

			case ECreatureRelation::Predator:
				if (DistSq < BestPredatorDistSq)
				{
					BestPredatorDistSq = DistSq;
					SeenPredator = Actor;
				}
				break;

			case ECreatureRelation::Peer:
				if (DistSq < BestPeerDistSq)
				{
					BestPeerDistSq = DistSq;
					SeenPeer = Actor;
				}
				break;

			default:
				break;
			}
		}

		// peer: reported only while directly seen
		Data.PeerTarget = SeenPeer;
		Data.bHasPeer = (SeenPeer != nullptr);

		// prey with memory: keep the last target for MemoryDuration after losing sight
		if (SeenPrey)
		{
			Data.PreyTarget = SeenPrey;
			Data.bHasPrey = true;
			Data.LastKnownPreyLocation = SeenPrey->GetActorLocation();
			Data.TimeSincePreySeen = 0.0f;
		}
		else
		{
			Data.TimeSincePreySeen += DeltaTime;
			if (Data.PreyTarget && Data.TimeSincePreySeen < Data.MemoryDuration)
			{
				// still remembered: keep chasing the last prey a little longer
				Data.bHasPrey = true;
			}
			else
			{
				Data.PreyTarget = nullptr;
				Data.bHasPrey = false;
			}
		}

		// predator with memory: keep fleeing after we turn our back and lose sight
		if (SeenPredator)
		{
			Data.PredatorTarget = SeenPredator;
			Data.bHasPredator = true;
			Data.LastKnownPredatorLocation = SeenPredator->GetActorLocation();
			Data.TimeSincePredatorSeen = 0.0f;
		}
		else
		{
			Data.TimeSincePredatorSeen += DeltaTime;
			if (Data.PredatorTarget && Data.TimeSincePredatorSeen < Data.MemoryDuration)
			{
				Data.bHasPredator = true;
			}
			else
			{
				Data.PredatorTarget = nullptr;
				Data.bHasPredator = false;
			}
		}
	}
}

EStateTreeRunStatus FStateTreeCreaturePerceptionTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	// run an immediate perception pass so outputs are valid right away
	PerformPerception(InstanceData, 0.0f);

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FStateTreeCreaturePerceptionTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	// refresh perception each tick
	PerformPerception(InstanceData, DeltaTime);

	return EStateTreeRunStatus::Running;
}

#if WITH_EDITOR
FText FStateTreeCreaturePerceptionTask::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting /*= EStateTreeNodeFormatting::Text*/) const
{
	return FText::FromString("<b>Creature Perception</b>");
}
#endif // WITH_EDITOR

////////////////////////////////////////////////////////////////////

bool FStateTreeWithinDistanceCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	// need both actors to measure a distance
	if (!InstanceData.Source || !InstanceData.Target)
	{
		return false;
	}

	const float DistSq = (InstanceData.Target->GetActorLocation() - InstanceData.Source->GetActorLocation()).SizeSquared();
	const bool bWithin = DistSq <= FMath::Square(InstanceData.Distance);

	return InstanceData.bInvert ? !bWithin : bWithin;
}

#if WITH_EDITOR
FText FStateTreeWithinDistanceCondition::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting /*= EStateTreeNodeFormatting::Text*/) const
{
	return FText::FromString("<b>Actor Within Distance</b>");
}
#endif // WITH_EDITOR

////////////////////////////////////////////////////////////////////

namespace
{
	/**
	 *  Picks a random reachable point within WanderRadius of the pawn and issues
	 *  a move request. Returns true only if the pawn actually started moving
	 *  (i.e. it wasn't already at the goal and pathfinding didn't fail outright).
	 */
	bool StartWanderMove(FStateTreeWanderInstanceData& Data)
	{
		AAIController* Controller = Data.Controller;
		if (!Controller)
		{
			return false;
		}

		const APawn* Pawn = Controller->GetPawn();
		if (!Pawn)
		{
			return false;
		}

		UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(Pawn->GetWorld());
		if (!NavSys)
		{
			return false;
		}

		// find somewhere on the navmesh we can actually reach from here
		FNavLocation Destination;
		if (!NavSys->GetRandomReachablePointInRadius(Pawn->GetActorLocation(), Data.WanderRadius, Destination))
		{
			return false;
		}

		const EPathFollowingRequestResult::Type Result = Controller->MoveToLocation(Destination.Location, Data.AcceptanceRadius);
		return Result == EPathFollowingRequestResult::RequestSuccessful;
	}

	/** Enters the pausing phase with a fresh randomized rest duration. */
	void BeginPause(FStateTreeWanderInstanceData& Data)
	{
		const float MinPause = FMath::Min(Data.MinPauseTime, Data.MaxPauseTime);
		const float MaxPause = FMath::Max(Data.MinPauseTime, Data.MaxPauseTime);

		Data.bIsPausing = true;
		Data.PauseTimeRemaining = FMath::FRandRange(MinPause, MaxPause);
	}
}

EStateTreeRunStatus FStateTreeWanderTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	// head to a first point straight away; if we can't pick one, fall into a
	// pause and retry on a later tick
	if (StartWanderMove(Data))
	{
		Data.bIsPausing = false;
	}
	else
	{
		BeginPause(Data);
	}

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FStateTreeWanderTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	AAIController* Controller = Data.Controller;
	if (!Controller)
	{
		// nothing to drive; stay in the state so the brain keeps ownership
		return EStateTreeRunStatus::Running;
	}

	if (Data.bIsPausing)
	{
		// resting at a reached point; count down, then set off again
		Data.PauseTimeRemaining -= DeltaTime;
		if (Data.PauseTimeRemaining <= 0.0f)
		{
			if (StartWanderMove(Data))
			{
				Data.bIsPausing = false;
			}
			else
			{
				BeginPause(Data);
			}
		}
	}
	else
	{
		// moving; once the path request finishes (reached or aborted), pause
		if (Controller->GetMoveStatus() == EPathFollowingStatus::Idle)
		{
			BeginPause(Data);
		}
	}

	return EStateTreeRunStatus::Running;
}

void FStateTreeWanderTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	// don't leave a stale wander move running when the brain switches state (e.g. to CHASE)
	if (Data.Controller && !Data.bIsPausing)
	{
		Data.Controller->StopMovement();
	}
}

#if WITH_EDITOR
FText FStateTreeWanderTask::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting /*= EStateTreeNodeFormatting::Text*/) const
{
	return FText::FromString("<b>Wander</b>");
}
#endif // WITH_EDITOR

////////////////////////////////////////////////////////////////////

namespace
{
	/**
	 *  Picks a navmesh point directly away from the threat and issues a move.
	 *  Returns true only if the pawn actually started moving.
	 */
	bool StartFleeMove(FStateTreeFleeInstanceData& Data)
	{
		AAIController* Controller = Data.Controller;
		if (!Controller || !Data.Threat)
		{
			return false;
		}

		const APawn* Pawn = Controller->GetPawn();
		if (!Pawn)
		{
			return false;
		}

		UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(Pawn->GetWorld());
		if (!NavSys)
		{
			return false;
		}

		const FVector SelfLoc = Pawn->GetActorLocation();
		const FVector ThreatLoc = Data.Threat->GetActorLocation();

		// flat direction pointing away from the threat
		FVector Away = (SelfLoc - ThreatLoc).GetSafeNormal2D();
		if (Away.IsNearlyZero())
		{
			// threat is right on top of us; just run the way we're facing
			Away = Pawn->GetActorForwardVector().GetSafeNormal2D();
		}

		const FVector DesiredGoal = SelfLoc + Away * Data.FleeDistance;

		// snap the desired escape point onto the navmesh; if it isn't reachable,
		// fall back to any reachable point roughly that far out
		const FVector ProjectExtent(Data.FleeDistance, Data.FleeDistance, Data.FleeDistance);
		FNavLocation Escape;
		if (!NavSys->ProjectPointToNavigation(DesiredGoal, Escape, ProjectExtent) &&
			!NavSys->GetRandomReachablePointInRadius(DesiredGoal, Data.FleeDistance, Escape))
		{
			return false;
		}

		const EPathFollowingRequestResult::Type Result = Controller->MoveToLocation(Escape.Location, Data.AcceptanceRadius);
		return Result == EPathFollowingRequestResult::RequestSuccessful;
	}
}

EStateTreeRunStatus FStateTreeFleeTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	// bolt away from the threat straight away
	Data.TimeSinceRepath = 0.0f;
	StartFleeMove(Data);

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FStateTreeFleeTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	AAIController* Controller = Data.Controller;
	if (!Controller)
	{
		return EStateTreeRunStatus::Running;
	}

	Data.TimeSinceRepath += DeltaTime;

	// repath as the threat keeps moving, or the moment we've reached the last escape point
	const bool bReached = Controller->GetMoveStatus() == EPathFollowingStatus::Idle;
	if (bReached || Data.TimeSinceRepath >= Data.RepathInterval)
	{
		Data.TimeSinceRepath = 0.0f;
		StartFleeMove(Data);
	}

	return EStateTreeRunStatus::Running;
}

void FStateTreeFleeTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	// don't leave a stale escape move running once the threat is gone
	if (Data.Controller)
	{
		Data.Controller->StopMovement();
	}
}

#if WITH_EDITOR
FText FStateTreeFleeTask::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting /*= EStateTreeNodeFormatting::Text*/) const
{
	return FText::FromString("<b>Flee</b>");
}
#endif // WITH_EDITOR
