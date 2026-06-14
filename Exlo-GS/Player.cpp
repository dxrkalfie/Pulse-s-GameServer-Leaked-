#include "pch.h"
#include "Player.h"
#include "Utils.h"
#include "Inventory.h"
#include "Configuration.h"
#include "LateGame.h"
#include "API.h"

void Player::ServerAcknowledgePossession(AFortPlayerControllerAthena* PlayerController, APawn* P)
{
	PlayerController->AcknowledgedPawn = P;

	AFortPlayerStateAthena* PlayerState = Cast<AFortPlayerStateAthena>(PlayerController->PlayerState);

	if (PlayerState == NULL)
		return;

	PlayerState->ApplyCharacterCustomization(PlayerController->MyFortPawn);

	UFortGlobals::InitializePlayerGameplayAbilities(Utils::GetInterfaceAddress<IAbilitySystemInterface>(PlayerState));

	if (AFortGameModeAthena* GameMode = Cast<AFortGameModeAthena>(UWorld::GetWorld()->AuthorityGameMode))
	{
		for (const FItemAndCount& StartingItem : GameMode->StartingItems)
		{
			if (StartingItem.Count > 0)
			{
				Inventory::CreateItem(PlayerController->WorldInventory, StartingItem.Item, StartingItem.Count);
			}
		}

		if (Configuration::bIs1V1Map)
		{
			PlayerController->MyFortPawn->SetShield(100.f);

			Inventory::CreateItem(PlayerController->WorldInventory, Utils::StaticFindObject<UFortItemDefinition>(TEXT("WID_Shotgun_Standard_Athena_SR_Ore_T03"), ANY_PACKAGE), 1, Inventory::GetStats((UFortWeaponItemDefinition*)Utils::StaticFindObject<UFortItemDefinition>(TEXT("WID_Shotgun_Standard_Athena_SR_Ore_T03"), ANY_PACKAGE))->ClipSize);
			Inventory::CreateItem(PlayerController->WorldInventory, Utils::StaticFindObject<UFortItemDefinition>(TEXT("WID_Assault_AutoHigh_Athena_SR_Ore_T03"), ANY_PACKAGE), 1, Inventory::GetStats((UFortWeaponItemDefinition*)Utils::StaticFindObject<UFortItemDefinition>(TEXT("WID_Shotgun_Standard_Athena_SR_Ore_T03"), ANY_PACKAGE))->ClipSize);

			Inventory::CreateItem(PlayerController->WorldInventory, Utils::StaticFindObject<UFortItemDefinition>(TEXT("WID_Hook_Gun_Slide"), ANY_PACKAGE), 1);

			Inventory::CreateItem(PlayerController->WorldInventory, Utils::StaticFindObject<UFortItemDefinition>(TEXT("Athena_PurpleStuff"), ANY_PACKAGE), 3);
			Inventory::CreateItem(PlayerController->WorldInventory, Utils::StaticFindObject<UFortItemDefinition>(TEXT("Athena_ShieldSmall"), ANY_PACKAGE), 3);

			Inventory::CreateItem(PlayerController->WorldInventory, LateGame::GetResource(EFortResourceType::Wood), 500);
			Inventory::CreateItem(PlayerController->WorldInventory, LateGame::GetResource(EFortResourceType::Stone), 500);
			Inventory::CreateItem(PlayerController->WorldInventory, LateGame::GetResource(EFortResourceType::Metal), 500);

			Inventory::CreateItem(PlayerController->WorldInventory, LateGame::GetAmmo(EAmmoType::Assault), 250);
			Inventory::CreateItem(PlayerController->WorldInventory, LateGame::GetAmmo(EAmmoType::Shotgun), 100);
			Inventory::CreateItem(PlayerController->WorldInventory, LateGame::GetAmmo(EAmmoType::Submachine), 400);
			Inventory::CreateItem(PlayerController->WorldInventory, LateGame::GetAmmo(EAmmoType::Sniper), 20);
		}

		Inventory::CreateItem(PlayerController->WorldInventory, PlayerController->CosmeticLoadoutPC.Pickaxe->WeaponDefinition);
	}
}

