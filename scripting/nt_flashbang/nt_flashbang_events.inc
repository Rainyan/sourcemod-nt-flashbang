#if defined _flashbangs_events_included_
  #endinput
#endif
#define _flashbangs_events_included_

public Action Event_RoundStart(Handle event, const char[] name, bool dontBroadcast)
{
  g_bCanModifyNade = true;

  Assaults_GiveSpawnInformation();

  if (GetConVarInt(g_hCvar_Mode) == MODE_SPAWN_PICK)
    CreateTimer(15.0, Timer_CanModifyNade_Revoke);
}

public void Cvar_Enabled(ConVar cvar, const char[] oldVal, const char[] newVal)
{
  int iNew = StringToInt(newVal);
  int iOld = StringToInt(oldVal);
  // Plugin enabled
  if (iNew && !iOld)
  {
    HookEvent("game_round_start", Event_RoundStart);

    // Update modify bool since it hasn't been updated while plugin was off
    if (GetConVarInt(g_hCvar_Mode) == MODE_FREE_SWITCH)
    {
      g_bCanModifyNade = true;
    }
    else
    {
      g_bCanModifyNade = false;
    }
  }
  // Plugin disabled
  else if (!iNew && iOld)
  {
    UnhookEvent("game_round_start", Event_RoundStart);
  }
}
