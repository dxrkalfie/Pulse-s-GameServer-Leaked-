#pragma once
#include "pch.h"

class Inventory
{
public:
	static UFortWorldItem* CreateItem(AFortInventory*, UFortItemDefinition*, int32 = 1, int32 = 0);

	static UFortWorldItem* GetWorldItem(AFortInventory*, FGuid&);

	static FFortItemEntry* GetItemEntry(AFortInventory*, FGuid&);
	static FFortItemEntry* GetItemEntry(AFortInventory*, UFortItemDefinition*);
	static FFortItemEntry* GetItemEntry(AFortInventory*, UClass*);

	static FFortRangedWeaponStats* GetStats(UFortWeaponItemDefinition*);

	static FFortItemEntry* MakeItemEntry(UFortItemDefinition*, int32, int32);

	static AFortPickupAthena* SpawnPickup(FVector, FFortItemEntry*, int32, AFortPlayerPawn*, EFortPickupSourceTypeFlag, EFortPickupSpawnSource);
	static AFortPickupAthena* SpawnPickup(FVector, UFortItemDefinition*, int32, AFortPlayerPawn*, EFortPickupSourceTypeFlag, EFortPickupSpawnSource);

	static EFortQuickBars GetQuickbar(UFortItemDefinition*);

	static void UpdateEntry(AFortInventory*, FFortItemEntry*);
	static void RemoveItem(AFortInventory*, FGuid&);
private:
	static void SetLoadedAmmo(UFortWorldItem*, int32);
	static bool RemoveInventoryItem(IInterface*, FGuid&, int32, bool);
public:
	static void Setup();
};