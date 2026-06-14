#include "pch.h"
#include "Abilities.h"
#include "Utils.h"

void Abilities::InternalServerTryActivateAbility(UAbilitySystemComponent* AbilitySystemComponent, FGameplayAbilitySpecHandle Handle, bool InputPressed, const FPredictionKey& PredictionKey, const FGameplayEventData* TriggerEventData)
{
	FGameplayAbilitySpec* Spec = AbilitySystemComponent->FindAbilitySpecFromHandle(Handle);

	if (Spec == NULL)
	{
		// Can potentially happen in race conditions where client tries to activate ability that is removed server side before it is received.

		AbilitySystemComponent->ClientActivateAbilityFailed(Handle, PredictionKey.Current);
		return;
	}

	const UGameplayAbility* AbilityToActivate = Spec->Ability;

	if (AbilityToActivate == NULL)
	{
		AbilitySystemComponent->ClientActivateAbilityFailed(Handle, PredictionKey.Current);
		return;
	}

	UGameplayAbility* InstancedAbility = NULL;
	Spec->InputPressed = true;

	// Attempt to activate the ability (server side) and tell the client if it succeeded or failed.
	if (AbilitySystemComponent->InternalTryActivateAbility(Handle, PredictionKey, &InstancedAbility, nullptr, TriggerEventData))
	{
		// TryActivateAbility handles notifying the client of success
	}
	else
	{
		AbilitySystemComponent->ClientActivateAbilityFailed(Handle, PredictionKey.Current);
		Spec->InputPressed = false;

		AbilitySystemComponent->ActivatableAbilities.MarkItemDirty(*Spec);
	}
}

void Abilities::Setup()
{
	Utils::Virtual<UFortAbilitySystemComponentAthena>(0xF4, InternalServerTryActivateAbility);
}