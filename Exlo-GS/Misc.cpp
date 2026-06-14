#include "pch.h"
#include "Misc.h"
#include "Utils.h"
#include "Configuration.h"
#include "API.h"

void Misc::TickFlush(UNetDriver* NetDriver, float DeltaSeconds)
{
	if (UReplicationDriver* ReplicationDriver = NetDriver->ReplicationDriver)
	{
		ReplicationDriver->ServerReplicateActors(DeltaSeconds);
	}

	if (Configuration::bIs1V1Map)
	{
		static uint64 LastTime = 0;
		uint64 CurrentTime = GetTickCount64();

		if (CurrentTime - LastTime >= 300000)
		{
			LastTime = CurrentTime;

			TArray<AActor*> Buildings;

			UGameplayStatics::GetAllActorsOfClass(UWorld::GetWorld(), ABuildingSMActor::StaticClass(), &Buildings);

			for (AActor* _BuildingSMActor : Buildings)
			{
				if (ABuildingSMActor* BuildingSMActor = Cast<ABuildingSMActor>(_BuildingSMActor))
				{
					if (BuildingSMActor->bPlayerPlaced)
						BuildingSMActor->K2_DestroyActor();
				}
			}
		}
	}

	if (Configuration::bIsProd)
	{
		if (AFortGameModeAthena* GameMode = Cast<AFortGameModeAthena>(UWorld::GetWorld()->AuthorityGameMode))
		{
			if (AFortGameStateAthena* GameState = Cast<AFortGameStateAthena>(UWorld::GetWorld()->GameState))
			{
				if (GameMode->AlivePlayers.Num() <= 1 && GameState->GamePhase >= EAthenaGamePhase::Aircraft && !Configuration::bIsRestarting)
				{
					Configuration::bIsRestarting = true;

					std::thread([]() {
						std::this_thread::sleep_for(std::chrono::seconds(20));
						TerminateProcess(GetCurrentProcess(), 0);
					}).detach();
				}
			}
		}
	}
	else
	{
		if (GetKeyState(VK_F4))
		{
			UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), L"startaircraft", NULL);
		}
	}

	return Originals::TickFlush(NetDriver, DeltaSeconds);
}

void Misc::SendRequestNow(UMcpProfileGroup* McpProfileGroup, const void* HttpRequest, FBaseUrlContext::EContextCredentials ContextCredentials)
{
	return Originals::SendRequestNow(McpProfileGroup, HttpRequest, FBaseUrlContext::EContextCredentials::CXC_Public);
}

bool Misc::StartAircraftPhase(AFortGameModeAthena* GameMode, char a2)
{
	bool Ret = Originals::StartAircraftPhase(GameMode, a2);

	if (Configuration::bIs1V1Map)
		return Ret;

	if (Configuration::bIsLateGame && !Configuration::bIsGameStarted)
	{
		Configuration::bIsGameStarted = true;

		if (AFortGameStateAthena* GameState = Cast<AFortGameStateAthena>(GameMode->GameState))
		{
			GameState->DefaultParachuteDeployTraceForGroundDistance = 1000.f;

			if (AFortAthenaAircraft* Aircraft = GameState->Aircrafts[0])
			{
				FVector Location = GameMode->SafeZoneLocations[4];
				Location.Z = 17500.f;

				Aircraft->FlightInfo.FlightSpeed = 1000.f;
				Aircraft->FlightInfo.FlightStartLocation = FVector_NetQuantize100(Location);

				Aircraft->FlightInfo.TimeTillFlightEnd = 9;
				Aircraft->FlightInfo.TimeTillDropEnd = 9;
				Aircraft->FlightInfo.TimeTillDropStart = 1;

				Aircraft->DropStartTime = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld()) + 1;
				Aircraft->DropEndTime = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld()) + 9;

				GameState->bAircraftIsLocked = false;
				GameState->SafeZonesStartTime = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
			}
		}
	}

	return Ret;
}

void Misc::StartNewSafeZonePhase(AFortGameModeAthena* GameMode, int SafeZonePhase)
{
	Originals::StartNewSafeZonePhase(GameMode, SafeZonePhase);

	AFortGameStateAthena* GameState = Cast<AFortGameStateAthena>(UWorld::GetWorld()->GameState);

	if (GameState == NULL)
		return;

	if (Configuration::bIsLateGame && GameMode->SafeZonePhase < 4)
	{
		GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime = UGameplayStatics::GetTimeSeconds(GameState);
		GameMode->SafeZoneIndicator->SafeZoneFinishShrinkTime = GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime + 0.15f;
	}
}