void Player::GetPlayerViewPoint(AFortPlayerControllerAthena* PlayerController, FVector& Location, FRotator& Rotation)
{
	if (PlayerController->GetViewTarget())
	{
		Location = PlayerController->GetViewTarget()->K2_GetActorLocation();
		Rotation = PlayerController->GetControlRotation();
	}
	else if (PlayerController->StateName == UKismetStringLibrary::Conv_StringToName(L"Spectating"))
	{
		Location = PlayerController->LastSpectatorSyncLocation;
		Rotation = PlayerController->LastSpectatorSyncRotation;
	}
	else
	{
		return Originals::GetPlayerViewPoint(PlayerController, Location, Rotation);
	}
}

void Player::ServerExecuteInventoryItem(AFortPlayerController* PlayerController, FGuid& ItemGuid)
{
	AFortPlayerPawnAthena* PlayerPawn = Cast<AFortPlayerPawnAthena>(PlayerController->MyFortPawn);

	if (PlayerPawn == NULL)
		return;

	FFortItemEntry* ItemEntry = Inventory::GetItemEntry(PlayerController->WorldInventory, ItemGuid);

	if (ItemEntry == NULL)
		return;

	UFortWeaponItemDefinition* WeaponItemDefinition = Cast<UFortWeaponItemDefinition>(ItemEntry->ItemDefinition);

	if (WeaponItemDefinition == NULL)
		return;

	if (UFortContextTrapItemDefinition* ContextTrapItemDefinition = Cast<UFortContextTrapItemDefinition>(WeaponItemDefinition))
	{
		PlayerPawn->PickUpActor(NULL, ContextTrapItemDefinition);
		PlayerPawn->CurrentWeapon->ItemEntryGuid = ItemGuid;

		if (AFortDecoTool_ContextTrap* ContextTrap = Cast<AFortDecoTool_ContextTrap>(PlayerPawn->CurrentWeapon))
		{
			ContextTrap->ContextTrapItemDefinition = ContextTrapItemDefinition;
		}
	}
	else
	{
		PlayerPawn->EquipWeaponDefinition(WeaponItemDefinition, ItemGuid);
	}
}

void Player::ServerPlayEmoteItem(AFortPlayerControllerAthena* PlayerController, UFortMontageItemDefinitionBase* EmoteAsset)
{
	AFortPlayerPawn* PlayerPawn = PlayerController->MyFortPawn;

	if (PlayerPawn == NULL)
		return;

	AFortPlayerStateAthena* PlayerState = Cast<AFortPlayerStateAthena>(PlayerController->PlayerState);

	if (PlayerState == NULL)
		return;

	UClass* AbilityClass = NULL;

	if (UAthenaDanceItemDefinition* DanceItemDefinition = Cast<UAthenaDanceItemDefinition>(EmoteAsset))
	{
		AbilityClass = DanceItemDefinition->CustomDanceAbility.Get() ? DanceItemDefinition->CustomDanceAbility.Get() : Utils::StaticFindObject<UClass>(TEXT("/Game/Abilities/Emotes/GAB_Emote_Generic.GAB_Emote_Generic_C"));

		PlayerPawn->bMovingEmote = DanceItemDefinition->bMovingEmote;
		PlayerPawn->EmoteWalkSpeed = DanceItemDefinition->WalkForwardSpeed;
		PlayerPawn->bMovingEmoteForwardOnly = DanceItemDefinition->bMoveForwardOnly;
	}
	else if (UAthenaSprayItemDefinition* SprayItemDefinition = Cast<UAthenaSprayItemDefinition>(EmoteAsset))
	{
		AbilityClass = Utils::StaticFindObject<UClass>(TEXT("/Game/Abilities/Sprays/GAB_Spray_Generic.GAB_Spray_Generic_C"));
	}

	if (AbilityClass != NULL)
	{
		FGameplayAbilitySpec Spec;
		Spec.ConstructAbilitySpec(Cast<UGameplayAbility>(AbilityClass->DefaultObject), 1, -1, EmoteAsset);

		PlayerState->AbilitySystemComponent->GiveAbilityAndActivateOnce(&Spec.Handle, &Spec);
	}
}

