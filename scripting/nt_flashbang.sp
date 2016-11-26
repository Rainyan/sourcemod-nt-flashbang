#include <sourcemod>
#include <sdkhooks>
#include <sdktools>
#include <neotokyo>
#include "nt_flashbang/nt_flashbang_base"
#include "nt_flashbang/nt_flashbang_events"
#include "nt_flashbang/nt_flashbang_timers"
#include "nt_flashbang/nt_flashbang_trace"

#define PLUGIN_VERSION "0.3.3"

public Plugin myinfo = {
  name = "NT Flashbangs",
  description = "Replace HE grenades with flashbangs. Experimental.",
  author = "Rain",
  version = PLUGIN_VERSION,
  url = "https://github.com/Rainyan/sourcemod-nt-flashbang"
};

public void OnPluginStart()
{
  g_hCvar_Enabled = CreateConVar("sm_flashbang_enabled", "1.0", "Toggle NT flashbang plugin on/off", _, true, 0.0, true, 1.0);
  g_hCvar_Mode = CreateConVar("sm_flashbang_mode", "3", "How flashbangs work. 1 = all frags are always flashbangs, 2 = players can choose between frag/flash at spawn with the alt fire mode key, 3 = players can freely switch between a frag or flash at any time with the alt fire mode key.", _, true, 1.0, true, 3.0);

  HookConVarChange(g_hCvar_Enabled, Cvar_Enabled);

  CreateConVar("sm_flashbang_version", PLUGIN_VERSION, "NT Flashbang plugin version.", FCVAR_PLUGIN|FCVAR_SPONLY|FCVAR_REPLICATED);
}

public void OnConfigsExecuted()
{
  AutoExecConfig(true);

  if (g_bFirstLaunch && GetConVarBool(g_hCvar_Enabled))
  {
    g_bFirstLaunch = false;
    HookEvent("game_round_start", Event_RoundStart);
  }

  if (GetConVarInt(g_hCvar_Mode) == MODE_FREE_SWITCH)
  {
    g_bCanModifyNade = true;
  }
  else
  {
    g_bCanModifyNade = false;
  }
}

// Purpose: Precache assets used
public void OnMapStart()
{
  PrecacheSound(g_sFlashSound_Environment);
  PrecacheSound(g_sFlashSound_Victim);

  g_iExplosionMark = PrecacheDecal(g_sDecal_ExplosionMark);
  g_iSpecBlindHint_Full = PrecacheModel(g_sDecal_SpectatorBlindHint_Full);
  g_iSpecBlindHint_Half = PrecacheModel(g_sDecal_SpectatorBlindHint_Half);
}

public void OnClientDisconnect(int client)
{
  g_bIsForbiddenVision[client] = false;
  g_bWantsFlashbang[client] = false;
  g_bModifyCooldown[client] = false;
}

public void OnEntityCreated(int entity, const char[] classname)
{
  if (!GetConVarBool(g_hCvar_Enabled))
    return;

  if (StrEqual(classname, "grenade_projectile"))
    SDKHook(entity, SDKHook_SpawnPost, SpawnPost_Grenade);
}

// Purpose: Create a new timer on each
// thrown HE grenade to turn them into flashes
public void SpawnPost_Grenade(int entity)
{
  int owner = GetEntPropEnt(entity, Prop_Data, "m_hOwnerEntity");
  if (!IsValidClient(owner) && GetConVarBool(g_hCvar_Enabled))
  {
    ThrowError("Grenade %i has owner %i who is invalid client!", entity, owner);
  }

  // This flash mode allows players to opt for a regular frag grenade
  if (GetConVarInt(g_hCvar_Mode) != MODE_FORCE_FLASH)
  {
    if (!g_bWantsFlashbang[owner])
      return;
  }

  CreateTimer(FLASHBANG_FUSE, Timer_Flashify, EntIndexToEntRef(entity));
}

// Purpose: Block vision mode use while being flashed.
// Check for user input (alt fire + grenade equipped),
// handle according to the server "sm_flashbang_mode" setting.
public Action OnPlayerRunCmd(int client, int &buttons)
{
  if (!GetConVarBool(g_hCvar_Enabled))
    return Plugin_Continue;

  if (buttons & IN_VISION == IN_VISION && g_bIsForbiddenVision[client])
  {
    SetPlayerVision(client, VISION_NONE);
    return Plugin_Continue;
  }

  if (g_bModifyCooldown[client])
    return Plugin_Continue;

  g_bModifyCooldown[client] = true;
  CreateTimer(0.5, Timer_ModifyCooldown, GetClientUserId(client));

  // Nade toggling is not allowed at all in this cvar mode
  if (GetConVarInt(g_hCvar_Mode) == MODE_FORCE_FLASH)
    return Plugin_Continue;
  // Nade toggling time has expired in this "sm_flashbang_mode" mode
  if (!g_bCanModifyNade)
    return Plugin_Continue;
  // Player isn't pressing the alt fire mode key
  if ((buttons & IN_ATTACK2) != IN_ATTACK2)
    return Plugin_Continue;

  decl String:weaponName[19];
  GetClientWeapon(client, weaponName, sizeof(weaponName));
  // Player doesn't have a frag grenade equipped
  if (!StrEqual(weaponName, "weapon_grenade"))
    return Plugin_Continue;

  // Flip flashbang preference for client
  g_bWantsFlashbang[client] = !g_bWantsFlashbang[client];
  // Announce current preference to client
  PrintToChat(client, "[SM] Grenade type: %s",
    g_sNadeType[g_bWantsFlashbang[client]]);

  return Plugin_Continue;
}