void Misc::ServerUpdatePhysicsParams(AFortPhysicsPawn* PhysicsPawn, FReplicatedPhysicsPawnState& InState)
{
	PhysicsPawn->ReplicatedMovement.AngularVelocity = InState.AngularVelocity;
	PhysicsPawn->ReplicatedMovement.LinearVelocity = InState.LinearVelocity;

	PhysicsPawn->ReplicatedMovement.Location = InState.Translation;
	PhysicsPawn->OnRep_ReplicatedMovement();

	if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(PhysicsPawn->RootComponent))
	{
		PrimitiveComponent->K2_SetWorldLocationAndRotation(InState.Translation, FRotator(), false, NULL, true);
		PrimitiveComponent->SetPhysicsLinearVelocity(InState.LinearVelocity, false, FName());
		PrimitiveComponent->SetPhysicsAngularVelocity(InState.LinearVelocity, false, FName());

		PrimitiveComponent->bComponentToWorldUpdated = true;
	}
}

void Misc::SetDynamicFoundationEnabled(ABuildingFoundation* BuildingFoundation, FFrame& Stack)
{
	bool bEnabled;

	Stack.StepCompiledIn(&bEnabled);
	Stack.IncrementCode();

	EDynamicFoundationEnabledState EnabledState = bEnabled ? EDynamicFoundationEnabledState::Enabled : EDynamicFoundationEnabledState::Disabled;

	BuildingFoundation->FoundationEnabledState = EnabledState;
}

void Misc::SetDynamicFoundationTransform(ABuildingFoundation* BuildingFoundation, FFrame& Stack)
{
	FTransform NewTransform;

	Stack.StepCompiledIn(&NewTransform);
	Stack.IncrementCode();

	BuildingFoundation->DynamicFoundationTransform = NewTransform;
	BuildingFoundation->StreamingData.FoundationLocation = NewTransform.Translation;
	BuildingFoundation->StreamingData.BoundingBox = BuildingFoundation->StreamingBoundingBox;
}

float Misc::GetMaxTickRate()
{
	return Configuration::bIsLateGame ? 120.0f : 30.0f;
}

bool Misc::Listen()
{
	FName NetDriverDefinition = UKismetStringLibrary::Conv_StringToName(L"GameNetDriver");
	UNetDriver* NetDriver = UEngine::GetEngine()->CreateNetDriver(UWorld::GetWorld(), NetDriverDefinition);

	NetDriver->World = UWorld::GetWorld();
	NetDriver->NetDriverName = NetDriverDefinition;

	FString Error;

	FURL InURL;
	InURL.Port = 7778;

	if (!NetDriver->InitListen(UWorld::GetWorld(), InURL, false, Error))
		return false;

	NetDriver->SetWorld(UWorld::GetWorld());

	UWorld::GetWorld()->NetDriver = NetDriver;

	for (FLevelCollection& LevelCollection : UWorld::GetWorld()->LevelCollections)
	{
		LevelCollection.NetDriver = UWorld::GetWorld()->NetDriver;
	}

	return true;
}

static int ForceReturnTrue()
{
	return 1;
}

void Misc::Setup()
{
	Utils::Exec(TEXT("/Script/FortniteGame.BuildingFoundation.SetDynamicFoundationTransform"), SetDynamicFoundationTransform);
	Utils::Exec(TEXT("/Script/FortniteGame.BuildingFoundation.SetDynamicFoundationEnabled"), SetDynamicFoundationEnabled);

	Utils::Hook(InSDKUtils::GetImageBase() + 0x2ED22B0, TickFlush, (void**)&Originals::TickFlush);
	Utils::Hook(InSDKUtils::GetImageBase() + 0xB10DE0, SendRequestNow, (void**)&Originals::SendRequestNow);
	Utils::Hook(InSDKUtils::GetImageBase() + 0x109D9D0, StartNewSafeZonePhase, (void**)&Originals::StartNewSafeZonePhase);

	Utils::Hook(InSDKUtils::GetImageBase() + 0x109C520, StartAircraftPhase, (void**)&Originals::StartAircraftPhase);

	Utils::Hook(InSDKUtils::GetImageBase() + 0x315D2C0, GetMaxTickRate);

	Utils::Hook(InSDKUtils::GetImageBase() + 0x2D9AE90, ForceReturnTrue);
	Utils::Hook(InSDKUtils::GetImageBase() + 0x890540, ForceReturnTrue);

	Utils::Hook(InSDKUtils::GetImageBase() + 0x31AC180, ForceReturnTrue); // WorldNetMode
	Utils::Hook(InSDKUtils::GetImageBase() + 0x659929A, ForceReturnTrue); // CollectGarbage

	//Utils::Hook(InSDKUtils::GetImageBase() + 0x1C33940, ServerUpdatePhysicsParams);

	Utils::Patch(InSDKUtils::GetImageBase() + 0x29A2100, 0xC3);
	Utils::Patch(InSDKUtils::GetImageBase() + 0x1403040, 0xC3);
	Utils::Patch(InSDKUtils::GetImageBase() + 0x1F84D30, 0xC3);

	Utils::Patch(InSDKUtils::GetImageBase() + 0x10D9CCC, 0x85);
	Utils::Patch(InSDKUtils::GetImageBase() + 0x31AEBCA, 0x74);

	Utils::Rel32(InSDKUtils::GetImageBase() + 0x3164856, Listen);
}