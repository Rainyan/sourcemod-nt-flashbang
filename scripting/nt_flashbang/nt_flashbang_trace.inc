#if defined _flashbangs_trace_included_
  #endinput
#endif
#define _flashbangs_trace_included_

bool TraceHitEyes(int client, float[3] startPos, float[3] eyePos)
{
  Handle ray = TR_TraceRayFilterEx(
    startPos, eyePos, MASK_VISIBLE, RayType_EndPoint, TraceFilter_IsPlayer, client);

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
