#if defined _clientcommands_included_
  #endinput
#endif
#define _clientcommands_included_

public Action Command_FlashColor(int client, int args)
{
  if (args != RGB_ENUM_COUNT)
  {
    ReplyToCommand(client, "[SM] Usage: !flashcolor <R> <G> <B> (values 0-255)");
    return Plugin_Stop;
  }

  int rgb[RGB_ENUM_COUNT];
  decl String:argBuffer[3];

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

  ReplyToCommand(client, "RGB: %s", cookieBuffer);

  return Plugin_Handled;
}
