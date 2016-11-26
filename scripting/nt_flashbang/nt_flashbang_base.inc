#if defined _base_included_
  #endinput
#endif
#define _base_included_

#define FLASHBANG_FUSE 1.4

enum {
  MODE_FORCE_FLASH = 1,
  MODE_SPAWN_PICK,
  MODE_FREE_SWITCH
};

int g_iExplosionMark;
int g_iSpecBlindHint_Full;
int g_iSpecBlindHint_Half;

bool g_bCanModifyNade;
bool g_bIsForbiddenVision[MAXPLAYERS+1];
bool g_bModifyCooldown[MAXPLAYERS+1];
bool g_bWantsFlashbang[MAXPLAYERS+1];

new const String:g_sDecal_ExplosionMark[] = "decals/scorchfademodel";
new const String:g_sDecal_SpectatorBlindHint_Full[] = "vgui/hud/ctg/g_beacon_circle_filled";
new const String:g_sDecal_SpectatorBlindHint_Half[] = "vgui/hud/ctg/g_beacon_circle";
new const String:g_sFlashSound_Environment[] = "player/cx_fire.wav";
new const String:g_sFlashSound_Victim[] = "weapons/hegrenade/frag_explode.wav";

new const String:g_sNadeType[][] = {
  "FRAG",
  "FLASH"
};

Handle g_hCvar_Enabled;
Handle g_hCvar_Mode;

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

  //Spectators_ShowBlindHint(client, intensity);
}

// Purpose: Display a HUD hint over flashed player's head for spectators.
void Spectators_ShowBlindHint(int client, int intensity)
{
  return;

  if (!IsValidClient(client))
    return;

  float position[3];
  GetClientEyePosition(client, position);
  position[2] += 10.0;
/*
  TE_Start("GlowSprite");
  TE_WriteVector("m_vecOrigin", position);
  TE_WriteFloat("m_fScale", 1.0);
  TE_WriteFloat("m_fLife", 3.5);
  TE_WriteNum("m_nBrightness", 100);
  if (intensity < 100)
  {
    TE_WriteNum("m_nModelIndex", g_iSpecBlindHint_Half);
  }
  else
  {
    TE_WriteNum("m_nModelIndex", g_iSpecBlindHint_Full);
  }
*/

  float angles[3] = {0.0, 0.0, 0.0};
  TE_Start("EffectDispatch");
  TE_WriteFloat("m_vOrigin[0]", position[0]);
  TE_WriteFloat("m_vOrigin[1]", position[1]);
  TE_WriteFloat("m_vOrigin[2]", position[2]);
  TE_WriteFloat("m_vStart[0]", position[0]);
  TE_WriteFloat("m_vStart[1]", position[1]);
  TE_WriteFloat("m_vStart[2]", position[2]);
  TE_WriteVector("m_vAngles", angles);
  TE_WriteVector("m_vNormal", angles);
  TE_WriteNum("m_fFlags", 0);
  TE_WriteFloat("m_flMagnitude", 1.0);
  TE_WriteFloat("m_flScale", 1.0);

  TE_WriteNum("m_nAttachmentIndex", 0);
  TE_WriteNum("m_nSurfaceProp", 0);
  TE_WriteNum("m_iEffectName", 0);
  TE_WriteNum("m_nMaterial", 0);
  TE_WriteNum("m_nDamageType", 0);
  TE_WriteNum("m_nHitBox", 0);
  TE_WriteNum("entindex", g_iSpecBlindHint_Full);
  TE_WriteNum("m_nColor", 0);
  TE_WriteFloat("m_flRadius", 100.0);


  for (int i = 1; i <= MaxClients; i++)
  {
    if (!IsValidClient(i) || IsFakeClient(i))
      continue;

    //int team = GetClientTeam(i);
    //if (IsPlayerAlive(i) || team != TEAM_SPECTATOR)
    //  continue;

    TE_SendToClient(i);
  }
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
    MakeVectorFromPoints(eyePos, pos, vecDir);
    // Convert to angles
    float realDir[3];
    GetVectorAngles (vecDir, realDir);

    // Subtract eyes/world offset from grenade angle
    float angle[3];
    SubtractVectors(realDir, eyeAngles, angle);

    // How many degrees turned away from flash,
    // 180 = completely turned, 0 = completely facing.
    // Angles over 180 get capped, eg. 181 -> 179.
    for (int j = 0; j < 2; j++)
    {
      if (angle[j] > 180)
      {
        angle[j] -= 360;
      }
      else if (angle[j] < -180)
      {
        angle[j] += 360;
      }

      if (angle[j] < 0)
        angle[j] *= -1;
    }
    //PrintToChat(i, "Final angles: %f, %f", angle[0], angle[1]);

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

bool IsAssault(int client)
{
  int team = GetClientTeam(client);
  if (team != TEAM_NSF && team != TEAM_JINRAI)
    return false;

  if (GetPlayerClass(client) != CLASS_ASSAULT)
    return false;

  return true;
}
