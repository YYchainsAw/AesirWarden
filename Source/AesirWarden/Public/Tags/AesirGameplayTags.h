#pragma once

#include "NativeGameplayTags.h"

namespace AesirGameplayTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Damage);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_GuardPressure);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Boss_GlobalAttack);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Player_Guard_Perfect);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Combat_Defend_Blocked);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Stunned);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dead);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_AttackRecoiled);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Blocking);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_GuardCounterWindow);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_PerfectGuard);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Invulnerable);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dodging);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Pursuing);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Disengaging);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Player_Guard_Blocked);
}
