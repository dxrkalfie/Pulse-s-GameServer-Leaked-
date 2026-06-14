#pragma once
#include "pch.h"
#include "Utils.h"

class Misc
{
	class Originals
	{
	public:
		static inline void (*TickFlush)(UNetDriver*, float);
		static inline void (*SendRequestNow)(UMcpProfileGroup*, const void*, FBaseUrlContext::EContextCredentials);
		static inline bool (*StartAircraftPhase)(AFortGameModeAthena*, char);
		static inline void (*StartNewSafeZonePhase)(AFortGameModeAthena*, int);
	};

private:
	static void TickFlush(UNetDriver*, float);
	static void SendRequestNow(UMcpProfileGroup*, const void*, FBaseUrlContext::EContextCredentials);

	static bool StartAircraftPhase(AFortGameModeAthena*, char);
	static void StartNewSafeZonePhase(AFortGameModeAthena*, int);

	static void ServerUpdatePhysicsParams(AFortPhysicsPawn*, FReplicatedPhysicsPawnState&);

	static void SetDynamicFoundationEnabled(ABuildingFoundation*, FFrame&);
	static void SetDynamicFoundationTransform(ABuildingFoundation*, FFrame&);

	static float GetMaxTickRate();
	static bool Listen();
public:
	static void Setup();
};