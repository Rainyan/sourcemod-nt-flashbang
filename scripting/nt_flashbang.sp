#include <sourcemod>
#include <sdktools>
#include <neotokyo>

#define TIMER_GRENADE 1.5

new const String:g_sFlashSound_Environment[] = "player/cx_fire.wav";
new const String:g_sFlashSound_Victim[] = "weapons/hegrenade/frag_explode.wav";

public Plugin myinfo = {
  name = "NT Flashbangs",
  description = "Replace HE grenades with flashbangs. Experimental.",
  author = "Rain",
  version = "0.1",
  url = "https://github.com/Rainyan/sourcemod-nt-flashbang"
};

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
  PrintToChatAll("Doing trace for pos %f %f %f", pos[0], pos[1], pos[2]);

  for (int i = 1; i <= MaxClients; i++) {
    if (!IsValidClient(i))
      continue;

    float eyePos[3];
    GetClientEyePosition(i, eyePos);

    if (!TraceHitEyes(i, pos, eyePos)) {
      PrintToChatAll("MISS!");
      continue;
    }

    BlindPlayer(i, 1000, 255);

    char clientName[MAX_NAME_LENGTH];
    GetClientName(i, clientName, sizeof(clientName));
    PrintToChatAll("Trace hit client %i \"%s\" at eye pos %f %f %f",
      i, clientName, eyePos[0], eyePos[1], eyePos[2]);
    PrintToChatAll("HIT!");
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
void BlindPlayer(int client, int duration, int alpha, float volume = 0.65)
{
  if (!IsValidClient(client))
    return;

  if (duration < 1)
    ThrowError("Invalid blind duration %i", duration);

  if (alpha < 1 || alpha > 255)
    ThrowError("Invalid alpha amount %i, expected value within 1 - 255", alpha);

  if (volume <= 0 || volume > 1)
    ThrowError("Invalid volume %f, expected value within range 0.0-1.0", volume);

  Handle userMsg = StartMessageOne("Fade", client);
  BfWriteShort(userMsg, duration); // Flash duration
  BfWriteShort(userMsg, 0); // View reset duration
  BfWriteShort(userMsg, 0x0001); // Fade in flag
  BfWriteByte(userMsg, 255); // R
  BfWriteByte(userMsg, 255); // G
  BfWriteByte(userMsg, 255); // B
  BfWriteByte(userMsg, alpha); // A
  EndMessage();

  EmitSoundToClient(client,
    g_sFlashSound_Victim, _, _, SNDLEVEL_NORMAL, _, 0.65, 200);
}
