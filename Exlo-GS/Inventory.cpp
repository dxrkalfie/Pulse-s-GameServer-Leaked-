#include "pch.h"
#include "Inventory.h"
#include "Utils.h"
#include "Configuration.h"

UFortWorldItem* Inventory::CreateItem(AFortInventory* WorldInventory, UFortItemDefinition* ItemDefinition, int32 Count, int32 LoadedAmmo)
{
	if (ItemDefinition == NULL)
		return NULL;

	UFortWorldItem* WorldItem = (UFortWorldItem*)ItemDefinition->CreateTemporaryItemInstanceBP(Count, 0);

	if (WorldItem == NULL)
		return NULL;

	AFortPlayerController* PlayerController = Cast<AFortPlayerController>(WorldInventory->GetOwner() ? WorldInventory->GetOwner() : WorldInventory->GetInstigatorController());

	if (PlayerController == NULL)
		return NULL;

	WorldItem->OwnerInventory = PlayerController->WorldInventory;
	WorldItem->SetOwningControllerForTemporaryItem(PlayerController);

	WorldItem->ItemEntry.ItemDefinition = ItemDefinition;
	WorldItem->ItemEntry.LoadedAmmo = LoadedAmmo;

	WorldInventory->Inventory.ItemInstances.Add(WorldItem);
	WorldInventory->Inventory.ReplicatedEntries.Add(WorldItem->ItemEntry);

	WorldInventory->Inventory.MarkItemDirty(WorldItem->ItemEntry);
	WorldInventory->HandleInventoryLocalUpdate();

	return WorldItem;
}

UFortWorldItem* Inventory::GetWorldItem(AFortInventory* WorldInventory, FGuid& ItemGuid)
{
	UFortWorldItem* WorldItem = NULL;

	for (UFortWorldItem* ItemInstance : WorldInventory->Inventory.ItemInstances)
	{
		if (ItemInstance->ItemEntry.ItemGuid == ItemGuid)
		{
			WorldItem = ItemInstance;
			break;
		}
	}

	return WorldItem;
}

FFortItemEntry* Inventory::GetItemEntry(AFortInventory* WorldInventory, FGuid& ItemGuid)
{
	FFortItemEntry* ItemEntry = NULL;

	for (FFortItemEntry& ReplicatedEntry : WorldInventory->Inventory.ReplicatedEntries)
	{
		if (ReplicatedEntry.ItemGuid == ItemGuid)
		{
			ItemEntry = &ReplicatedEntry;
			break;
		}
	}

	return ItemEntry;
}

FFortItemEntry* Inventory::GetItemEntry(AFortInventory* WorldInventory, UFortItemDefinition* ItemDefinition)
{
	FFortItemEntry* ItemEntry = NULL;

	for (FFortItemEntry& ReplicatedEntry : WorldInventory->Inventory.ReplicatedEntries)
	{
		if (ReplicatedEntry.ItemDefinition == ItemDefinition)
		{
			ItemEntry = &ReplicatedEntry;
			break;
		}
	}

	return ItemEntry;
}

FFortItemEntry* Inventory::GetItemEntry(AFortInventory* WorldInventory, UClass* Class)
{
	FFortItemEntry* ItemEntry = NULL;

	for (FFortItemEntry& ReplicatedEntry : WorldInventory->Inventory.ReplicatedEntries)
	{
		if (ReplicatedEntry.ItemDefinition->Class == Class)
		{
			ItemEntry = &ReplicatedEntry;
			break;
		}
	}

	return ItemEntry;
}

FFortRangedWeaponStats* Inventory::GetStats(UFortWeaponItemDefinition* ItemDefinition)
{
	if (ItemDefinition == NULL || ItemDefinition->WeaponStatHandle.DataTable == NULL)
		return NULL;

	for (const auto& [RowName, RowValue] : ItemDefinition->WeaponStatHandle.DataTable->RowMap)
	{
		FFortRangedWeaponStats* WeaponStats = (FFortRangedWeaponStats*)RowValue;

		if (WeaponStats == NULL)
			continue;

		if (ItemDefinition->WeaponStatHandle.RowName == RowName)
		{
			return WeaponStats;
		}
	}

	return NULL;
}

