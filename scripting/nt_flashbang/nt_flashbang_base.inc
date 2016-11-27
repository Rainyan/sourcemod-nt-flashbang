#if defined _flashbangs_base_included_
  #endinput
#endif
#define _flashbangs_base_included_

#define PLUGIN_VERSION "0.3.4"

#define FLASHBANG_FUSE 1.4

enum {
  MODE_FORCE_FLASH = 1,
  MODE_SPAWN_PICK,
  MODE_FREE_SWITCH
};

enum {
  PITCH = 0,
  YAW,
  ROLL
};

int g_iExplosionMark;
int g_iSpecBlindHint_Full;
int g_iSpecBlindHint_Half;

int g_iFlashColor[MAXPLAYERS+1][3];
int g_iFlashPreview[MAXPLAYERS+1];

bool g_bFirstLaunch = true;
bool g_bCanModifyNade;
bool g_bIsForbiddenVision[MAXPLAYERS+1];
bool g_bModifyCooldown[MAXPLAYERS+1];
bool g_bWantsFlashbang[MAXPLAYERS+1];

char g_sFlashColorName[MAXPLAYERS+1][MAX_COLOR_STRLEN];

new const String:g_sDecal_ExplosionMark[] = "decals/scorchfademodel";
new const String:g_sTexture_SpectatorBlindHint_Full[] = "materials/vgui/hud/ctg/g_beacon_circle_filled.vmt";
new const String:g_sTexture_SpectatorBlindHint_Half[] = "materials/vgui/hud/ctg/g_beacon_circle.vmt";
new const String:g_sFlashSound_Environment[] = "player/cx_fire.wav";
new const String:g_sFlashSound_Victim[] = "weapons/hegrenade/frag_explode.wav";
new const String:g_sMenuSound_Cancel[] = "buttons/combine_button7.wav";
new const String:g_sMenuSound_OK[] = "buttons/button14.wav";

new const String:g_sNadeType[][] = {
  "FRAG",
  "FLASH"
};

Handle g_hCookie_FlashColor;
Handle g_hCookie_FlashColorName;

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

// Purpose: Format and send a chat message to all assault players
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

  // 2.55 * 100 = full 255 alpha
  int alpha = RoundToNearest(2.55 * intensity);
  if (alpha < 5)
    alpha = 5;

  float volume = 0.007 * intensity;
  if (volume < 0.1)
    volume = 0.1;

  Handle userMsg = StartMessageOne("Fade", client);
  BfWriteShort(userMsg, 500); // Flash duration
  BfWriteShort(userMsg, resetDuration); // View reset duration (ms times 2??)
  BfWriteShort(userMsg, 0x0001); // Fade in flag
  BfWriteByte(userMsg, g_iFlashColor[client][RGB_RED]);
  BfWriteByte(userMsg, g_iFlashColor[client][RGB_GREEN]);
  BfWriteByte(userMsg, g_iFlashColor[client][RGB_BLUE]);
  BfWriteByte(userMsg, alpha);
  EndMessage();

  EmitSoundToClient(client,
    g_sFlashSound_Victim, _, _, SNDLEVEL_NORMAL, _, volume, 200);

  Spectators_ShowBlindHint(client, intensity);
}

