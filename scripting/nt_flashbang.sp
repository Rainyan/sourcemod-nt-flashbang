#include <sourcemod>
#include <sdkhooks>
#include <sdktools>
#include <neotokyo>

#define PLUGIN_VERSION "0.3"

#define FLASHBANG_FUSE 1.5

bool g_bIsForbiddenVision[MAXPLAYERS+1];

new const String:g_sFlashSound_Environment[] = "player/cx_fire.wav";
new const String:g_sFlashSound_Victim[] = "weapons/hegrenade/frag_explode.wav";
new const String:g_sNadeType[][] = {"FRAG", "FLASH"};

bool g_bCanModifyNade;
bool g_bModifyCooldown[MAXPLAYERS+1];
bool g_bWantsFlashbang[MAXPLAYERS+1];

Handle g_hCvar_Mode;

enum {
  MODE_FORCE_FLASH = 1,
  MODE_SPAWN_PICK,
  MODE_FREE_SWITCH
};

Handle g_hCvar_Enabled;

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

  CreateConVar("sm_flashbang_version", PLUGIN_VERSION, "NT Flashbang plugin version.", FCVAR_PLUGIN|FCVAR_SPONLY|FCVAR_REPLICATED);

  g_hCvar_Mode = CreateConVar("sm_flashbang_mode", "3", "How flashbangs work. 1 = all frags are always flashbangs, 2 = players can choose between frag/flash at spawn with the alt fire mode key, 3 = players can freely switch between a frag or flash at any time with the alt fire mode key.", _, true, 1.0, true, 3.0);

  HookEvent("game_round_start", Event_RoundStart);
}

public void OnConfigsExecuted()
{
  AutoExecConfig(true);

  if (GetConVarInt(g_hCvar_Mode) == MODE_FREE_SWITCH)
  {
    g_bCanModifyNade = true;
  }
  else
  {
    g_bCanModifyNade = false;
  }
}

public Action Event_RoundStart(Handle event, const char[] name, bool dontBroadcast)
{
  g_bCanModifyNade = true;
  Assaults_GiveSpawnInformation();

  if (GetConVarInt(g_hCvar_Mode) == MODE_SPAWN_PICK)
    CreateTimer(15.0, Timer_CanModifyNade_Revoke);
}

// Purpose: Prevent nade switching after spawn freezetime, if desired
public Action Timer_CanModifyNade_Revoke(Handle timer)
{
  g_bCanModifyNade = false;
  Assaults_SendMessage("[SM] Flashbang choose time has expired.");
}

