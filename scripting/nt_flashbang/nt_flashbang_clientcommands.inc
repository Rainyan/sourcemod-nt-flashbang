#if defined _clientcommands_included_
  #endinput
#endif
#define _clientcommands_included_

public Action Command_FlashColor(int client, int args)
{/*
  Handle panel = CreatePanel();

  decl String:colorTitle[128];
  Format(colorTitle, sizeof(colorTitle), "Flash color: %i %i %i",
    g_iFlashColor[client][RGB_RED],
    g_iFlashColor[client][RGB_GREEN],
    g_iFlashColor[client][RGB_BLUE]);

  panel.SetPanelTitle(colorTitle);
  panel.DrawPanelText(" ");
  panel.DrawPanelText("You can manually set color with !flashbang_rgb,");
  panel.DrawPanelText("or pick one from the presets below.")
  panel.DrawPanelText(" ");

  panel.DrawPanelItem("White");
  panel.DrawPanelItem("Black");
  panel.DrawPanelItem("Light pink");
  panel.DrawPanelItem("Light yellow");
  panel.DrawPanelItem("Light aqua");

  panel.DrawPanelItem("Close menu");

  SendPanelToClient(panel, client, ColorMenu);
  delete panel;
*/
  return Plugin_Handled;
}

public int ColorMenu(Menu menu, MenuAction action, int client, int choice)
{
  return;
}

public Action Command_FlashColorRGB(int client, int args)
{
  if (args != RGB_ENUM_COUNT)
  {
    ReplyToCommand(client, "[SM] Usage: !flashcolor <R> <G> <B> (values 0-255)");
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

  ReplyToCommand(client, "RGB set to: %s", cookieBuffer);

  return Plugin_Handled;
}
