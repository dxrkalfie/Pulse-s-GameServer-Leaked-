#include "pch.h"
#include "LateGame.h"
#include "Utils.h"
#include "Inventory.h"

struct LateGameWeapons
{
    std::vector<UFortItemDefinition*> RifleOptions;
    std::vector<UFortItemDefinition*> PumpWeapons;

    std::vector<std::pair<UFortItemDefinition*, int32>> Movement;
    std::vector<std::pair<UFortItemDefinition*, int32>> Meds;
};

LateGameWeapons Weapons = LateGameWeapons{};

void LateGame::InitializeLategameLoadouts()
{
    Weapons.PumpWeapons.push_back(Utils::StaticFindObject<UFortWeaponRangedItemDefinition>(TEXT("/Game/Athena/Items/Weapons/WID_Shotgun_Standard_Athena_SR_Ore_T03.WID_Shotgun_Standard_Athena_SR_Ore_T03")));
    Weapons.PumpWeapons.push_back(Utils::StaticFindObject<UFortWeaponRangedItemDefinition>(TEXT("/Game/Athena/Items/Weapons/WID_Shotgun_Standard_Athena_VR_Ore_T03.WID_Shotgun_Standard_Athena_VR_Ore_T03")));

    Weapons.PumpWeapons.push_back(Utils::StaticFindObject<UFortWeaponRangedItemDefinition>(TEXT("/Game/Athena/Items/Weapons/WID_Shotgun_Combat_Athena_VR_Ore_T03.WID_Shotgun_Combat_Athena_VR_Ore_T03")));
    Weapons.PumpWeapons.push_back(Utils::StaticFindObject<UFortWeaponRangedItemDefinition>(TEXT("/Game/Athena/Items/Weapons/WID_Shotgun_Combat_Athena_SR_Ore_T03.WID_Shotgun_Combat_Athena_SR_Ore_T03")));

    Weapons.RifleOptions.push_back(Utils::StaticFindObject<UFortWeaponRangedItemDefinition>(TEXT("/Game/Athena/Items/Weapons/WID_Assault_AutoHigh_Athena_VR_Ore_T03.WID_Assault_AutoHigh_Athena_VR_Ore_T03")));
    Weapons.RifleOptions.push_back(Utils::StaticFindObject<UFortWeaponRangedItemDefinition>(TEXT("/Game/Athena/Items/Weapons/WID_Assault_AutoHigh_Athena_SR_Ore_T03.WID_Assault_AutoHigh_Athena_SR_Ore_T03")));
    Weapons.RifleOptions.push_back(Utils::StaticFindObject<UFortWeaponRangedItemDefinition>(TEXT("/Game/Athena/Items/Weapons/WID_Assault_AutoDrum_Athena_R_Ore_T03.WID_Assault_AutoDrum_Athena_R_Ore_T03")));

    Weapons.Meds.push_back({ Utils::StaticFindObject<UFortWeaponRangedItemDefinition>(TEXT("/Game/Athena/Items/Consumables/ShieldSmall/Athena_ShieldSmall.Athena_ShieldSmall")), 3 });
    Weapons.Meds.push_back({ Utils::StaticFindObject<UFortWeaponRangedItemDefinition>(TEXT("/Game/Athena/Items/Consumables/PurpleStuff/Athena_PurpleStuff.Athena_PurpleStuff")), 2 });
    Weapons.Meds.push_back({ Utils::StaticFindObject<UFortWeaponRangedItemDefinition>(TEXT("/Game/Athena/Items/Consumables/Medkit/Athena_Medkit.Athena_Medkit")), 2 });
    Weapons.Meds.push_back({ Utils::StaticFindObject<UFortWeaponRangedItemDefinition>(TEXT("/Game/Athena/Items/Consumables/Shields/Athena_Shields.Athena_Shields")), 3 });

    Weapons.Movement.push_back({ Utils::StaticFindObject<UFortWeaponRangedItemDefinition>(TEXT("/Game/Athena/Items/Consumables/ShockwaveGrenade/Athena_ShockGrenade.Athena_ShockGrenade")), 3 });
    Weapons.Movement.push_back({ Utils::StaticFindObject<UFortWeaponRangedItemDefinition>(TEXT("/Game/Athena/Items/Weapons/WID_Hook_Gun_VR_Ore_T03.WID_Hook_Gun_VR_Ore_T03")), 1 });
    Weapons.Movement.push_back({ Utils::StaticFindObject<UFortWeaponRangedItemDefinition>(TEXT("/Game/Athena/Items/Weapons/WID_Pistol_HandCannon_Athena_SR_Ore_T03.WID_Pistol_HandCannon_Athena_SR_Ore_T03")), 1 });
}

