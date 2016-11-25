#if defined _timers_included_
  #endinput
#endif
#define _timers_included_

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

  // Make flash explosion sound at grenade position
  EmitSoundToAll(g_sFlashSound_Environment, entity, _, SNDLEVEL_GUNFIRE, _, 1.0);

  // Remove the grenade
  AcceptEntityInput(entity, "kill");

  // Draw explosion mark on environment
  TE_Start("BSP Decal");
  TE_WriteVector("m_vecOrigin", explosionPos);
  TE_WriteNum("m_nEntity", 0);
  TE_WriteNum("m_nIndex", g_iExplosionMark);
  TE_SendToAll();

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
