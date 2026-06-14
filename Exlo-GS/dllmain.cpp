#include "pch.h"
#include "Misc.h"
#include "GameMode.h"
#include "Player.h"
#include "Abilities.h"
#include "Building.h"
#include "Inventory.h"
#include "Configuration.h"

DWORD WINAPI LaunchWindowsStartup(LPVOID)
{
    Sleep(5000);
    AllocConsole();
    SetConsoleTitleA("Exlo | Loading");

    FILE* F = NULL;

    freopen_s(&F, "CONIN$", "r", stdin);
    freopen_s(&F, "CONOUT$", "w", stdout);
    freopen_s(&F, "CONOUT$", "w", stderr);

    MH_Initialize();

    Misc::Setup();
    Abilities::Setup();
    Building::Setup();
    GameMode::Setup();
    Player::Setup();
    Inventory::Setup();

    *(bool*)(InSDKUtils::GetImageBase() + 0x5BE45F3) = false;
    *(bool*)(InSDKUtils::GetImageBase() + 0x5BE45F3 + 1) = true;

    UWorld::GetWorld()->OwningGameInstance->LocalPlayers.Remove(0);
    UGameplayStatics::OpenLevel(UWorld::GetWorld(), Configuration::bIs1V1Map ? UKismetStringLibrary::Conv_StringToName(L"/Game/Jett/Maps/1V1") : UKismetStringLibrary::Conv_StringToName(L"Athena_Terrain"), false, FString());

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
        CreateThread(NULL, 0, LaunchWindowsStartup, NULL, 0, NULL);

    return TRUE;
}