void Player::RemoveAllInventoryItems(AFortPlayerControllerAthena* PlayerController)
{
	PlayerController->WorldInventory->Inventory.ReplicatedEntries.Free();
	PlayerController->WorldInventory->Inventory.ItemInstances.Free();

	PlayerController->WorldInventory->Inventory.MarkArrayDirty();
}

void Player::ServerSendZiplineState(AFortPlayerPawn* PlayerPawn, FZiplinePawnState& InZiplineState)
{
	PlayerPawn->ZiplineState = InZiplineState;
	PlayerPawn->OnRep_ZiplineState();

	if (InZiplineState.bJumped)
	{
		FVector Velocity = FVector{};

		Velocity.X = 0;
		Velocity.Z = 1500;

		PlayerPawn->LaunchCharacterJump(Velocity, false, false, true, true);
	}
}

void Player::ClientOnPawnDied(AFortPlayerControllerAthena* PlayerController, FFortPlayerDeathReport& DeathReport)
{
	Originals::ClientOnPawnDied(PlayerController, DeathReport);

	AFortPlayerStateAthena* PlayerState = Cast<AFortPlayerStateAthena>(PlayerController->PlayerState);

	if (PlayerState == NULL)
		return;

	AFortPlayerPawn* PlayerPawn = PlayerController->MyFortPawn;

	if (PlayerPawn == NULL)
		return;

	AFortGameStateAthena* GameState = Cast<AFortGameStateAthena>(UWorld::GetWorld()->GameState);

	if (GameState == NULL)
		return;

	AFortPlayerStateAthena* KillerPlayerState = Cast<AFortPlayerStateAthena>(DeathReport.KillerPlayerState);

	if (!GameState->IsRespawningAllowed(PlayerState))
	{
		if (PlayerController->WorldInventory != NULL && PlayerController->WorldInventory->Inventory.ReplicatedEntries.IsValid())
		{
			for (FFortItemEntry& ItemEntry : PlayerController->WorldInventory->Inventory.ReplicatedEntries)
			{
				if (ItemEntry.ItemDefinition && !ItemEntry.ItemDefinition->IsA(UFortWeaponMeleeItemDefinition::StaticClass()) && (ItemEntry.ItemDefinition->IsA(UFortResourceItemDefinition::StaticClass()) || ItemEntry.ItemDefinition->IsA(UFortWeaponRangedItemDefinition::StaticClass()) || ItemEntry.ItemDefinition->IsA(UFortConsumableItemDefinition::StaticClass()) || ItemEntry.ItemDefinition->IsA(UFortAmmoItemDefinition::StaticClass())))
				{
					Inventory::SpawnPickup(PlayerPawn->K2_GetActorLocation() + PlayerController->Pawn->GetActorForwardVector() * 70.f + FVector(0, 0, 50), &ItemEntry, ItemEntry.Count, PlayerPawn, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::PlayerElimination);
				}
			}
		}
	}

	AFortPawn* KillerPawn = DeathReport.KillerPawn;

	if (KillerPawn == NULL)
		return;

	FDeathInfo DeathInfo = FDeathInfo{};

	DeathInfo.bDBNO = PlayerPawn ? PlayerPawn->IsDBNO() : false;
	DeathInfo.bInitialized = true;
	DeathInfo.DeathCause = PlayerState->ToDeathCause(DeathReport.Tags, PlayerPawn ? PlayerPawn->IsDBNO() : false);
	DeathInfo.DeathLocation = PlayerPawn->K2_GetActorLocation();
	DeathInfo.DeathTags = DeathReport.Tags;
	DeathInfo.Distance = (KillerPawn && PlayerPawn) ? KillerPawn->GetDistanceTo(PlayerPawn) : 0.f;
	DeathInfo.Downer = KillerPlayerState ? KillerPlayerState : PlayerState;
	DeathInfo.FinisherOrDowner = KillerPlayerState ? KillerPlayerState : PlayerState;

	PlayerState->DeathInfo = DeathInfo;
	PlayerState->OnRep_DeathInfo();

	if (KillerPlayerState && KillerPlayerState != PlayerState && KillerPawn)
	{
		if (KillerPawn->AbilitySystemComponent)
			KillerPawn->AbilitySystemComponent->NetMulticast_InvokeGameplayCueExecuted(FGameplayTag(UKismetStringLibrary::Conv_StringToName(L"GameplayCue.Shield.PotionConsumed")), {}, {});

		float Health = KillerPawn->GetHealth();
		float Shield = KillerPawn->GetShield();

		if (Health == 100)
		{
			Shield += Shield + 50;
		}
		else if (Health + 50 > 100)
		{
			Health = 100;
			Shield += (Health + 50) - 100;
		}
		else if (Health + 50 <= 100)
		{
			Health += 50;
		}

		KillerPawn->SetHealth(Health);
		KillerPawn->SetShield(Shield);
	}

	if (KillerPlayerState && KillerPlayerState != PlayerState)
	{
		PlayerController->StateName = UKismetStringLibrary::Conv_StringToName(L"Spectating"); // idk if needed in this season

		KillerPlayerState->KillScore++;
		KillerPlayerState->OnRep_Kills();

		KillerPlayerState->ClientReportKill(PlayerState);

		if (Configuration::bIsProd)
		{
			API::PostAsync("http://45.92.217.51:8080/api/exlo/hype/add", API::Json({
				{"username", KillerPlayerState->GetPlayerName().ToString()}, {"amount",50}
			}));

			API::PostAsync("http://45.92.217.51:8080/api/exlo/vbucks/add", API::Json({
				{"username", KillerPlayerState->GetPlayerName().ToString()}, {"amount",50}
			}));
		}

		if (AFortTeamInfo* PlayerTeam = KillerPlayerState->PlayerTeam)
		{
			for (AController* TeamMember : PlayerTeam->TeamMembers)
			{
				AFortPlayerStateAthena* TeamMemberState = Cast<AFortPlayerStateAthena>(TeamMember->PlayerState);

				if (TeamMemberState == NULL)
					continue;

				TeamMemberState->TeamKillScore++;

				TeamMemberState->OnRep_TeamKillScore();
				TeamMemberState->ClientReportTeamKill(1);
			}
		}
	}

	AFortGameModeAthena* GameMode = Cast<AFortGameModeAthena>(UWorld::GetWorld()->AuthorityGameMode);

	if (!GameState->IsRespawningAllowed(PlayerState))
	{
		PlayerController->ClientSendEndBattleRoyaleMatchForPlayer(true, FAthenaRewardResult());

		FAthenaMatchTeamStats TeamStats{};

		if (PlayerState)
		{
			PlayerState->Place = GameState->PlayersLeft;
			PlayerState->OnRep_Place();
		}

		TeamStats.Place = PlayerState->Place;
		TeamStats.TotalPlayers = GameState->TotalPlayers;

		PlayerController->ClientSendMatchStatsForPlayer(FAthenaMatchStats());
		PlayerController->ClientSendTeamStatsForPlayer(TeamStats);

		GameMode->RemoveFromAlivePlayers(PlayerController, KillerPlayerState && KillerPlayerState == PlayerState ? NULL : KillerPlayerState, (AFortPlayerPawn*)DeathReport.KillerPawn, NULL, PlayerState ? PlayerState->DeathInfo.DeathCause : EDeathCause::Rifle, 0);

		if (KillerPlayerState != NULL)
		{
			if (KillerPlayerState->Place == 1)
			{
				AFortPlayerControllerAthena* KillerController = Cast<AFortPlayerControllerAthena>(KillerPlayerState->GetOwner());

				if (KillerController == NULL)
					return;

				KillerController->ClientSendEndBattleRoyaleMatchForPlayer(true, FAthenaRewardResult());

				KillerController->PlayWinEffects(DeathReport.KillerPawn, NULL, PlayerState->DeathInfo.DeathCause, false);
				KillerController->ClientNotifyWon(DeathReport.KillerPawn, NULL, PlayerState->DeathInfo.DeathCause);
				KillerController->ClientNotifyTeamWon(DeathReport.KillerPawn, NULL, PlayerState->DeathInfo.DeathCause);

				KillerController->ClientSendMatchStatsForPlayer(FAthenaMatchStats());
				KillerController->ClientSendTeamStatsForPlayer(FAthenaMatchTeamStats());

				GameState->WinningPlayerState = KillerPlayerState;
				GameState->WinningTeam = KillerPlayerState->TeamIndex;

				GameState->OnRep_WinningPlayerState();
				GameState->OnRep_WinningTeam();

				if (Configuration::bIsProd)
				{
					API::PostAsync("http://45.92.217.51:8080/api/exlo/hype/add", API::Json({
						{"username", KillerPlayerState->GetPlayerName().ToString()}, {"amount",150}
					}));

					API::PostAsync("http://45.92.217.51:8080/api/exlo/vbucks/add", API::Json({
						{"username", KillerPlayerState->GetPlayerName().ToString()}, {"amount",150}
					}));
				}
			}
		}
	}
}

