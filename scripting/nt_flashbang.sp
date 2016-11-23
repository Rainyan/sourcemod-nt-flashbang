#include <sourcemod>
#include <sdkhooks>
#include <sdktools>
#include <neotokyo>

#define PLUGIN_VERSION "0.1"
#define TIMER_GRENADE 1.5

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

public Plugin myinfo = {
  name = "NT Flashbangs",
  description = "Replace HE grenades with flashbangs. Experimental.",
  author = "Rain",
  version = PLUGIN_VERSION,
  url = "https://github.com/Rainyan/sourcemod-nt-flashbang"
};

public void OnPluginStart()
{
  CreateConVar("sm_flashbang_version", PLUGIN_VERSION, "NT Flashbang plugin version.", FCVAR_PLUGIN|FCVAR_SPONLY|FCVAR_REPLICATED);

  g_hCvar_Mode = CreateConVar("sm_flashbang_mode", "3", "How flashbangs work. 1 = all frags are always flashbangs, 2 = players can choose between frag/flash at spawn with the alt fire mode key, 3 = players can freely switch between a frag or flash at any time with the alt fire mode key.", _, true, 1.0, true, 3.0);

  HookEvent("game_round_start", Event_RoundStart);
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

// Purpose: Check for user input (alt fire + grenade equipped),
// handle according to the server "sm_flashbang_mode" setting.
public Action OnPlayerRunCmd(int client, int &buttons)
{
  // Check for cooldown before doing anything else, for performance reasons
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

// Purpose: Precache the sounds used for flash effects
public void OnMapStart()
{
  PrecacheSound(g_sFlashSound_Environment);
  PrecacheSound(g_sFlashSound_Victim);
}

public void OnEntityCreated(int entity, const char[] classname)
{
  // Need to wait for entity spawn to get its coordinates
  if (StrEqual(classname, "grenade_projectile"))
    SDKHook(entity, SDKHook_SpawnPost, SpawnPost_Grenade);
}

// Purpose: Create a new timer on each thrown HE grenade to turn them into flashes
public void SpawnPost_Grenade(int entity)
{
  float position[3];
  GetEntPropVector(entity, Prop_Send, "m_vecOrigin", position);

  int owner = GetFragOwner(entity, position);
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

  CreateTimer(TIMER_GRENADE, Timer_Flashify, entityData);
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
      PrintToChatAll("MISS!");
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
    angle[0] -= 180;
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

    // Start at 100% flashed
    float flashedPercent = 125.0;
    // Reduce flashedness based on distance
    flashedPercent -= distance / 85;
    // Cap reduction
    float minimumInitialFlash = 25.0;
    if (flashedPercent < minimumInitialFlash)
      flashedPercent = minimumInitialFlash;

    PrintToChat(i, "Flashed! Initial flash: %i percent", RoundToNearest(flashedPercent));

    float basePercentile = 0.555; // flashed percentile unit (~100/180)
    float bestPossibleDodge = 10.0; // can negate max 90% of flash by turning

    // Reduce flashedness based on dodge on X and Y axes
    float flashAvoidance_Y = (angle[0] * basePercentile) - (flashedPercent / 2);
    float flashAvoidance_X = (angle[1] * basePercentile) - (flashedPercent / 2);

    // Emphasize horizonal dodge over vertical
    if (flashAvoidance_Y < flashAvoidance_X)
      flashAvoidance_Y / flashAvoidance_X;

    flashedPercent -= flashAvoidance_Y;
    flashedPercent -= flashAvoidance_X;

    // Cap final flash amount
    if (flashedPercent < bestPossibleDodge)
      flashedPercent = bestPossibleDodge;
    else if (flashedPercent > 100)
      flashedPercent = 100.0;

    int intensity = RoundToNearest(flashedPercent);

    PrintToChat(i, "Amount after dodge: %i percent", intensity);
    BlindPlayer(i, intensity);

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
  PrintToChatAll("Tracing from %f %f %f to %f %f %f",
    startPos[0], startPos[1], startPos[2],
    eyePos[0], eyePos[1], eyePos[2]);

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
    PrintToChatAll("TR_GetEntityIndex %i is not client %i", hitIndex, client);
    return false;
  }

  return true;
}

// Purpose: Check whether trace hit entity index equals desired client index.
// This means the ray hit this player.
public bool TraceFilter_IsPlayer(int hitEntity, int mask, any targetClient)
{
  if (hitEntity == targetClient) {
    PrintToServer("hitEntity %i target %i = %b", hitEntity, targetClient, true);
    return true;
  }

  PrintToServer("hitEntity %i target %i = %b", hitEntity, targetClient, false);
  return false;
}

// Purpose: Flash client's screen white and play a sound effect
void BlindPlayer(int client, int intensity)
{
  if (!IsValidClient(client))
    return;

  if (intensity < 1 || intensity > 100)
    ThrowError("Invalid intensity %i, expected a value between 1-100.", intensity);

  // Close vision if enabled
  // FIXME: This acts kind of weird with half flashes sometimes
  if (IsUsingVision(client))
  {
    ClientCommand(client, "-thermoptic");
  }

  int resetDuration = RoundToNearest(10.0 * intensity);

  int alpha = RoundToNearest(2.5 * intensity);
  if (alpha < 5)
    alpha = 5;

  float volume = 0.007 * intensity;
  if (volume < 0.1)
    volume = 0.1;

  Handle userMsg = StartMessageOne("Fade", client);
  BfWriteShort(userMsg, 500); // Flash duration
  BfWriteShort(userMsg, resetDuration); // View reset duration
  BfWriteShort(userMsg, 0x0001); // Fade in flag
  BfWriteByte(userMsg, 255); // R
  BfWriteByte(userMsg, 255); // G
  BfWriteByte(userMsg, 255); // B
  BfWriteByte(userMsg, alpha); // A
  EndMessage();

  EmitSoundToClient(client,
    g_sFlashSound_Victim, _, _, SNDLEVEL_NORMAL, _, volume, 200);
}

// Purpose: Deduce the frag entity owner by finding the closest
// grenade holding player upon entity creation. Kind of hacky...
int GetFragOwner(int entity, float[3] position)
{
  if (!IsValidEntity(entity))
    return 0;

  float eyePos[3];
  float distance[MAXPLAYERS+1];

  int candidates;
  for (int i = 1; i <= MaxClients; i++)
  {
    if (!IsValidClient(i))
      continue;

    // Client has grenade equipped
    decl String:weaponName[19];
    GetClientWeapon(i, weaponName, sizeof(weaponName));
    if (!StrEqual(weaponName, "weapon_grenade"))
      continue;

    GetClientEyePosition(i, eyePos);

    // Get client distance from frag
    distance[i] = GetVectorDistance(position, eyePos);
    candidates++;
  }

  // Get the closest client to the frag
  float distSort[2];
  PrintToServer("There are %i candidates", candidates);
  for (int i = 1; i <= MaxClients; i++)
  {
    if (distance[i] == 0)
    {
      continue;
    }
    else if (distSort[1] == 0 || distance[i] < distSort[1])
    {
      distSort[0] = i*1.0;
      distSort[1] = distance[i];
    }
  }

  int owner = RoundToNearest(distSort[0]);

  PrintToServer("Owner is %i with distance %f", owner, distSort[1]);

  decl String:clientName[MAX_NAME_LENGTH];
  GetClientName(owner, clientName, sizeof(clientName));
  PrintToChatAll("Grenade owner: %i %s", owner, clientName);

  return owner;
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
