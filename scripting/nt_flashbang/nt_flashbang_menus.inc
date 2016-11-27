#if defined _flashbangs_menus_included_
  #endinput
#endif
#define _flashbangs_menus_included_

void ShowMenu_Main(int client)
{
  if (!IsValidClient(client) || IsFakeClient(client))
    ThrowError("Client %i is invalid or a bot.");

  Panel panel = new Panel();

  decl String:title[24];
  Format(title, sizeof(title), "= Flashbangs (v%s) =", PLUGIN_VERSION);

  bool isEnabled = GetConVarBool(g_hCvar_Enabled);
  char status[][] = { "disabled", "enabled" };
  decl String:enabledBuffer[25];
  Format(enabledBuffer, sizeof(enabledBuffer), "Flashbangs are: %s",
    status[isEnabled]);

  panel.SetTitle(title);
  panel.DrawText(" ");
  panel.DrawText(enabledBuffer);
  panel.DrawText(" ");
  panel.DrawItem("Change color");
  panel.CurrentKey = 10;
  panel.DrawText(" ");
  panel.DrawItem("Exit");
  panel.Send(client, PanelHandler_Main, MENU_TIME_FOREVER);
  delete panel;
}

void ShowMenu_Color_Main(int client)
{
  if (!IsValidClient(client) || IsFakeClient(client))
    ThrowError("Client %i is invalid or a bot.");

  Menu menu = new Menu(MenuHandler_Color_Main);

  // Format panel title
  decl String:colorTitle[17 + MAX_RGB_STRLEN + MAX_COLOR_STRLEN];
  Format(colorTitle, sizeof(colorTitle), "Flash color: %i %i %i",
    g_iFlashColor[client][RGB_RED],
    g_iFlashColor[client][RGB_GREEN],
    g_iFlashColor[client][RGB_BLUE]);
  // Append color name if it exists
  if (strlen(g_sFlashColorName[client]) > 0)
  {
    Format(colorTitle, sizeof(colorTitle), "%s (%s)",
      colorTitle, g_sFlashColorName[client]);
  }

  menu.SetTitle(colorTitle);
  menu.AddItem("shade", "Choose new color");
  menu.AddItem("manual", "Manually set color RGB");
  menu.AddItem("preview", "Preview current color");
  menu.ExitButton = false;
  menu.ExitBackButton = true
  menu.Display(client, MENU_TIME_FOREVER);
}

void ShowMenu_Color_Preset(int client)
{
  if (!IsValidClient(client) || IsFakeClient(client))
    ThrowError("Client %i is invalid or a bot.");

  Menu menu = new Menu(MenuHandler_Color_Preset);
  menu.ExitButton = false;
  menu.ExitBackButton = true;

  int shade = g_iShadeHolder[client];

  menu.SetTitle("Choose flashbang color:");
  for (int i = 0; i < COLOR_ENUM_COUNT; i++)
  {
    menu.AddItem("shade", g_sPreset_Color[shade][i]);
  }
  menu.Display(client, MENU_TIME_FOREVER);
}

public int MenuHandler_Shade_Preset(Menu menu, MenuAction action, int client, int choice)
{
  if (action == MenuAction_End)
  {
    delete menu;
    return;
  }
  else if (action == MenuAction_Cancel && choice == MenuCancel_ExitBack)
  {
    ShowMenu_Main(client);
    return;
  }
  else if (action != MenuAction_Select)
  {
    return;
  }

  g_iShadeHolder[client] = choice;
  ShowMenu_Color_Preset(client);
}

void ShowMenu_Shade_Preset(int client)
{
  Menu menu = new Menu(MenuHandler_Shade_Preset);
  menu.ExitButton = false;
  menu.ExitBackButton = true;

  menu.SetTitle("Choose color group:");
  for (int i = 0; i < SHADE_ENUM_COUNT; i++)
  {
    menu.AddItem("color", g_sPreset_Shade[i]);
  }
  menu.Display(client, MENU_TIME_FOREVER);
}

void ShowMenu_Color_Preset_Preview(int client)
{
  int shade = g_iShadeHolder[client];
  int color = g_iColorHolder[client];

  decl String:colorBuffer[MAX_COLOR_STRLEN];
  Format(colorBuffer, sizeof(colorBuffer), "%i %i %i",
    g_iColor_Preset[shade][color][RGB_RED],
    g_iColor_Preset[shade][color][RGB_GREEN],
    g_iColor_Preset[shade][color][RGB_BLUE]);

  Panel panel = new Panel();
  panel.SetTitle("Color preview");
  panel.DrawText(" ");
  panel.DrawText(g_sPreset_Color[shade][color]);
  panel.DrawText(colorBuffer);
  panel.DrawText(" ");
  panel.DrawItem("Preview flash effect");
  panel.DrawItem("Save color");
  panel.DrawText(" ");
  panel.DrawItem("Back");
  panel.Send(client, PanelHandler_Color_Preset_Preview, MENU_TIME_FOREVER);
  delete panel;
}

