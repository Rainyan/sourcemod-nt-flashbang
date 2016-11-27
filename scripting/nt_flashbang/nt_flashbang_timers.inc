#if defined _flashbangs_timers_included_
  #endinput
#endif
#define _flashbangs_timers_included_

// Purpose: Prevent nade switching after spawn freezetime, if desired
public Action Timer_CanModifyNade_Revoke(Handle timer)
{
  g_bCanModifyNade = false;
  Assaults_SendMessage("[SM] Flashbang choose time has expired.");
}

// Purpose: Only allow a few input checks per client per second for performance
public Action Timer_ModifyCooldown(Handle timer, int userid)
{
  int client = GetClientOfUserId(userid);
  g_bModifyCooldown[client] = false;
}

// Purpose: Turn a grenade into a flashbang by entity index
public Action Timer_Flashify(Handle timer, int entRef)
{
  int entity = EntRefToEntIndex(entRef);
  if (entity == INVALID_ENT_REFERENCE)
    return Plugin_Stop;

  float explosionPos[3];
  GetEntPropVector(entity, Prop_Send, "m_vecOrigin", explosionPos);

  // Remove the grenade
  AcceptEntityInput(entity, "kill");

  // Create temp dynamic light for flash effect
  TE_Start("Dynamic Light");
  TE_WriteVector("m_vecOrigin", explosionPos);
  TE_WriteNum("r", 255);
  TE_WriteNum("g", 255);
  TE_WriteNum("b", 255);
  TE_WriteNum("exponent", 1);
  TE_WriteFloat("m_fRadius", 512.0);
  TE_WriteFloat("m_fTime", 0.1);
  TE_WriteFloat("m_fDecay", 0.0);
  TE_SendToAll();

  // Draw explosion mark on environment
  TE_Start("BSP Decal");
  TE_WriteVector("m_vecOrigin", explosionPos);
  TE_WriteNum("m_nEntity", 0);
  TE_WriteNum("m_nIndex", g_iExplosionMark);
  TE_SendToAll();

  // Make flash explosion sound at grenade position
  EmitSoundToAll(g_sFlashSound_Environment, entity, _, SNDLEVEL_GUNFIRE, _, 1.0);

  // See if anyone gets blinded
  CheckIfFlashed(explosionPos);

  return Plugin_Handled;
}

// Purpose: Flip bool to allow player toggling
// their vision mode again after having been blinded
public Action Timer_AllowVision(Handle timer, int userid)
{
  int client = GetClientOfUserId(userid);
  if (!IsValidClient(client))
    return Plugin_Stop;

  //PrintToChat(client, "Fade expired");
  g_bIsForbiddenVision[client] = false;

  return Plugin_Handled;
}

// Purpose: Wait before fading away when
// previewing colors for a smoother fade in/out
public Action Timer_PreviewFlash_Remove(Handle timer, int userid)
{
  int client = GetClientOfUserId(userid);
  if (!IsValidClient(client))
    return Plugin_Stop;

  Handle msgIn = StartMessageOne("Fade", client);
  BfWriteShort(msgIn, 500); // Flash duration
  BfWriteShort(msgIn, 2000); // View reset duration (ms times 2??)
  BfWriteShort(msgIn, 0x0001); // Fade in flag
  BfWriteByte(msgIn, g_iColor_Preset[g_iFlashPreview[client]][RGB_RED]);
  BfWriteByte(msgIn, g_iColor_Preset[g_iFlashPreview[client]][RGB_GREEN]);
  BfWriteByte(msgIn, g_iColor_Preset[g_iFlashPreview[client]][RGB_BLUE]);
  BfWriteByte(msgIn, 255);
  EndMessage();

  return Plugin_Handled;
}