// Purpose: Block vision mode use while being flashed.
// Check for user input (alt fire + grenade equipped),
// handle according to the server "sm_flashbang_mode" setting.
public Action OnPlayerRunCmd(int client, int &buttons)
{
  if (buttons & IN_VISION && g_bIsForbiddenVision[client])
  {
    SetPlayerVision(client, VISION_NONE);
    return Plugin_Continue;
  }

  if (g_bModifyCooldown[client])
    return Plugin_Continue;

  g_bModifyCooldown[client] = true;
  CreateTimer(0.5, Timer_ModifyCooldown, client);

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

// Purpose: Only allow a few input checks per client per second for performance
public Action Timer_ModifyCooldown(Handle timer, any client)
{
  g_bModifyCooldown[client] = false;

  return Plugin_Handled;
}

public void OnClientDisconnect(int client)
{
  g_bIsForbiddenVision[client] = false;
}

// Purpose: Precache the sounds used for flash effects
public void OnMapStart()
{
  PrecacheSound(g_sFlashSound_Environment);
  PrecacheSound(g_sFlashSound_Victim);
}

public void OnEntityCreated(int entity, const char[] classname)
{
  if (!GetConVarBool(g_hCvar_Enabled)) {
    return;
  }

  // Need to wait for entity spawn to get its coordinates
  if (StrEqual(classname, "grenade_projectile"))
    SDKHook(entity, SDKHook_SpawnPost, SpawnPost_Grenade);
}

// Purpose: Create a new timer on each thrown HE grenade to turn them into flashes
public void SpawnPost_Grenade(int entity)
{
  float position[3];
  GetEntPropVector(entity, Prop_Send, "m_vecOrigin", position);

  int owner = GetEntPropEnt(entity, Prop_Data, "m_hOwnerEntity");
  // This flash mode allows players to opt for a regular frag grenade
  if (GetConVarInt(g_hCvar_Mode) != MODE_FORCE_FLASH)
  {
    if (!g_bWantsFlashbang[owner])
      return;
  }

  DataPack entityData = new DataPack();
  entityData.WriteCell(EntIndexToEntRef(entity));
  //entityData.WriteCell(owner);

  PrintToChatAll("Written coords: %f %f %f", position[0], position[1], position[2]);

  CreateTimer(FLASHBANG_FUSE, Timer_Flashify, entityData);
}

// Purpose: Turn a grenade into a flashbang by entity index
public Action Timer_Flashify(Handle timer, DataPack entityData)
{
  entityData.Reset();
  int entRef =  entityData.ReadCell(); // entity reference
  //int owner = entityData.ReadCell(); // owner client index

  int entity = EntRefToEntIndex(entRef);
  if (entity == INVALID_ENT_REFERENCE)
    return Plugin_Stop;

  float explosionPos[3];
  GetEntPropVector(entity, Prop_Send, "m_vecOrigin", explosionPos);

  // Make flash explosion sound at grenade position
  EmitSoundToAll(g_sFlashSound_Environment, entity, _, SNDLEVEL_GUNFIRE, _, 1.0);

  // Remove the grenade
  AcceptEntityInput(entity, "kill");

  // See if anyone gets blinded
  CheckIfFlashed(explosionPos);

  return Plugin_Handled;
}

// Purpose: Check for line of sight from flashbang
// origin to all players and flash them accordingly.
void CheckIfFlashed(float[3] pos)
{
  //PrintToChatAll("Doing trace for pos %f %f %f", pos[0], pos[1], pos[2]);

  for (int i = 1; i <= MaxClients; i++) {
    if (!IsValidClient(i) || IsFakeClient(i))
      continue;

    float eyePos[3];
    GetClientEyePosition(i, eyePos);

    if (!TraceHitEyes(i, pos, eyePos)) {
      //PrintToChatAll("MISS!");
      continue;
    }

    // Eye angles player is facing, in other words world offset
    float eyeAngles[3];
    GetClientEyeAngles(i, eyeAngles);

    // Direction vector from player to flashbang
    float vecDir[3];
    MakeVectorFromPoints(pos, eyePos, vecDir);
    // Convert to angles
    float realDir[3];
    GetVectorAngles (vecDir, realDir);

    // Subtract eyes/world offset from grenade angle
    float angle[3];
    SubtractVectors(realDir, eyeAngles, angle);

    // How many degrees turned away from flash,
    // 180 = completely turned, 0 = completely facing
    //angle[0] -= 180;
    angle[1] -= 180;
    for (int j = 0; j < 3; j++)
    {
      if (angle[j] > 180)
        angle[j] -= 360;

      if (angle[j] < -180)
        angle[j] += 360;

      if (angle[j] < 0)
       angle[j] *= -1;
    }

    // Get eyes distance from flash
    float distance = GetVectorDistance(eyePos, pos);

    // Start at this percentage flashed
    float flashedPercent = 100.0;

    //PrintToChat(i, "Flashed! Initial flash: %i percent", RoundToNearest(flashedPercent));

    // flashed percentile unit (~100/180 = 0.555)
    float basePercentile = 0.555;
    // can negate max 100%-bestPossibleDodge of flash by turning
    float bestPossibleDodge = 25.0;

    // Reduce flashedness based on dodge on X and Y axes
    float flashAvoidance_Y = (angle[0] * basePercentile);
    float flashAvoidance_X = (angle[1] * basePercentile);

    //PrintToServer("Angles %f %f", angle[0], angle[1]);
    if (angle[0] >= 75 || angle[1] >= 75)
    {
      //PrintToServer("Reducing from %f", flashedPercent);
      flashedPercent -= flashAvoidance_Y;
      flashedPercent -= flashAvoidance_X;
      //PrintToServer("to %f", flashedPercent);
    }
    else
    {
      //PrintToServer("False, %f >= 90 || %f >= 90", angle[0], angle[1]);
    }

    // Cap final flash amount
    if (flashedPercent < bestPossibleDodge)
      flashedPercent = bestPossibleDodge;
    else if (flashedPercent > 100)
      flashedPercent = 100.0;

    int intensity = RoundToNearest(flashedPercent);
    int duration = RoundToNearest(intensity * 10 - distance*0.5);
    //PrintToServer("duration %i = %i * 10 - %f", duration, intensity, distance/10);

    if (duration > 1000)
      duration = 1000;
    else if (duration < 50)
      duration = 50;

    PrintToChat(i, "Flashed! Intensity %i%%, duration %i%%)", intensity, duration/10);
    BlindPlayer(i, intensity, duration);

    /*
    PrintToConsole(i, "Eye %f %f - dir %f %f = %f %f",
      eyeAngles[0], eyeAngles[1],
      vecDir[0], vecDir[1],
      angle[0], angle[1]);

    PrintToChatAll("Angles: %f %f %f", angle[0], angle[1], angle[2])

    char clientName[MAX_NAME_LENGTH];
    GetClientName(i, clientName, sizeof(clientName));
    PrintToChatAll("Trace hit client %i \"%s\" at eye pos %f %f %f",
      i, clientName, eyePos[0], eyePos[1], eyePos[2]);
    PrintToChatAll("HIT!");
    */
  }
}

bool TraceHitEyes(int client, float[3] startPos, float[3] eyePos)
{
  Handle ray = TR_TraceRayFilterEx(
    startPos, eyePos, MASK_VISIBLE, RayType_EndPoint, TraceFilter_IsPlayer, client);

    /*// Ray hit nothing(??)
    if (!TR_DidHit(ray)) {
      PrintToChatAll("TR_DidHit = false");
      delete ray;
      return false;
    }*/

  int hitIndex = TR_GetEntityIndex(ray);
  delete ray;
  if (hitIndex != client) {
    //PrintToChatAll("TR_GetEntityIndex %i is not client %i", hitIndex, client);
    return false;
  }

  return true;
}

// Purpose: Check whether trace hit entity index equals desired client index.
// This means the ray hit this player.
public bool TraceFilter_IsPlayer(int hitEntity, int mask, any targetClient)
{
  if (hitEntity == targetClient) {
    //PrintToServer("hitEntity %i target %i = %b", hitEntity, targetClient, true);
    return true;
  }

  //PrintToServer("hitEntity %i target %i = %b", hitEntity, targetClient, false);
  return false;
}

// Purpose: Flash client's screen white and play a sound effect
void BlindPlayer(int client, int intensity, int resetDuration)
{
  if (!IsValidClient(client))
    return;

  // TODO: These checks will become redundant once the blind values are decided
  if (intensity < 1 || intensity > 100)
    ThrowError("Invalid intensity %i, expected a value between 1-100.", intensity);

  if (resetDuration < 1 || resetDuration > 1000)
    ThrowError("Invalid reset duration %i, expected a value between  1-1000.", resetDuration);

  // Vision mode can make half flashes easy to
  // see through, so vision mode gets disabled
  g_bIsForbiddenVision[client] = true;
  SetPlayerVision(client, VISION_NONE);
  int userid = GetClientUserId(client);
  CreateTimer(0.25 + resetDuration / 500.0, Timer_AllowVision, userid);

  int alpha = RoundToNearest(2.5 * intensity);
  if (alpha < 5)
    alpha = 5;

  float volume = 0.007 * intensity;
  if (volume < 0.1)
    volume = 0.1;

  Handle userMsg = StartMessageOne("Fade", client);
  BfWriteShort(userMsg, 500); // Flash duration
  BfWriteShort(userMsg, resetDuration); // View reset duration (ms times 2??)
  BfWriteShort(userMsg, 0x0001); // Fade in flag
  BfWriteByte(userMsg, 255); // R
  BfWriteByte(userMsg, 255); // G
  BfWriteByte(userMsg, 255); // B
  BfWriteByte(userMsg, alpha); // A
  EndMessage();

  EmitSoundToClient(client,
    g_sFlashSound_Victim, _, _, SNDLEVEL_NORMAL, _, volume, 200);
}

public Action Timer_AllowVision(Handle timer, int userid)
{
  int client = GetClientOfUserId(userid);
  if (!IsValidClient(client))
    return Plugin_Stop;

  //PrintToChat(client, "Fade expired");
  g_bIsForbiddenVision[client] = false;

  return Plugin_Handled;
}

// Purpose: Let assault players know which flashbang rules the server is using
void Assaults_GiveSpawnInformation()
{
  int mode = GetConVarInt(g_hCvar_Mode);

  for (int i = 1; i <= MaxClients; i++)
  {
    if (!IsValidClient(i) || IsFakeClient(i) || !IsAssault(i))
      continue;

    switch (mode)
    {
      case MODE_SPAWN_PICK:
      {
        PrintToChat(i, "[SM] During round start, you can choose between a frag \
and flashbang with the fire mode key.");
      }
      case MODE_FORCE_FLASH:
      {
        PrintToChat(i, "[SM] All frag grenades are flashbangs.");
      }
      case MODE_FREE_SWITCH:
      {
        PrintToChat(i, "[SM] You can freely switch between a frag and flashbang \
during the round with the fire mode key.");
      }
    }
  }
}

void Assaults_SendMessage(const char[] message, any ...)
{
  decl String:formatMessage[128];
  VFormat(formatMessage, sizeof(formatMessage), message, 2);

  for (int i = 1; i <= MaxClients; i++)
  {
    if (!IsValidClient(i) || IsFakeClient(i) || !IsAssault(i))
      continue;

    PrintToChat(i, formatMessage);
  }
}

bool IsAssault(int client)
{
  int team = GetClientTeam(client);
  if (team != TEAM_NSF && team != TEAM_JINRAI)
    return false;

  if (GetPlayerClass(client) != CLASS_ASSAULT)
    return false;

  return true;
}
