#include <sourcemod>
#include <sdktools>
#include <neotokyo>

#define PLUGIN_VERSION "0.1"

new const String:g_sFlashSound_Environment[] = "player/cx_fire.wav";
new const String:g_sFlashSound_Victim[] = "weapons/hegrenade/frag_explode.wav";

Handle g_hCvar_Debug_FuseLength;
Handle g_hCvar_Debug_FlashPercent;
Handle g_hCvar_Debug_FlashPercentDivisor;
Handle g_hCvar_Debug_MinimumInitialFlash;
Handle g_hCvar_Debug_BasePercentile;
Handle g_hCvar_Debug_BestDodge;
Handle g_hCvar_Debug_FlashAvoidanceDivisorX;
Handle g_hCvar_Debug_FlashAvoidanceDivisorY;
Handle g_hCvar_Debug_ResetDuration_Multipier;
Handle g_hCvar_Debug_ViewAlpha_Multiplier;
Handle g_hCvar_Debug_ViewAlpha_Min;
Handle g_hCvar_Debug_Volume_Multiplier;
Handle g_hCvar_Debug_Volume_Min;

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

  g_hCvar_Debug_FuseLength = CreateConVar("sm_flashbang_debug_fuse", "1.5", "Flashbang fuse length. Debug command.", _, true, 0.1);
  g_hCvar_Debug_FlashPercent = CreateConVar("sm_flashbang_debug_CheckIfFlashed_initial_flashed_percent", "125", "CheckIfFlashed - float flashedPercent (Start at this percentage flashed)", _, true, 0.0);
  g_hCvar_Debug_FlashPercentDivisor = CreateConVar("sm_flashbang_debug_CheckIfFlashed_initial_flashed_percent_divisor", "85", "CheckIfFlashed - float distance (Reduce flashedness based on distance)", _, true, 1.0);
  g_hCvar_Debug_MinimumInitialFlash = CreateConVar("sm_flashbang_debug_CheckIfFlashed_minimum_initial_flash", "25", "CheckIfFlashed - float minimumInitialFlash (Cap flash reduction)", _, true, 0.0, true, 100.0);

  g_hCvar_Debug_BasePercentile = CreateConVar("sm_flashbang_debug_base_percentile", "0.555", "CheckIfFlashed - float basePercentile (flashed percentile unit, ~100/180)");
  g_hCvar_Debug_BestDodge = CreateConVar("sm_flashbang_debug_best_dodge", "10", "CheckIfFlashed - float bestPossibleDodge (can negate max 100%-bestPossibleDodge of flash by turning)");

  g_hCvar_Debug_FlashAvoidanceDivisorX = CreateConVar("sm_flashbang_debug_avoidance_divisor_x", "2", "CheckIfFlashed - int (Reduce flashedness based on dodge on X and Y axes)", _, true, 0.1);
  g_hCvar_Debug_FlashAvoidanceDivisorY = CreateConVar("sm_flashbang_debug_avoidance_divisor_y", "2", "CheckIfFlashed - int (Reduce flashedness based on dodge on X and Y axes)", _, true, 0.1);

  g_hCvar_Debug_ResetDuration_Multipier = CreateConVar("sm_flashbang_debug_view_reset_duration_multiplier", "10", "BlindPlayer - float (intensity * this multiplier = view reset duration)", _, true, 0.0);

  g_hCvar_Debug_ViewAlpha_Multiplier = CreateConVar("sm_flashbang_debug_view_alpha_multiplier", "2.5", "BlindPlayer - float (multiplier * internsity = view alpha)");
  g_hCvar_Debug_ViewAlpha_Min = CreateConVar("sm_flashbang_debug_view_alpha_minimum", "5", "BlindPlayer - int (minimum view alpha)", _, true, 0.0);

  g_hCvar_Debug_Volume_Multiplier = CreateConVar("sm_flashbang_debug_volume_multipliler", "0.007", "BlindPlayer - float (multiplier * intensity = blind victim fx volume)", _, true, 0.0);
  g_hCvar_Debug_Volume_Min = CreateConVar("sm_flashbang_debug_volume_minimum", "0.1", "BlindPlayer - float (minimum blind victim fx volume, range 0.0-1.0)", _, true, 0.0, true, 1.0);
}

public void OnMapStart()
{
  PrecacheSound(g_sFlashSound_Environment);
  PrecacheSound(g_sFlashSound_Victim);
}

// Purpose: Create a new timer on each thrown HE grenade to turn them into flashes
public void OnEntityCreated(int entity, const char[] classname)
{
  if (StrEqual(classname, "grenade_projectile")) {
    CreateTimer(GetConVarFloat(g_hCvar_Debug_FuseLength), Timer_Flashify, entity);
  }
}

public Action Timer_Flashify(Handle timer, any entity)
{
  if (!IsValidEntity(entity))
    return Plugin_Stop;

  // Get flash position
  float nadeCoords[3];
  GetEntPropVector(entity, Prop_Send, "m_vecOrigin", nadeCoords);

  // Make flash explosion sound at grenade position
  EmitSoundToAll(g_sFlashSound_Environment, entity, _, SNDLEVEL_GUNFIRE, _, 1.0);

  // Remove the grenade
  AcceptEntityInput(entity, "kill");

  // See if anyone gets blinded
  CheckIfFlashed(nadeCoords);

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

    // Start at this percentage flashed
    float flashedPercent = GetConVarFloat(g_hCvar_Debug_FlashPercent);
    // Reduce flashedness based on distance
    flashedPercent -= distance / GetConVarFloat(g_hCvar_Debug_FlashPercentDivisor);
    // Cap reduction
    float minimumInitialFlash = GetConVarFloat(g_hCvar_Debug_MinimumInitialFlash);
    if (flashedPercent < minimumInitialFlash)
      flashedPercent = minimumInitialFlash;

    PrintToChat(i, "Flashed! Initial flash: %i percent", RoundToNearest(flashedPercent));

    // flashed percentile unit (~100/180 = 0.555)
    float basePercentile = GetConVarFloat(g_hCvar_Debug_BasePercentile);
    // can negate max 100%-bestPossibleDodge of flash by turning
    float bestPossibleDodge = GetConVarFloat(g_hCvar_Debug_BestDodge);

    // Reduce flashedness based on dodge on X and Y axes
    float flashAvoidance_Y = (angle[0] * basePercentile) - (flashedPercent / GetConVarFloat(g_hCvar_Debug_FlashAvoidanceDivisorY));
    float flashAvoidance_X = (angle[1] * basePercentile) - (flashedPercent / GetConVarFloat(g_hCvar_Debug_FlashAvoidanceDivisorX));

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

  int resetDuration = RoundToNearest(GetConVarFloat(g_hCvar_Debug_ResetDuration_Multipier) * intensity);

  int alpha = RoundToNearest(GetConVarFloat(g_hCvar_Debug_ViewAlpha_Multiplier) * intensity);
  if (alpha < GetConVarInt(g_hCvar_Debug_ViewAlpha_Min))
    alpha = GetConVarInt(g_hCvar_Debug_ViewAlpha_Min);

  float volume = GetConVarFloat(g_hCvar_Debug_Volume_Multiplier) * intensity;
  if (volume < GetConVarFloat(g_hCvar_Debug_Volume_Min))
    volume = GetConVarFloat(g_hCvar_Debug_Volume_Min);

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
