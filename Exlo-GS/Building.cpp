#include "pch.h"
#include "Building.h"
#include "Utils.h"
#include "Inventory.h"
#include "Configuration.h"

void Building::SetEditingPlayer(ABuildingSMActor* Building, AFortPlayerStateAthena* EditingPlayer)
{
	if (Building->Role == ENetRole::ROLE_Authority && (!Building->EditingPlayer || !EditingPlayer))
	{
		Building->SetNetDormancy(ENetDormancy(2 - (EditingPlayer != 0)));
		Building->ForceNetUpdate();

		if (Building->EditingPlayer)
		{
			if (AActor* Owner = Building->EditingPlayer->Owner)
			{
				if (AFortPlayerControllerAthena* PlayerController = Cast<AFortPlayerControllerAthena>(Owner))
				{
					Building->EditingPlayer = EditingPlayer;
					Building->OnRep_EditingPlayer();
				}

				return;
			}
		}
		else
		{
			if (EditingPlayer == NULL)
			{
				Building->EditingPlayer = EditingPlayer;
				Building->OnRep_EditingPlayer();

				return;
			}

			if (AActor* Owner = EditingPlayer->Owner)
			{
				if (AFortPlayerControllerAthena* PlayerController = Cast<AFortPlayerControllerAthena>(Owner))
				{
					Building->EditingPlayer = EditingPlayer;
					Building->OnRep_EditingPlayer();
				}

				return;
			}
		}
	}
}

void Building::ServerBeginEditingBuildingActor(AFortPlayerController* PlayerController, ABuildingSMActor* Building)
{
	if (PlayerController->MyFortPawn == NULL || Building == NULL || ((AFortPlayerStateAthena*)PlayerController->PlayerState)->TeamIndex != Building->TeamIndex)
		return;

	AFortPlayerStateAthena* PlayerState = Cast<AFortPlayerStateAthena>(PlayerController->PlayerState);

	if (PlayerState == NULL)
		return;

	SetEditingPlayer(Building, PlayerState);

	if (!PlayerController->MyFortPawn->CurrentWeapon->IsA(AFortWeap_EditingTool::StaticClass()))
	{
		FFortItemEntry* ItemEntry = Inventory::GetItemEntry(PlayerController->WorldInventory, UFortEditToolItemDefinition::StaticClass());

		if (ItemEntry != NULL)
		{
			PlayerController->MyFortPawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)ItemEntry->ItemDefinition, ItemEntry->ItemGuid);
		}
	}

	if (AFortWeap_EditingTool* EditingTool = Cast<AFortWeap_EditingTool>(PlayerController->MyFortPawn->CurrentWeapon))
	{
		EditingTool->EditActor = Building;
		
		EditingTool->ForceNetUpdate();
		EditingTool->OnRep_EditActor();
	}
}

void Building::ServerEditBuildingActor(AFortPlayerController* PlayerController, ABuildingSMActor* Building, TSubclassOf<ABuildingSMActor> NewClass, uint8 RotationIterations, bool bMirrored)
{
	if (Building == NULL || NewClass == NULL || Building->bDestroyed || Building->EditingPlayer != PlayerController->PlayerState || ((AFortPlayerStateAthena*)PlayerController->PlayerState)->TeamIndex != Building->TeamIndex)
		return;

	SetEditingPlayer(Building, NULL);

	if (ABuildingSMActor* NewBuilding = Building->ReplaceBuildingActor(EBuildingReplacementType::BRT_Edited, NewClass, Building->CurrentBuildingLevel, RotationIterations, bMirrored, PlayerController))
	{
		NewBuilding->bPlayerPlaced = true;
	}
}

void Building::ServerEndEditingBuildingActor(AFortPlayerController* PlayerController, ABuildingSMActor* Building)
{
	if (PlayerController->MyFortPawn == NULL || Building == NULL || Building->bDestroyed || ((AFortPlayerStateAthena*)PlayerController->PlayerState)->TeamIndex != Building->TeamIndex)
		return;

	SetEditingPlayer(Building, NULL);

	if (AFortWeap_EditingTool* EditingTool = Cast<AFortWeap_EditingTool>(PlayerController->MyFortPawn->CurrentWeapon))
	{
		EditingTool->EditActor = NULL;

		EditingTool->ForceNetUpdate();
		EditingTool->OnRep_EditActor();
	}
}

