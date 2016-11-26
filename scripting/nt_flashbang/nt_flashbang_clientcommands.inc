#if defined _clientcommands_included_
  #endinput
#endif
#define _clientcommands_included_

public Action Command_FlashColor(int client, int args)
{
  if (args != 3)
  {
    ReplyToCommand(client, "[SM] Usage: !flashcolor <R> <G> <B> (values 0-255)");
    return Plugin_Stop;
  }

  decl String:sArg[4];
  for (int i = 0; i < sizeof(g_iFlashColor[]); i++)
  {
    GetCmdArg(i + 1, sArg, sizeof(sArg));
    g_iFlashColor[client][i] = StringToInt(sArg);
  }

  ReplyToCommand(client, "RBG: %i %i %i",
    GetClientFlashColor(client, COLOR_R),
    GetClientFlashColor(client, COLOR_G),
    GetClientFlashColor(client, COLOR_B));

  return Plugin_Handled;
}