// Purpose: Display a HUD hint over flashed player's head for spectators.
void Spectators_ShowBlindHint(int client, int intensity)
{
  if (!IsValidClient(client))
    return;

  float position[3];
  GetClientEyePosition(client, position);
  position[2] += 32;

  if (intensity < 100)
  {
    TE_SetupGlowSprite(position, g_iSpecBlindHint_Half, 3.5, 0.1, 150);
  }
  else
  {
    TE_SetupGlowSprite(position, g_iSpecBlindHint_Full, 3.5, 0.1, 150);
  }

  for (int i = 1; i <= MaxClients; i++)
  {
    if (!IsValidClient(i) || IsFakeClient(i))
      continue;

    int team = GetClientTeam(i);
    if (IsPlayerAlive(i) || team != TEAM_SPECTATOR)
      continue;

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
    for (int j = PITCH; j < ROLL; j++)
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
    float flashAvoidance_Y = (angle[PITCH] * basePercentile);
    float flashAvoidance_X = (angle[YAW] * basePercentile);

    //PrintToServer("Angles %f %f", angle[PITCH], angle[YAW]);
    if (angle[PITCH] >= 75 || angle[YAW] >= 75)
    {
      //PrintToServer("Reducing from %f", flashedPercent);
      flashedPercent -= flashAvoidance_Y;
      flashedPercent -= flashAvoidance_X;
      //PrintToServer("to %f", flashedPercent);
    }
    else
    {
      //PrintToServer("False, %f >= 90 || %f >= 90", angle[PITCH], angle[YAW]);
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
      eyeAngles[PITCH], eyeAngles[YAW],
      vecDir[0], vecDir[1],
      angle[PITCH], angle[YAW]);

    PrintToChatAll("Angles: %f %f", angle[PITCH], angle[YAW])

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

void GetClientPreferences(int client)
{
  if (!IsValidClient(client) || !AreClientCookiesCached(client))
    return;

  OnClientCookiesCached(client);
}

// Purpose: Reset flash color for client index.
// Color is reset back to default white, in case the
// player has no color cookie set or it can't be read.
void ResetClientPreferences(int client, bool resetColorName = true)
{
  if (client < 1 || client > MaxClients)
    ThrowError("Out of bounds client index %i", client);

  for (int i = 0; i < sizeof(g_iFlashColor[]); i++)
  {
    g_iFlashColor[client][i] = 255;
  }

  if (resetColorName)
  {
    strcopy(g_sFlashColorName[client], sizeof(g_sFlashColorName[]), "");
  }
}

// Purpose: Convert a "<R> <G> <B>" string to integer array
int[] GetRGB(const char[] sRGB)
{
  int rgb[RGB_ENUM_COUNT];
  decl String:rgbBuffer[RGB_ENUM_COUNT][4];
  ExplodeString(sRGB, " ", rgbBuffer, sizeof(rgbBuffer), sizeof(rgbBuffer[]));

  for (int i = 0; i < sizeof(rgbBuffer); i++)
  {
    rgb[i] = CapRGBValue(StringToInt(rgbBuffer[i]));
  }

  return rgb;
}

int CapRGBValue(int value)
{
  if (value < 0)
  {
    return 0;
  }
  else if (value > 255)
  {
    return 255;
  }

  return value;
}

void PreviewFlash(int client, bool:previewCurrent = false)
{
  // Block command spam or overriding real flashes with this
  if (g_bIsForbiddenVision[client])
  {
    PrintToChat(client, "[SM] Please wait.");
    return;
  }
  g_bIsForbiddenVision[client] = true;

  int rgb[RGB_ENUM_COUNT];
  char colorName[MAX_COLOR_STRLEN];

  if (previewCurrent)
  {
    char rgbBuffer[MAX_RGB_STRLEN];
    GetClientCookie(client, g_hCookie_FlashColor, rgbBuffer, sizeof(rgbBuffer));
    rgb = GetRGB(rgbBuffer);

    GetClientCookie(client, g_hCookie_FlashColorName, colorName, sizeof(colorName));
  }
  else
  {
    rgb = g_iColor_Preset[g_iFlashPreview[client]];

    strcopy(colorName, sizeof(colorName),
      g_sColorName_Preset[g_iFlashPreview[client]]);
  }

  int resetDuration = 2000;
  int alpha = 150;
  Handle msgOut = StartMessageOne("Fade", client);
  BfWriteShort(msgOut, 500); // Flash duration
  BfWriteShort(msgOut, resetDuration); // View reset duration (ms times 2??)
  BfWriteShort(msgOut, 0x0002); // Fade out flag
  BfWriteByte(msgOut, rgb[RGB_RED]);
  BfWriteByte(msgOut, rgb[RGB_GREEN]);
  BfWriteByte(msgOut, rgb[RGB_BLUE]);
  BfWriteByte(msgOut, alpha);
  EndMessage();

  int userid = GetClientUserId(client);

  DataPack pack = new DataPack();
  pack.WriteCell(userid);
  pack.WriteCell(previewCurrent);
  pack.WriteCell(alpha);

  CreateTimer(0.25 + resetDuration / 500.0, Timer_AllowVision, userid);
  CreateTimer(1.0, Timer_PreviewFlash_Remove, pack);

  PrintToChat(client, "[SM] Previewing %s (%i %i %i)",
    colorName,
    rgb[RGB_RED],
    rgb[RGB_GREEN],
    rgb[RGB_BLUE]);
}
