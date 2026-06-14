#pragma once
#include "pch.h"

class GameMode
{
	class Originals
	{
	public:
		static inline void (*HandleStartingNewPlayer)(AGameModeBase*, APlayerController*);
		static inline void (*OnAircraftExitedDropZone)(AFortGameModeAthena*, AFortAthenaAircraft*);
	};

private:
	static bool ReadyToStartMatch(AFortGameModeAthena*);
	static void HandleStartingNewPlayer(AGameModeBase*, APlayerController*);

	static APawn* SpawnDefaultPawnFor(AGameModeBase*, AController*, AActor*);
	static EFortTeam PickTeam(AFortGameModeAthena*, uint8, AFortPlayerControllerAthena*);

	static void OnAircraftExitedDropZone(AFortGameModeAthena*, AFortAthenaAircraft*);
public:
	static void Setup();
};