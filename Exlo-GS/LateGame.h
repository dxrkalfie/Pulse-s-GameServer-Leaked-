#pragma once
#include "pch.h"

enum class EAmmoType : uint8
{
    Assault = 0,
    Shotgun = 1,
    Submachine = 2,
    Rocket = 3,
    Sniper = 4
};

class LateGame
{
public:
    static void InitializeLategameLoadouts();
    static void GivePlayerRandomWeapon(AFortPlayerController*);

    static UFortAmmoItemDefinition* GetAmmo(EAmmoType);
    static UFortResourceItemDefinition* GetResource(EFortResourceType);
};