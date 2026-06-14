#include "pch.h"
#include "GameMode.h"
#include "Utils.h"
#include "LateGame.h"
#include "Configuration.h"
#include "API.h"

bool GameMode::ReadyToStartMatch(AFortGameModeAthena* GameMode)
{
	AFortGameStateAthena* GameState = Cast<AFortGameStateAthena>(GameMode->GameState);

	if (!Configuration::bIs1V1Map && GameState->MapInfo == NULL)
		return false;

	if (!GameMode->bWorldIsReady)
	{
		UFortPlaylistAthena* Playlist = Utils::StaticFindObject<UFortPlaylistAthena>(Configuration::bIs1V1Map ? TEXT("Playlist_1V1") : TEXT("Playlist_ShowdownAlt_Solo"), ANY_PACKAGE);

		if (Configuration::bIs1V1Map)
		{
			Playlist->RespawnType = EAthenaRespawnType::InfiniteRespawn;
			Playlist->RespawnTime = FScalableFloat(3.f);
		}

		GameState->CurrentPlaylistId = GameMode->CurrentPlaylistId;

		GameState->CurrentPlaylistInfo.BasePlaylist = Playlist;
		GameState->CurrentPlaylistInfo.PlaylistReplicationKey++;

		GameState->CurrentPlaylistInfo.MarkArrayDirty();

		GameState->OnRep_CurrentPlaylistId();
		GameState->OnRep_CurrentPlaylistInfo();

		GameMode->CurrentPlaylistName = Playlist->PlaylistName;
		GameMode->CurrentPlaylistId = Playlist->PlaylistId;

		GameMode->WarmupRequiredPlayerCount = 1;
		GameMode->GameSession->MaxPlayers = Playlist->MaxPlayers;

		GameState->AirCraftBehavior = Playlist->AirCraftBehavior;
		GameState->CachedSafeZoneStartUp = Playlist->SafeZoneStartUp;

		LateGame::InitializeLategameLoadouts();

		for (TSoftObjectPtr<UWorld>& AdditionalLevel : Playlist->AdditionalLevels)
		{
			bool bSuccess = false;
			ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr(UWorld::GetWorld(), AdditionalLevel, FVector(), FRotator(), &bSuccess);

			if (bSuccess)
				GameState->AdditionalPlaylistLevelsStreamed.Add(AdditionalLevel.ObjectID.AssetPathName);
		}

		GameState->OnRep_AdditionalPlaylistLevelsStreamed();
		GameState->OnFinishedStreamingAdditionalPlaylistLevel();

		Utils::ShowFoundation(Utils::StaticLoadObject<ABuildingFoundation>(TEXT("/Game/Athena/Maps/Athena_POI_Foundations.Athena_POI_Foundations.PersistentLevel.LF_Athena_POI_50x53_Volcano")));
		Utils::ShowFoundation(Utils::StaticLoadObject<ABuildingFoundation>(TEXT("/Game/Athena/Maps/Athena_POI_Foundations.Athena_POI_Foundations.PersistentLevel.SLAB_2")));

		SetConsoleTitleA("Exlo | Joinable");

		GameMode->bWorldIsReady = true;
	}

	return GameMode->bDelayedStart == 0 && GameMode->MatchState == UKismetStringLibrary::Conv_StringToName(TEXT("WaitingToStart")) && GameMode->NumPlayers + GameMode->NumBots > 0;
}

