#if defined _flashbangs_trace_included_
  #endinput
#endif
#define _flashbangs_trace_included_

bool TraceHitEyes(int client, float[3] startPos, float[3] eyePos)
{
  Handle ray = TR_TraceRayFilterEx(
    startPos, eyePos, MASK_VISIBLE, RayType_EndPoint,
    TraceFilter_IsPlayer, client);

  int hitIndex = TR_GetEntityIndex(ray);
  delete ray;
  if (hitIndex != client) {
    //PrintToChatAll("TR_GetEntityIndex %i is not client %i", hitIndex, client);
    RenderRayTrace(false, startPos, eyePos);
    return false;
  }
  RenderRayTrace(true, startPos, eyePos);
  return true;
}

bool TraceHitBody(int client, float[3] startPos, float[3] eyePos)
{
  float clientMins[3];
  float clientMaxs[3];
  GetEntPropVector(client, Prop_Send, "m_vecMins", clientMins);
  GetEntPropVector(client, Prop_Send, "m_vecMaxs", clientMaxs);

  float bodyThreshold = 0.0;
  for (int i = 0; i < 3; i++)
  {
    clientMins[i] -= bodyThreshold;
    clientMaxs[i] += bodyThreshold;
  }
/*
mins -16.000000 -16.000000 0.000000
maxs 16.000000 16.000000 64.000000
*/

  //PrintToConsole(client, "mins %f %f %f", clientMins[0], clientMins[1], clientMins[2]);
  //PrintToConsole(client, "maxs %f %f %f", clientMaxs[0], clientMaxs[1], clientMaxs[2]);

  bool hitBody;

  PrintToConsole(client, "Hull tracing from %f %f %f to ",
    startPos[0], startPos[1], startPos[2]
  );

  PrintToConsole(client, "%f %f %f - %f %f %f",
    eyePos[0]+clientMins[0], eyePos[1]+clientMins[1], eyePos[2]+clientMins[2],
    eyePos[0]+clientMaxs[0], eyePos[1]+clientMaxs[1], eyePos[2]+clientMaxs[2]
  );

  Handle hull = TR_TraceHullEx(
    startPos, eyePos, clientMins, clientMaxs, MASK_VISIBLE);

  int hitIndex = TR_GetEntityIndex(hull);
  delete hull;
  if (hitIndex == client) {
    //RenderHullTrace(startPos, eyePos, clientMaxs);
    hitBody = true;
  }
  RenderHullTrace(hitBody, startPos, eyePos, clientMins, clientMaxs);
  return hitBody;
}

void RenderRayTrace(bool success, float[3] startPos, float[3] eyePos)
{
  int modelIndex = PrecacheModel("materials/sprites/laserbeam.vmt");
  int color[4];
  if (success)
    color = { 150, 255, 150, 200 };
  else
    color = { 255, 150, 150, 200 };

  TE_SetupBeamPoints(startPos, eyePos, modelIndex, modelIndex,
    0, 0, 10.0, 2.0, 5.0, 2, 1.0, color, 1);
  TE_SendToAll();
}

void RenderHullTrace(bool success, float[3] startPos, float[3] eyePos, float[3] mins, float[3] maxs)
{
  int modelIndex = PrecacheModel("materials/sprites/laserbeam.vmt");
  int color[4];
  float endBuffer_mins1[3];
  float endBuffer_mins2[3];
  float endBuffer_maxs1[3];
  float endBuffer_maxs2[3];
  
  if (success)
    color = { 150, 255, 150, 200 };
  else
    color = { 255, 150, 150, 200 };

  AddVectors(eyePos, mins, endBuffer_mins1);
  TE_SetupBeamPoints(startPos, endBuffer_mins1, modelIndex, modelIndex,
    0, 0, 10.0, 2.0, 5.0, 2, 1.0, color, 1);
  TE_SendToAll();

  SubtractVectors(eyePos, mins, endBuffer_mins2);
  TE_SetupBeamPoints(startPos, endBuffer_mins2, modelIndex, modelIndex,
    0, 0, 10.0, 2.0, 5.0, 2, 1.0, color, 1);
  TE_SendToAll();

  AddVectors(eyePos, maxs, endBuffer_maxs1);
  TE_SetupBeamPoints(startPos, endBuffer_maxs1, modelIndex, modelIndex,
    0, 0, 10.0, 2.0, 5.0, 2, 1.0, color, 1);
  TE_SendToAll();

  SubtractVectors(eyePos, maxs, endBuffer_maxs2);
  TE_SetupBeamPoints(startPos, endBuffer_maxs2, modelIndex, modelIndex,
    0, 0, 10.0, 2.0, 5.0, 2, 1.0, color, 1);
  TE_SendToAll();

  TE_SetupBeamPoints(endBuffer_mins1, endBuffer_maxs1, modelIndex, modelIndex,
    0, 0, 10.0, 2.0, 5.0, 2, 1.0, color, 1);
  TE_SendToAll();

  TE_SetupBeamPoints(endBuffer_maxs1, endBuffer_mins2, modelIndex, modelIndex,
    0, 0, 10.0, 2.0, 5.0, 2, 1.0, color, 1);
  TE_SendToAll();

  TE_SetupBeamPoints(endBuffer_mins2, endBuffer_maxs2, modelIndex, modelIndex,
    0, 0, 10.0, 2.0, 5.0, 2, 1.0, color, 1);
  TE_SendToAll();

  TE_SetupBeamPoints(endBuffer_maxs2, endBuffer_mins1, modelIndex, modelIndex,
    0, 0, 10.0, 2.0, 5.0, 2, 1.0, color, 1);
  TE_SendToAll();

}

// Purpose: Check whether trace hit entity index equals desired client index.
// This means the ray hit this player.
public bool TraceFilter_IsPlayer(int hitEntity, int mask, int targetClient)
{
  if (hitEntity == targetClient) {
    //PrintToServer("hitEntity %i target %i = %b", hitEntity, targetClient, true);
    return true;
  }

  //PrintToServer("hitEntity %i target %i = %b", hitEntity, targetClient, false);
  return false;
}
