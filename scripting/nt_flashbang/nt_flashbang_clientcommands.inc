#if defined _clientcommands_included_
  #endinput
#endif
#define _clientcommands_included_

public Action Command_FlashMenu(int client, int args)
{
  if (args)
  {
    ReplyToCommand(client, "[SM] Usage: !flash (opens an options menu)");
    return Plugin_Stop;
  }

  ShowMenu_Main(client);
  return Plugin_Handled;
}

public int ColorMenu(Menu menu, MenuAction action, int client, int choice)
{
  return;
}

public Action Command_FlashRGB(int client, int args)
{
  if (args != RGB_ENUM_COUNT && args != RGB_ENUM_COUNT + 1)
  {
    ReplyToCommand(client, "[SM] Your flash color is: %i %i %i",
      g_iFlashColor[client][RGB_RED],
      g_iFlashColor[client][RGB_GREEN],
      g_iFlashColor[client][RGB_BLUE]);
    ReplyToCommand(client, "Usage: !flash_rgb <Red> <Green> <Blue> <Name> \
with color values 0-255");

    return Plugin_Stop;
  }

  int rgb[RGB_ENUM_COUNT];
  decl String:argBuffer[4];

  // Convert the RGB args into numbers before
  // passing them to the flashbang color cookie.
  // This ensures the string based cookie will
  // always hold sane RGB integers capped within
  // the range 0-255. NaN will convert into a 0.
  for (int i = 0; i < sizeof(rgb); i++)
  {
    // n+1 because the 0th index holds the command itself
    GetCmdArg(i + 1, argBuffer, sizeof(argBuffer));
    rgb[i] = CapRGBValue(StringToInt(argBuffer));
  }

  decl String:cookieBuffer[MAX_RGB_STRLEN];
  Format(cookieBuffer, sizeof(cookieBuffer), "%i %i %i",
    rgb[RGB_RED], rgb[RGB_GREEN], rgb[RGB_BLUE]);

  SetClientCookie(client, g_hCookie_FlashColor, cookieBuffer);

  // Change values directly since cookie contents are currently known
  for (int i = 0; i < RGB_ENUM_COUNT; i++)
  {
    g_iFlashColor[client][i] = rgb[i];
  }

  // Set custom color name
  if (args == 4)
  {
    GetCmdArg(4, g_sFlashColorName[client], sizeof(g_sFlashColorName[]));
  }
  else if (strlen(g_sFlashColorName[client]) < 1)
  {
    strcopy(g_sFlashColorName[client], sizeof(g_sFlashColorName[]), "Custom");
  }

  ReplyToCommand(client, "[SM] Your flashbang color %s was set to: %s",
    g_sFlashColorName[client], cookieBuffer);

  return Plugin_Handled;
}