void GameMode::HandleStartingNewPlayer(AGameModeBase* GameMode, APlayerController* NewPlayer)
{
	Originals::HandleStartingNewPlayer(GameMode, NewPlayer);

	AFortPlayerStateAthena* PlayerState = Cast<AFortPlayerStateAthena>(NewPlayer->PlayerState);

	if (PlayerState == NULL)
		return;

	AFortGameStateAthena* GameState = Cast<AFortGameStateAthena>(GameMode->GameState);

	if (GameState == NULL)
		return;

	PlayerState->SquadId = PlayerState->TeamIndex - 3;
	PlayerState->OnRep_SquadId();

	PlayerState->WorldPlayerId = PlayerState->PlayerID;

	FGameMemberInfo GameMemberInfo = FGameMemberInfo{};

	GameMemberInfo.ReplicationID = -1;
	GameMemberInfo.ReplicationKey = -1;
	GameMemberInfo.MostRecentArrayReplicationKey = -1;

	GameMemberInfo.SquadId = PlayerState->SquadId;
	GameMemberInfo.TeamIndex = PlayerState->TeamIndex;
	GameMemberInfo.MemberUniqueId = PlayerState->UniqueId;

	GameState->GameMemberInfoArray.Members.Add(GameMemberInfo);
	GameState->GameMemberInfoArray.MarkArrayDirty();

	GameMemberInfo.PostReplicatedAdd(&GameState->GameMemberInfoArray);

	if (AFortPlayerControllerAthena* PlayerController = Cast<AFortPlayerControllerAthena>(NewPlayer))
	{
		if (PlayerController->MatchReport == NULL)
			PlayerController->MatchReport = Cast<UAthenaPlayerMatchReport>(UGameplayStatics::SpawnObject(UAthenaPlayerMatchReport::StaticClass(), PlayerController));
	}
}

APawn* GameMode::SpawnDefaultPawnFor(AGameModeBase* GameMode, AController* NewPlayer, AActor* StartSpot)
{
	return GameMode->SpawnDefaultPawnAtTransform(NewPlayer, StartSpot->GetTransform());
}

EFortTeam GameMode::PickTeam(AFortGameModeAthena* GameMode, uint8 PreferredTeam, AFortPlayerControllerAthena* ControllerToPickFor)
{
	static uint8_t Team = 3;
	static uint8_t PlayersOnTeam = 0;

	uint8_t ChosenTeam = Team;

	if (AFortGameStateAthena* GameState = Cast<AFortGameStateAthena>(GameMode->GameState))
	{
		UFortPlaylistAthena* Playlist = GameState->CurrentPlaylistInfo.BasePlaylist;

		if (Playlist == NULL)
			return EFortTeam::MAX;

		if (++PlayersOnTeam >= Playlist->MaxSquadSize)
		{
			Team++;
			PlayersOnTeam = 0;
		}
	}

	return EFortTeam(ChosenTeam);
}

void GameMode::OnAircraftExitedDropZone(AFortGameModeAthena* GameMode, AFortAthenaAircraft* FortAthenaAircraft)
{
	AFortGameStateAthena* GameState = Cast<AFortGameStateAthena>(GameMode->GameState);

	if (GameState == NULL)
		return Originals::OnAircraftExitedDropZone(GameMode, FortAthenaAircraft);

	for (AFortPlayerControllerAthena* PlayerController : GameMode->AlivePlayers)
	{
		if (PlayerController->IsInAircraft())
			PlayerController->ServerAttemptAircraftJump(FRotator());
	}

	if (Configuration::bIsLateGame)
	{ 
		GameState->GamePhase = EAthenaGamePhase::SafeZones;
		GameState->GamePhaseStep = EAthenaGamePhaseStep::StormHolding;

		GameState->OnRep_GamePhase(EAthenaGamePhase::Aircraft);
	}

	return Originals::OnAircraftExitedDropZone(GameMode, FortAthenaAircraft);
}

void GameMode::Setup()
{
	Utils::Hook(InSDKUtils::GetImageBase() + 0x108ADA0, OnAircraftExitedDropZone, (void**)&Originals::OnAircraftExitedDropZone);
	Utils::Hook(InSDKUtils::GetImageBase() + 0x108DC10, PickTeam);

	Utils::Virtual<AFortGameModeAthena>(0x618 / 8, SpawnDefaultPawnFor);
	Utils::Virtual<AFortGameModeAthena>(0x7E0 / 8, ReadyToStartMatch);
	Utils::Virtual<AFortGameModeAthena>(0x648 / 8, HandleStartingNewPlayer, (void**)&Originals::HandleStartingNewPlayer);
}