void Building::ServerCreateBuildingActor(AFortPlayerControllerAthena* PlayerController, FCreateBuildingActorData BuildingActorData)
{
	AFortGameStateAthena* GameState = Cast<AFortGameStateAthena>(UWorld::GetWorld()->GameState);

	if (GameState == NULL)
		return;

	UClass* BuildingClass = PlayerController->BroadcastRemoteClientInfo->RemoteBuildableClass.Get();

	if (BuildingClass == NULL)
		return;

	EFortBuildPreviewMarkerOptionalAdjustment MarkerOptionalAdjustment;

	TArray<ABuildingActor*> ExistingBuildings;

	EFortStructuralGridQueryResults QueryResults = GameState->StructuralSupportSystem->CanAddBuildingActorClassToGrid(UWorld::GetWorld(), BuildingClass, BuildingActorData.BuildLoc, BuildingActorData.BuildRot, BuildingActorData.bMirrored, &ExistingBuildings, &MarkerOptionalAdjustment, false);

	if (QueryResults == EFortStructuralGridQueryResults::CanAdd)
	{
		for (ABuildingActor* ExistingBuilding : ExistingBuildings)
		{
			ExistingBuilding->K2_DestroyActor();
		}

		ABuildingSMActor* Building = Utils::SpawnActor<ABuildingSMActor>(BuildingActorData.BuildLoc, BuildingActorData.BuildRot, BuildingClass, PlayerController);

		if (Building == NULL)
			return;

		Building->InitializeKismetSpawnedBuildingActor(Building, PlayerController, true);
		Building->bPlayerPlaced = true;

		if (AFortPlayerStateAthena* PlayerState = Cast<AFortPlayerStateAthena>(PlayerController->PlayerState))
		{
			Building->PlacedByPlayer(PlayerState);
			Building->SetTeam(PlayerState->TeamIndex);

			Building->OnRep_Team();
		}
		
		if (GameState->GamePhase > EAthenaGamePhase::Warmup)
		{
			if (FFortItemEntry* ItemEntry = Inventory::GetItemEntry(PlayerController->WorldInventory, UFortKismetLibrary::K2_GetResourceItemDefinition(((ABuildingSMActor*)BuildingClass->DefaultObject)->ResourceType)))
			{
				ItemEntry->Count -= 10;

				if (ItemEntry->Count <= 0)
					Inventory::RemoveItem(PlayerController->WorldInventory, ItemEntry->ItemGuid);
				else
					Inventory::UpdateEntry(PlayerController->WorldInventory, ItemEntry);
			}
		}
	}
}

void Building::AttemptSpawnResources(ABuildingSMActor* BuildingActor, AFortPlayerPawn* InstigatorPawn, float DamageDealt, bool bJustHitWeakspot)
{
	AFortPlayerController* PlayerController = Cast<AFortPlayerController>(InstigatorPawn->Controller);

	if (PlayerController == NULL)
		return;

	UFortResourceItemDefinition* ResourceItemDefinition = UFortKismetLibrary::K2_GetResourceItemDefinition(BuildingActor->ResourceType);

	if (ResourceItemDefinition == NULL)
		return;

	float Result;

	UFortKismetLibrary::EvaluateCurveTableRow(BuildingActor->BuildingResourceAmountOverride, 0.f, &Result, FString());

	int32 ResourceCount = UKismetMathLibrary::Round(Result / (BuildingActor->GetMaxHealth() / DamageDealt));

	if (ResourceCount == 0)
		return;

	UFortResourceItemDefinition* Resource = UFortKismetLibrary::K2_GetResourceItemDefinition(BuildingActor->ResourceType);

	if (Resource == 0)
		return;

	int32 MaxCount = Configuration::bIsLateGame ? 500 : 999;

	if (FFortItemEntry* ItemEntry = Inventory::GetItemEntry(PlayerController->WorldInventory, Resource))
	{
		ItemEntry->Count += ResourceCount;

		if (ItemEntry->Count > MaxCount)
		{
			Inventory::SpawnPickup(InstigatorPawn->K2_GetActorLocation(), ItemEntry, ResourceCount - 999, InstigatorPawn, EFortPickupSourceTypeFlag::Tossed, EFortPickupSpawnSource::Unset);

			ItemEntry->Count = MaxCount;
		}

		Inventory::UpdateEntry(PlayerController->WorldInventory, ItemEntry);
	}
	else
	{
		if (ResourceCount > MaxCount)
		{
			Inventory::SpawnPickup(InstigatorPawn->K2_GetActorLocation(), Resource, ResourceCount - 999, InstigatorPawn, EFortPickupSourceTypeFlag::Tossed, EFortPickupSpawnSource::Unset);

			ResourceCount = MaxCount;
		}

		Inventory::CreateItem(PlayerController->WorldInventory, Resource, ResourceCount);
	}

	PlayerController->ClientReportDamagedResourceBuilding(BuildingActor, BuildingActor->ResourceType, ResourceCount, BuildingActor->bDestroyed, bJustHitWeakspot);
}

void Building::Setup()
{
	Utils::Virtual<AFortPlayerControllerAthena>(0x1128 / 8, ServerEditBuildingActor);
	Utils::Virtual<AFortPlayerControllerAthena>(0x1150 / 8, ServerBeginEditingBuildingActor);
	Utils::Virtual<AFortPlayerControllerAthena>(0x1140 / 8, ServerEndEditingBuildingActor);
	Utils::Virtual<AFortPlayerControllerAthena>(0x1118 / 8, ServerCreateBuildingActor);

	Utils::VirtualEvery<ABuildingSMActor>(0xAA8 / 8, AttemptSpawnResources);
}