void ShowMenu_Color_Preset_Commit(int client)
{
  int shade = g_iShadeHolder[client];
  int color = g_iColorHolder[client];

  // Convert RGB to string for cookie
  decl String:colorBuffer[MAX_COLOR_STRLEN];
  Format(colorBuffer, sizeof(colorBuffer), "%i %i %i",
    g_iColor_Preset[shade][color][RGB_RED],
    g_iColor_Preset[shade][color][RGB_GREEN],
    g_iColor_Preset[shade][color][RGB_BLUE]);

  // Store variables
  strcopy(g_sFlashColorName[client], sizeof(g_sFlashColorName[]), g_sPreset_Color[shade][color]);
  g_iFlashColor[client] = g_iColor_Preset[shade][color];

  // Store as cookies
  SetClientCookie(client, g_hCookie_FlashColor, colorBuffer);
  SetClientCookie(client, g_hCookie_FlashColorName, g_sFlashColorName[client]);

  PrintToChat(client, "[SM] Flashbang color set to %s.", g_sFlashColorName[client]);
  ShowMenu_Color_Main(client);
}

void ShowMenu_Color_Manual(int client)
{
  Panel panel = new Panel();
  panel.SetTitle("Manually set color RGB");
  panel.DrawText(" ");
  panel.DrawText("You can set the flashbang color");
  panel.DrawText("manually with the console command:");
  panel.DrawText("sm_flash_rgb <Red> <Green> <Blue> <Name>");
  panel.DrawText("where colors are a value between 0-255 and");
  panel.DrawText("optionally <Name> is your custom color name.");
  panel.DrawText("Names with spaces need quotes around them.");
  panel.DrawText(" ");
  panel.DrawText("For example: sm_flash_rgb 142 59 89 \"Quinacridone Magenta\"");
  panel.DrawText(" ");

  panel.CurrentKey = 8;
  panel.DrawItem("Back");

  panel.Send(client, PanelHandler_Color_Manual, MENU_TIME_FOREVER);
  delete panel;
}

public int PanelHandler_Main(Menu menu, MenuAction action, int client, int choice)
{
  if (action != MenuAction_Select)
    return;

  switch (choice)
  {
    // Color menu
    case 1:
    {
      EmitSoundToClient(client, g_sMenuSound_OK);
      ShowMenu_Color_Main(client);
    }
    case 10:
    {
      EmitSoundToClient(client, g_sMenuSound_Cancel);
    }
  }
}

public int MenuHandler_Color_Main(Menu menu, MenuAction action, int client, int choice)
{
  if (action == MenuAction_End)
  {
    delete menu;
    return;
  }
  else if (action == MenuAction_Cancel && choice == MenuCancel_ExitBack)
  {
    ShowMenu_Main(client);
    return;
  }
  else if (action != MenuAction_Select)
  {
    return;
  }

  if (choice == 0)
  {
    ShowMenu_Shade_Preset(client);
  }
  else if (choice == 1)
  {
    ShowMenu_Color_Manual(client);
  }
  else if (choice == 2)
  {
    PreviewFlash(client, true);
    ShowMenu_Color_Main(client);
  }
  else
  {
    ThrowError("Client %i picked unexpected menu choice %i", client, choice);
  }
}

public int MenuHandler_Color_Preset(Menu menu, MenuAction action, int client, int choice)
{
  if (action == MenuAction_End)
  {
    delete menu;
    return;
  }
  else if (action == MenuAction_Cancel && choice == MenuCancel_ExitBack)
  {
    ShowMenu_Shade_Preset(client);
    return;
  }
  else if (action != MenuAction_Select)
  {
    return;
  }

  if (choice < 0 || choice > COLOR_ENUM_COUNT)
  {
    PrintToChat(client, "[SM] Sorry, something went wrong!");
    ThrowError("Client %i chose an invalid color enum %i", client, choice);
  }

  g_iColorHolder[client] = choice;
  ShowMenu_Color_Preset_Preview(client);
}

public int PanelHandler_Color_Preset_Preview(Menu menu, MenuAction action, int client, int choice)
{
  if (action != MenuAction_Select)
    return;

  switch (choice)
  {
    // Preview flash color
    case 1:
    {
      if (!g_bIsForbiddenVision[client])
      {
        EmitSoundToClient(client, g_sMenuSound_OK);
      }
      PreviewFlash(client);
      ShowMenu_Color_Preset_Preview(client);
    }
    // Commit flash color change
    case 2:
    {
      EmitSoundToClient(client, g_sMenuSound_OK);
      ShowMenu_Color_Preset_Commit(client);
    }
    // Go back
    case 3:
    {
      EmitSoundToClient(client, g_sMenuSound_Cancel);
      ShowMenu_Color_Main(client);
    }
  }
}

public int PanelHandler_Color_Manual(Menu menu, MenuAction action, int client, int choice)
{
  if (action != MenuAction_Select)
    return;

  EmitSoundToClient(client, g_sMenuSound_Cancel);
  ShowMenu_Color_Main(client);
}
