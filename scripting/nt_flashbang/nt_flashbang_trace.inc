#if defined _flashbangs_trace_included_
  #endinput
#endif
#define _flashbangs_trace_included_

bool TraceHitEyes(int client, float[3] startPos, float[3] eyePos)
{
  Handle ray = TR_TraceRayFilterEx(
    startPos, eyePos, MASK_VISIBLE, RayType_EndPoint, TraceFilter, client);

  int hitIndex = TR_GetEntityIndex(ray);
  delete ray;
  if (hitIndex != client) {
    //PrintToChatAll("TR_GetEntityIndex %i is not client %i", hitIndex, client);
    return false;
  }

  return true;
}

// Purpose: Use filter to specifically allow or disallow trace collision
// with surfaces/props that have unexpected visibility mask
public bool TraceFilter(int hitEntity, int mask, any targetClient)
{
  if (!IsValidModel(hitEntity))
    return false;

  decl String:modelname[24];
  GetEntPropString(hitEntity, Prop_Data,
    "m_ModelName", modelname, sizeof(modelname));

  for (int i = 0; i < sizeof(g_sFilterWhitelist); i++)
  {
    PrintToServer("Comparing %s vs %s", modelname, g_sFilterWhitelist[i]);
    if (StrEqual(modelname, g_sFilterWhitelist[i]))
    {
      return true;
    }
  }

  //PrintToServer("hitEntity %i target %i = %b", hitEntity, targetClient, false);
  return false;
}