void Player::ServerAttemptInventoryDrop(AFortPlayerController* PlayerController, FGuid& ItemGuid, int32 Count, bool bTrash)
{
	AFortPlayerPawn* PlayerPawn = PlayerController->MyFortPawn;

	if (PlayerPawn == NULL)
		return;

	FFortItemEntry* ItemEntry = Inventory::GetItemEntry(PlayerController->WorldInventory, ItemGuid);

	if (ItemEntry == NULL || ItemEntry->Count - Count < 0)
		return;

	UFortWorldItem* WorldItem = Inventory::GetWorldItem(PlayerController->WorldInventory, ItemGuid);

	if (WorldItem == NULL)
		return;

	ItemEntry->Count -= Count;

	Inventory::SpawnPickup(PlayerPawn->K2_GetActorLocation() + PlayerPawn->GetActorForwardVector() * 70.f + FVector(0, 0, 50), ItemEntry, Count, PlayerPawn, EFortPickupSourceTypeFlag::Tossed, EFortPickupSpawnSource::Unset);

	if (ItemEntry->Count <= 0 || Count < 0)
	{
		Inventory::RemoveItem(PlayerController->WorldInventory, ItemGuid);
	}
	else
	{ 
		WorldItem->ItemEntry.Count = ItemEntry->Count;

		Inventory::UpdateEntry(PlayerController->WorldInventory, ItemEntry);
	}
}