FFortItemEntry* Inventory::MakeItemEntry(UFortItemDefinition* ItemDefinition, int32 Count, int32 Level)
{
	FFortItemEntry ItemEntry = FFortItemEntry{};

	ItemEntry.MostRecentArrayReplicationKey = -1;
	ItemEntry.ReplicationID = -1;
	ItemEntry.ReplicationKey = -1;

	ItemEntry.ItemDefinition = ItemDefinition;
	ItemEntry.Count = Count;
	ItemEntry.Level = Level;

	if (UFortWeaponItemDefinition* Weapon = Cast<UFortWeaponItemDefinition>(ItemDefinition))
	{
		ItemEntry.LoadedAmmo = GetStats(Weapon)->ClipSize;
	}

	return &ItemEntry;
}

AFortPickupAthena* Inventory::SpawnPickup(FVector Location, FFortItemEntry* ItemEntry, int32 Count, AFortPlayerPawn* Pawn, EFortPickupSourceTypeFlag SourceTypeFlag, EFortPickupSpawnSource SpawnSource)
{
	AFortPickupAthena* Pickup = Utils::SpawnActor<AFortPickupAthena>(Location);

	if (Pickup == NULL)
		return NULL;

	Pickup->PrimaryPickupItemEntry.Level = ItemEntry->Level;
	Pickup->PrimaryPickupItemEntry.ItemDefinition = ItemEntry->ItemDefinition;
	Pickup->PrimaryPickupItemEntry.LoadedAmmo = ItemEntry->LoadedAmmo;
	Pickup->PrimaryPickupItemEntry.Count = Count != -1 ? Count : ItemEntry->Count;

	Pickup->OnRep_PrimaryPickupItemEntry();

	Pickup->bRandomRotation = true;

	Pickup->PawnWhoDroppedPickup = Pawn ? Pawn : NULL;
	Pickup->TossPickup(Location, Pickup->PawnWhoDroppedPickup, -1, true, SourceTypeFlag, SpawnSource);

	if (SpawnSource != EFortPickupSpawnSource::Unset)
	{
		Pickup->bTossedFromContainer = SpawnSource == EFortPickupSpawnSource::Chest || SpawnSource == EFortPickupSpawnSource::AmmoBox;
		Pickup->OnRep_TossedFromContainer();
	}

	return Pickup;
}

AFortPickupAthena* Inventory::SpawnPickup(FVector Location, UFortItemDefinition* ItemDefinition, int32 Count, AFortPlayerPawn* Pawn, EFortPickupSourceTypeFlag SourceTypeFlag, EFortPickupSpawnSource SpawnSource)
{
	return SpawnPickup(Location, MakeItemEntry(ItemDefinition, Count, 0), Count, Pawn, SourceTypeFlag, SpawnSource);
}

EFortQuickBars Inventory::GetQuickbar(UFortItemDefinition* ItemDefinition)
{
	if (ItemDefinition == NULL)
		return EFortQuickBars::Max_None;

	return ItemDefinition->IsA(UFortWeaponMeleeItemDefinition::StaticClass()) || ItemDefinition->IsA(UFortResourceItemDefinition::StaticClass()) || ItemDefinition->IsA(UFortAmmoItemDefinition::StaticClass()) || ItemDefinition->IsA(UFortTrapItemDefinition::StaticClass()) || ItemDefinition->IsA(UFortBuildingItemDefinition::StaticClass()) || ItemDefinition->IsA(UFortEditToolItemDefinition::StaticClass()) || ((UFortWorldItemDefinition*)ItemDefinition)->bForceIntoOverflow ? EFortQuickBars::Secondary : EFortQuickBars::Primary;
}

