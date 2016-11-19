#include <sourcemod>
#include <sdktools>
#include <neotokyo>

#define PLUGIN_VERSION "0.1"
#define TIMER_GRENADE 1.5

new const String:g_sFlashSound_Environment[] = "player/cx_fire.wav";
new const String:g_sFlashSound_Victim[] = "weapons/hegrenade/frag_explode.wav";

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
    CreateTimer(TIMER_GRENADE, Timer_Flashify, entity);
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