void Player::ServerAttemptAircraftJump(AFortPlayerController* PlayerController, FRotator& ClientRotation)
{
	Originals::ServerAttemptAircraftJump(PlayerController, ClientRotation);

	if (Configuration::bIsLateGame)
	{
		if (PlayerController->MyFortPawn)
		{
			PlayerController->MyFortPawn->SetShield(100.f);

			LateGame::GivePlayerRandomWeapon(PlayerController);

			if (AFortGameStateAthena* GameState = Cast<AFortGameStateAthena>(UWorld::GetWorld()->GameState))
			{
				if (AFortAthenaAircraft* Aircraft = GameState->GetAircraft(0))
				{
					FVector AircraftLocation = Aircraft->K2_GetActorLocation();

					float Angle = (float)rand() / 5215.03002625f;
					float Radius = (float)(rand() % 1000);

					float OffsetX = cosf(Angle) * Radius;
					float OffsetY = sinf(Angle) * Radius;

					FVector Offset = FVector{};

					Offset.X = OffsetX;
					Offset.Y = OffsetY;
					Offset.Z = 0.0f;

					FVector Location = AircraftLocation + Offset;

					PlayerController->MyFortPawn->K2_SetActorLocation(Location, false, NULL, true);
				}
			}
		}
	}
}

// scuffedd