void Inventory::UpdateEntry(AFortInventory* WorldInventory, FFortItemEntry* ItemEntry)
{
	if (ItemEntry->bIsReplicatedCopy)
	{
		ItemEntry->bIsDirty = false;

		WorldInventory->Inventory.MarkItemDirty(*ItemEntry);
		WorldInventory->bRequiresLocalUpdate = true;

		return;
	}

	for (FFortItemEntry& ReplicatedEntry : WorldInventory->Inventory.ReplicatedEntries)
	{
		if (ReplicatedEntry.ItemGuid == ItemEntry->ItemGuid)
		{
			ReplicatedEntry = *ItemEntry;
			ReplicatedEntry.bIsDirty = false;

			WorldInventory->Inventory.MarkItemDirty(*ItemEntry);

			WorldInventory->bRequiresLocalUpdate = true;
			WorldInventory->HandleInventoryLocalUpdate();

			if (ItemEntry)
				WorldInventory->Inventory.MarkItemDirty(*ItemEntry);
			else
				WorldInventory->Inventory.MarkArrayDirty();
		}
	}
}

void Inventory::RemoveItem(AFortInventory* WorldInventory, FGuid& ItemGuid)
{
	for (int i = 0; i < WorldInventory->Inventory.ItemInstances.Num(); i++)
	{
		if (WorldInventory->Inventory.ItemInstances[i]->ItemEntry.ItemGuid == ItemGuid)
		{
			WorldInventory->Inventory.ItemInstances.Remove(i);
			break;
		}
	}

	for (int i = 0; i < WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
	{
		if (WorldInventory->Inventory.ReplicatedEntries[i].ItemGuid == ItemGuid)
		{
			WorldInventory->Inventory.ReplicatedEntries.Remove(i);
			break;
		}
	}

	WorldInventory->bRequiresLocalUpdate = true;
	WorldInventory->HandleInventoryLocalUpdate();

	WorldInventory->Inventory.MarkArrayDirty();
}

void Inventory::SetLoadedAmmo(UFortWorldItem* WorldItem, int32 LoadedAmmo)
{
	AFortPlayerControllerAthena* PlayerController = Cast<AFortPlayerControllerAthena>(WorldItem->GetOwningController());

	if (PlayerController == NULL)
		return;

	FFortItemEntry* ItemEntry = Inventory::GetItemEntry(PlayerController->WorldInventory, WorldItem->ItemEntry.ItemGuid);

	if (ItemEntry == NULL)
		return;

	ItemEntry->LoadedAmmo = LoadedAmmo;
	WorldItem->ItemEntry.LoadedAmmo = LoadedAmmo;

	PlayerController->WorldInventory->Inventory.MarkItemDirty(*ItemEntry);

	PlayerController->WorldInventory->bRequiresLocalUpdate = true;
	PlayerController->WorldInventory->HandleInventoryLocalUpdate();
}

bool Inventory::RemoveInventoryItem(IInterface* Interface, FGuid& ItemGuid, int32 Count, bool bForceRemoval)
{
	AFortPlayerControllerAthena* PlayerController = (AFortPlayerControllerAthena*)(__int64(Interface) - 0x598);

	if (PlayerController == NULL || PlayerController->WorldInventory == NULL || Configuration::bIs1V1Map)
		return false;

	UFortWorldItem* WorldItem = Inventory::GetWorldItem(PlayerController->WorldInventory, ItemGuid);

	if (WorldItem == NULL)
		return false;

	FFortItemEntry* ItemEntry = Inventory::GetItemEntry(PlayerController->WorldInventory, ItemGuid);

	if (ItemEntry == NULL)
		return false;

	ItemEntry->Count -= Count;

	if (Count < 0 || ItemEntry->Count <= 0 || bForceRemoval)
	{
		RemoveItem(PlayerController->WorldInventory, ItemGuid);
	}
	else
	{
		WorldItem->ItemEntry.Count = ItemEntry->Count;

		PlayerController->WorldInventory->Inventory.MarkItemDirty(*ItemEntry);

		PlayerController->WorldInventory->bRequiresLocalUpdate = true;
		PlayerController->WorldInventory->HandleInventoryLocalUpdate();

		return true;
	}

	return true;
}

void Inventory::Setup()
{
	Utils::Hook(InSDKUtils::GetImageBase() + 0x1837A60, RemoveInventoryItem);

	Utils::Virtual<UFortWorldItem>(0x94, SetLoadedAmmo);
}