UFortAmmoItemDefinition* LateGame::GetAmmo(EAmmoType AmmoType)
{
    static std::vector<UFortAmmoItemDefinition*> Ammos
    {
        Utils::StaticFindObject<UFortAmmoItemDefinition>(TEXT("/Game/Athena/Items/Ammo/AthenaAmmoDataBulletsLight.AthenaAmmoDataBulletsLight")),
        Utils::StaticFindObject<UFortAmmoItemDefinition>(TEXT("/Game/Athena/Items/Ammo/AthenaAmmoDataShells.AthenaAmmoDataShells")),
        Utils::StaticFindObject<UFortAmmoItemDefinition>(TEXT("/Game/Athena/Items/Ammo/AthenaAmmoDataBulletsMedium.AthenaAmmoDataBulletsMedium")),
        Utils::StaticFindObject<UFortAmmoItemDefinition>(TEXT("/Game/Athena/Items/Ammo/AmmoDataRockets.AmmoDataRockets")),
        Utils::StaticFindObject<UFortAmmoItemDefinition>(TEXT("/Game/Athena/Items/Ammo/AthenaAmmoDataBulletsHeavy.AthenaAmmoDataBulletsHeavy"))
    };

    return Ammos[(uint8)AmmoType];
}

UFortResourceItemDefinition* LateGame::GetResource(EFortResourceType ResourceType)
{
    static std::vector<UFortResourceItemDefinition*> Resources
    {
        Utils::StaticFindObject<UFortResourceItemDefinition>(TEXT("/Game/Items/ResourcePickups/WoodItemData.WoodItemData")),
        Utils::StaticFindObject<UFortResourceItemDefinition>(TEXT("/Game/Items/ResourcePickups/StoneItemData.StoneItemData")),
        Utils::StaticFindObject<UFortResourceItemDefinition>(TEXT("/Game/Items/ResourcePickups/MetalItemData.MetalItemData"))
    };

    return Resources[(uint8)ResourceType];
}

void LateGame::GivePlayerRandomWeapon(AFortPlayerController* PlayerController)
{
    int32 Index = UKismetMathLibrary::RandomIntegerInRange(0, Weapons.PumpWeapons.size() - 1);

    Inventory::CreateItem(PlayerController->WorldInventory, Weapons.PumpWeapons[Index], 1, Inventory::GetStats((UFortWeaponItemDefinition*)Weapons.PumpWeapons[Index])->ClipSize);

    Index = UKismetMathLibrary::RandomIntegerInRange(0, Weapons.RifleOptions.size() - 1);

    Inventory::CreateItem(PlayerController->WorldInventory, Weapons.RifleOptions[Index], 1, Inventory::GetStats((UFortWeaponItemDefinition*)Weapons.RifleOptions[Index])->ClipSize);

    Index = UKismetMathLibrary::RandomIntegerInRange(0, Weapons.Meds.size() - 1);

    Inventory::CreateItem(PlayerController->WorldInventory, Weapons.Meds[Index].first, Weapons.Meds[Index].second);

    Index = UKismetMathLibrary::RandomIntegerInRange(0, Weapons.Meds.size() - 1);

    Inventory::CreateItem(PlayerController->WorldInventory, Weapons.Meds[Index].first, Weapons.Meds[Index].second);

    Index = UKismetMathLibrary::RandomIntegerInRange(0, Weapons.Movement.size() - 1);

    // im gay

    if (Weapons.Movement[Index].first->GetFullName().contains("WID_Hook_Gun_VR_Ore_T03"))
    {
        Inventory::CreateItem(PlayerController->WorldInventory, Weapons.Movement[Index].first, Weapons.Movement[Index].second, 10);
    }
    else if (Weapons.Movement[Index].first->GetFullName().contains("WID_Pistol_HandCannon_Athena_SR_Ore_T03"))
        Inventory::CreateItem(PlayerController->WorldInventory, Weapons.Movement[Index].first, Weapons.Movement[Index].second, Inventory::GetStats((UFortWeaponItemDefinition*)Weapons.Movement[Index].first)->ClipSize);
    else
        Inventory::CreateItem(PlayerController->WorldInventory, Weapons.Movement[Index].first, Weapons.Movement[Index].second);

    Inventory::CreateItem(PlayerController->WorldInventory, GetResource(EFortResourceType::Wood), 500);
    Inventory::CreateItem(PlayerController->WorldInventory, GetResource(EFortResourceType::Stone), 500);
    Inventory::CreateItem(PlayerController->WorldInventory, GetResource(EFortResourceType::Metal), 500);

    Inventory::CreateItem(PlayerController->WorldInventory, GetAmmo(EAmmoType::Assault), 250);
    Inventory::CreateItem(PlayerController->WorldInventory, GetAmmo(EAmmoType::Shotgun), 100);
    Inventory::CreateItem(PlayerController->WorldInventory, GetAmmo(EAmmoType::Submachine), 400);
    Inventory::CreateItem(PlayerController->WorldInventory, GetAmmo(EAmmoType::Sniper), 20);
}