void Player::ServerReviveFromDBNO(AFortPlayerPawnAthena* PlayerPawn, AController* EventInstigator)
{
	AFortPlayerControllerAthena* PlayerController = Cast<AFortPlayerControllerAthena>(PlayerPawn->Controller);

	if (PlayerController == NULL)
		return;

	AFortPlayerStateAthena* PlayerState = Cast<AFortPlayerStateAthena>(PlayerController->PlayerState);

	if (PlayerState == NULL)
		return;

	for (FGameplayAbilitySpec& AbilitySpec : PlayerState->AbilitySystemComponent->ActivatableAbilities.Items)
	{
		if (AbilitySpec.Ability->IsA(Utils::StaticFindObject<UClass>(TEXT("/Game/Abilities/NPC/Generic/GAB_AthenaDBNO.GAB_AthenaDBNO_C"))))
		{
			PlayerState->AbilitySystemComponent->ServerCancelAbility(AbilitySpec.Handle, AbilitySpec.ActivationInfo);
			PlayerState->AbilitySystemComponent->ServerEndAbility(AbilitySpec.Handle, AbilitySpec.ActivationInfo, AbilitySpec.ActivationInfo.PredictionKeyWhenActivated);
			PlayerState->AbilitySystemComponent->ClientCancelAbility(AbilitySpec.Handle, AbilitySpec.ActivationInfo);
			PlayerState->AbilitySystemComponent->ClientEndAbility(AbilitySpec.Handle, AbilitySpec.ActivationInfo);

			break;
		}
	}

	PlayerPawn->bIsDBNO = false;
	PlayerPawn->OnRep_IsDBNO();

	PlayerPawn->SetHealth(30);

	PlayerController->RespawnPlayerAfterDeath(false);
	PlayerController->ClientOnPawnRevived(EventInstigator);
}

void Player::ServerHandlePickup(AFortPlayerPawn* PlayerPawn, AFortPickup* Pickup, float InFlyTime, FVector& InStartDirection, bool bPlayPickupSound)
{
	if (Pickup != NULL)
		Pickup->SetPickupTarget(PlayerPawn, InFlyTime / PlayerPawn->PickupSpeedMultiplier, InStartDirection, bPlayPickupSound);
}

bool Player::FinishedTargetSpline(AFortPickup* Pickup)
{
	if (Pickup == NULL)
		return Originals::FinishedTargetSpline(Pickup);

	AFortPawn* PickupTarget = Pickup->PickupLocationData.PickupTarget;

	if (PickupTarget == NULL)
		return Originals::FinishedTargetSpline(Pickup);

	AFortPlayerPawnAthena* Pawn = Cast<AFortPlayerPawnAthena>(PickupTarget);

	if (Pawn == NULL)
		return Originals::FinishedTargetSpline(Pickup);

	AFortPlayerControllerAthena* PlayerController = Cast<AFortPlayerControllerAthena>(Pawn->Controller);

	if (PlayerController == NULL)
		return Originals::FinishedTargetSpline(Pickup);

	if (PlayerController->bTryPickupSwap)
	{
		float RandomAngle = UKismetMathLibrary::RandomFloatInRange(-18.f, 18.f);
		float FinalAngle = UKismetMathLibrary::DegreesToRadians(RandomAngle);

		FVector FinalLocation = Pawn->K2_GetActorLocation();
		FVector ForwardVector = Pawn->GetActorForwardVector();

		ForwardVector.Z = 0.0f;
		ForwardVector.Normalize();

		FinalLocation = FinalLocation + ForwardVector * 450.f;
		FinalLocation.Z += 50.f;
		FinalLocation.X += std::cos(FinalAngle) * 100.f;
		FinalLocation.Y += std::sin(FinalAngle) * 100.f;

		if (Inventory::GetQuickbar(Pawn->CurrentWeapon->WeaponData) == EFortQuickBars::Primary && Inventory::GetQuickbar((UFortItemDefinition*)Pickup->PrimaryPickupItemEntry.ItemDefinition) == EFortQuickBars::Primary)
		{
			PlayerController->bTryPickupSwap = false;

			FFortItemEntry* ItemEntry = Inventory::GetItemEntry(PlayerController->WorldInventory, Pawn->CurrentWeapon->ItemEntryGuid);

			if (ItemEntry != NULL)
			{
				Inventory::SpawnPickup(PlayerController->GetViewTarget()->K2_GetActorLocation(), ItemEntry, ItemEntry->Count, Pawn, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset);
				Inventory::RemoveItem(PlayerController->WorldInventory, ItemEntry->ItemGuid);

				if (UFortWorldItem* Item = Inventory::CreateItem(PlayerController->WorldInventory, (UFortItemDefinition*)Pickup->PrimaryPickupItemEntry.ItemDefinition, Pickup->PrimaryPickupItemEntry.Count, Pickup->PrimaryPickupItemEntry.LoadedAmmo))
				{
					PlayerController->ServerExecuteInventoryItem(Item->ItemEntry.ItemGuid);
				}
			}
		}
		else
		{
			Inventory::SpawnPickup(PlayerController->GetViewTarget()->K2_GetActorLocation(), &Pickup->PrimaryPickupItemEntry, Pickup->PrimaryPickupItemEntry.Count, Pawn, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset);
		}
	}
	else
	{
		InternalPickup(PlayerController, Pickup->PrimaryPickupItemEntry);
	}

	return Originals::FinishedTargetSpline(Pickup);
}

