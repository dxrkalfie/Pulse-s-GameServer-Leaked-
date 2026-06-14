#pragma once
#include "pch.h"

class FFrame;

class Player
{
	class Originals
	{
	public:
		static inline void (*GetPlayerViewPoint)(AFortPlayerControllerAthena*, FVector&, FRotator&);
		static inline void (*ClientOnPawnDied)(AFortPlayerController*, FFortPlayerDeathReport&);
		static inline void (*ServerAttemptAircraftJump)(AFortPlayerController*, FRotator&);
		static inline bool (*FinishedTargetSpline)(AFortPickup*);
		static inline void (*ServerClientIsReadyToRespawn)(AFortPlayerControllerAthena*);
	};

private:
	static void ServerAcknowledgePossession(AFortPlayerControllerAthena*, APawn*);
	static void GetPlayerViewPoint(AFortPlayerControllerAthena*, FVector&, FRotator&);
	static void ServerExecuteInventoryItem(AFortPlayerController*, FGuid&);
	static void ServerPlayEmoteItem(AFortPlayerControllerAthena*, UFortMontageItemDefinitionBase*);
	static void RemoveAllInventoryItems(AFortPlayerControllerAthena*);
	static void ServerSendZiplineState(AFortPlayerPawn*, FZiplinePawnState&);
	static void ClientOnPawnDied(AFortPlayerControllerAthena*, FFortPlayerDeathReport&);
	static void ServerAttemptInventoryDrop(AFortPlayerController*, FGuid&, int32, bool);
	static void ServerAttemptAircraftJump(AFortPlayerController*, FRotator&);
	static void ServerReviveFromDBNO(AFortPlayerPawnAthena*, AController*);
	static void ServerHandlePickup(AFortPlayerPawn*, AFortPickup*, float, FVector&, bool);
	static bool FinishedTargetSpline(AFortPickup*);
	static void InternalPickup(AFortPlayerControllerAthena*, FFortItemEntry&);
	static void MovingEmoteStopped(AFortPawn*, FFrame&);
	static void ServerClientIsReadyToRespawn(AFortPlayerControllerAthena*);

public:
	static void Setup();
};