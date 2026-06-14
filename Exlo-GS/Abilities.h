#pragma once
#include "pch.h"

class Abilities
{
private:
    static void InternalServerTryActivateAbility(UAbilitySystemComponent*, FGameplayAbilitySpecHandle, bool, const FPredictionKey&, const FGameplayEventData*);
public:
    static void Setup();
};