void Player::InternalPickup(AFortPlayerControllerAthena* PlayerController, FFortItemEntry& PickupEntry)
{
	if (PlayerController == NULL)
		return;

	int32 MaxStackSize = PickupEntry.ItemDefinition->MaxStackSize;
	int32 NumberOfSlotsToTake = 0;

	for (FFortItemEntry& ReplicatedEntry : PlayerController->WorldInventory->Inventory.ReplicatedEntries)
	{
		if (Inventory::GetQuickbar(ReplicatedEntry.ItemDefinition) == EFortQuickBars::Primary)
			NumberOfSlotsToTake += 1;
	}

	if (Configuration::bIsLateGame)
	{
		if (PickupEntry.ItemDefinition->IsA(UFortResourceItemDefinition::StaticClass()))
			MaxStackSize = 500;
	}

	auto GiveOrSwap = [&]() {
		if (NumberOfSlotsToTake >= 5 && Inventory::GetQuickbar((UFortItemDefinition*)PickupEntry.ItemDefinition) == EFortQuickBars::Primary)
		{
			if (Inventory::GetQuickbar(PlayerController->MyFortPawn->CurrentWeapon->WeaponData) == EFortQuickBars::Primary) 
			{
				FFortItemEntry* ItemEntry = Inventory::GetItemEntry(PlayerController->WorldInventory, PlayerController->MyFortPawn->CurrentWeapon->ItemEntryGuid);

				if (ItemEntry != NULL)
				{
					Inventory::SpawnPickup(PlayerController->GetViewTarget()->K2_GetActorLocation(), ItemEntry, ItemEntry->Count, (AFortPlayerPawnAthena*)PlayerController->Pawn, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset);

					Inventory::RemoveItem(PlayerController->WorldInventory, PlayerController->MyFortPawn->CurrentWeapon->ItemEntryGuid);
					Inventory::CreateItem(PlayerController->WorldInventory, (UFortItemDefinition*)PickupEntry.ItemDefinition, PickupEntry.Count, PickupEntry.LoadedAmmo);
				}
			}
			else {
				Inventory::SpawnPickup(PlayerController->GetViewTarget()->K2_GetActorLocation(), &PickupEntry, PickupEntry.Count, (AFortPlayerPawnAthena*)PlayerController->Pawn, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset);
			}
		}
		else
		{
			Inventory::CreateItem(PlayerController->WorldInventory, (UFortItemDefinition*)PickupEntry.ItemDefinition, PickupEntry.Count, PickupEntry.LoadedAmmo);
		}
		};

	auto GiveOrSwapStack = [&](int32 OriginalCount) 
		{
		if (((UFortItemDefinition*)PickupEntry.ItemDefinition)->bAllowMultipleStacks && NumberOfSlotsToTake < 5)
		{
			Inventory::CreateItem(PlayerController->WorldInventory, (UFortItemDefinition*)PickupEntry.ItemDefinition, OriginalCount - MaxStackSize, PickupEntry.LoadedAmmo);
		}
		else
		{
			Inventory::SpawnPickup(PlayerController->GetViewTarget()->K2_GetActorLocation(), &PickupEntry, OriginalCount - MaxStackSize, (AFortPlayerPawnAthena*)PlayerController->Pawn, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset);
		}
		};

	if (((UFortItemDefinition*)PickupEntry.ItemDefinition)->IsStackable()) 
	{
		FFortItemEntry* ItemEntry = NULL;

		for (FFortItemEntry& ReplicatedEntry : PlayerController->WorldInventory->Inventory.ReplicatedEntries)
		{
			if (ReplicatedEntry.ItemDefinition == PickupEntry.ItemDefinition && ReplicatedEntry.Count < MaxStackSize)
			{
				ItemEntry = &ReplicatedEntry;
				break;
			}
		}

		if (ItemEntry != NULL)
		{
			if ((ItemEntry->Count += PickupEntry.Count) > MaxStackSize)
			{
				int32 OriginalCount = ItemEntry->Count;
				ItemEntry->Count = MaxStackSize;

				GiveOrSwapStack(OriginalCount);
			}

			Inventory::UpdateEntry(PlayerController->WorldInventory, ItemEntry);
		}
		else {
			if (PickupEntry.Count > MaxStackSize)
			{
				int32 OriginalCount = PickupEntry.Count;
				PickupEntry.Count = MaxStackSize;

				GiveOrSwapStack(OriginalCount);
			}

			GiveOrSwap();
		}
	}
	else {
		GiveOrSwap();
	}
}

