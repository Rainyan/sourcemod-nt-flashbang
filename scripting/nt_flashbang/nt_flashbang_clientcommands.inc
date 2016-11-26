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

  int rgb[3];
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

    int color = StringToInt(argBuffer);
    // Cap within the RGB range
    if (color < 0)
    {
      color = 0;
    }
    else if (color > 255)
    {
      color = 255;
    }

    rgb[i] = color;
  }

  decl String:cookieBuffer[11];
  Format(cookieBuffer, sizeof(cookieBuffer), "%i %i %i", rgb[0], rgb[1], rgb[2]);

  SetClientCookie(client, g_hCookie_FlashColor, cookieBuffer);

  ReplyToCommand(client, "RGB: %s", cookieBuffer);

  return Plugin_Handled;
}
