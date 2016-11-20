#pragma semicolon 1

#include <sourcemod>
#include <neotokyo>

new const String:status[][] = {"disable", "enable"};

int g_iLastVoteEpoch;

Handle g_hCvar_Enabled;
Handle g_hCvar_Flashbang_Enabled;

public Plugin myinfo = {
  name = "NT Flashbangs Vote",
  description = "Allow players to vote for flashbangs",
  author = "Rain",
  version = "0.1",
  url = "https://github.com/Rainyan/sourcemod-nt-flashbang"
};

public void OnPluginStart()
{
  RegConsoleCmd("sm_voteflash", Command_VoteFlash);

  g_hCvar_Enabled = CreateConVar("sm_flashbang_vote_enabled", "1.0", "Toggle flashbang voting on/off", _, true, 0.0, true, 1.0);

  g_hCvar_Flashbang_Enabled = FindConVar("sm_flashbang_enabled");
  if (g_hCvar_Flashbang_Enabled == null)
    SetFailState("NT Flashbang plugin not found.");

  HookEvent("game_round_start", Event_RoundStart);
}

public Action Event_RoundStart(Handle event, const char[] name, bool dontBroadcast)
{
  PrintToChatAll("Turn frag grenades into flashbangs in currently: %sd", status[GetConVarBool(g_hCvar_Flashbang_Enabled)]);
  PrintToChatAll("You can toggle this with !voteflash");
}

public void OnConfigsExecuted()
{
  AutoExecConfig(true);
}

public Action Command_VoteFlash(int client, int args)
{
  if (!GetConVarBool(g_hCvar_Enabled))
  {
    ReplyToCommand(client, "Sorry, flashbang voting is currently disabled!");
    return Plugin_Stop;
  }

  Handle cvarVoteDelay = FindConVar("sm_vote_delay");
  int voteDelay = GetConVarInt(cvarVoteDelay);
  delete cvarVoteDelay;

  int timePassed = GetTime() - g_iLastVoteEpoch;
  if (timePassed < voteDelay)
  {
    ReplyToCommand(client, "You must wait at least %i seconds before another flashbang vote",
      voteDelay - timePassed);
    return Plugin_Stop;
  }

  bool flashEnabled = GetConVarBool(g_hCvar_Flashbang_Enabled);

  char menuTitle[20];
  Format(menuTitle, sizeof(menuTitle), "%s flashbangs?", status[!flashEnabled]);

  Menu menu = new Menu(MenuHandler_Vote);
  menu.SetTitle(menuTitle);
  menu.AddItem("Yes", "yes");
  menu.AddItem("No", "no");

  if (!VoteMenuToAll(menu, 10))
  {
    ReplyToCommand(client, "Vote is already running!");
    return Plugin_Stop;
  }

  char clientName[MAX_NAME_LENGTH];
  GetClientName(client, clientName, sizeof(clientName));

  PrintToChatAll("[SM] %s has initiated a %s flashbangs vote",
    clientName, status[!flashEnabled]);

  return Plugin_Handled;
}

public int MenuHandler_Vote(Menu menu, MenuAction action, int param1, int param2)
{
  if (action == MenuAction_End)
  {
    delete menu;
    return;
  }
  if (action != MenuAction_VoteEnd)
  {
    return;
  }

  g_iLastVoteEpoch = GetTime();

  // "Yes" in the visual menu = 0, "no" = 1
  bool result = !param1;
  bool flashEnabled = GetConVarBool(g_hCvar_Flashbang_Enabled);

  int winningVotes;
  int totalVotes;
  GetMenuVoteInfo(param2, winningVotes, totalVotes);

  if (!result)
  {
    PrintToChatAll("[SM] Flashbangs %s vote didn't pass (%i of %i voted against)",
      status[!flashEnabled], winningVotes, totalVotes);

    return;
  }

  ToggleFlashbangs();

  PrintToChatAll("[SM] Flashbangs have been %sd (%i of %i voted yes)",
    status[!flashEnabled], winningVotes, totalVotes);
}

void ToggleFlashbangs()
{
  bool enabled = GetConVarBool(g_hCvar_Flashbang_Enabled);
  SetConVarBool(g_hCvar_Flashbang_Enabled, !enabled);
}