void Player::MovingEmoteStopped(AFortPawn* Pawn, FFrame& Stack)
{
	Stack.IncrementCode();

	Pawn->bMovingEmote = false;
	Pawn->bMovingEmoteForwardOnly = false;
}

void Player::ServerClientIsReadyToRespawn(AFortPlayerControllerAthena* PlayerController)
{
	AFortPlayerStateAthena* PlayerState = Cast<AFortPlayerStateAthena>(PlayerController->PlayerState);

	if (PlayerState->RespawnData.bRespawnDataAvailable && PlayerState->RespawnData.bServerIsReady)
	{
		FVector Location = FVector{};

		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(UWorld::GetWorld(), AFortPlayerStartWarmup::StaticClass(), &PlayerStarts);

		if (PlayerStarts.IsValid())
		{
			Location = PlayerStarts[UKismetMathLibrary::RandomIntegerInRange(0, PlayerStarts.Num() - 1)]->GetTransform().Translation;
		}

		if (!Location.IsZero())
			PlayerState->RespawnData.RespawnLocation = Location;
	}

	return Originals::ServerClientIsReadyToRespawn(PlayerController);
}

void Player::Setup()
{
	Utils::Virtual<AFortPlayerControllerAthena>(0x840 / 8, ServerAcknowledgePossession);
	Utils::Virtual<AFortPlayerControllerAthena>(0xFE8 / 8, ServerExecuteInventoryItem);
	Utils::Virtual<AFortPlayerControllerAthena>(0xDE0 / 8, ServerPlayEmoteItem);

	Utils::Virtual<AFortPlayerControllerAthena>(0x23B0 / 8, ServerClientIsReadyToRespawn, (void**)&Originals::ServerClientIsReadyToRespawn);

	Utils::Virtual<AFortPlayerControllerAthena>(0x1080 / 8, ServerAttemptInventoryDrop);
	Utils::Virtual<AFortPlayerControllerAthena>(0x1420 / 8, ServerAttemptAircraftJump, (void**)&Originals::ServerAttemptAircraftJump);

	Utils::Virtual<AFortPlayerPawnAthena>(0xE40 / 8, ServerSendZiplineState);
	Utils::Virtual<AFortPlayerPawnAthena>(0xD48 / 8, ServerReviveFromDBNO);

	Utils::Virtual<AFortPlayerPawnAthena>(0xDE8 / 8, ServerHandlePickup);

	Utils::Exec(TEXT("/Script/FortniteGame.FortPawn.MovingEmoteStopped"), MovingEmoteStopped);

	Utils::Hook(InSDKUtils::GetImageBase() + 0x18172A0, GetPlayerViewPoint, (void**)&Originals::GetPlayerViewPoint);
	Utils::Hook(InSDKUtils::GetImageBase() + 0x1C61050, ClientOnPawnDied, (void**)&Originals::ClientOnPawnDied);
	Utils::Hook(InSDKUtils::GetImageBase() + 0x15772D0, FinishedTargetSpline, (void**)&Originals::FinishedTargetSpline);

	Utils::Rel32(InSDKUtils::GetImageBase() + 0x109CE55, RemoveAllInventoryItems);
}