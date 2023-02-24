#pragma semicolon 1

#undef REQUIRE_PLUGIN
#tryinclude <sourcebanspp>
#tryinclude <sourcebanschecker>
#tryinclude <sourcecomms>
#define REQUIRE_PLUGIN

#define PLUGIN_NAME "Sourcebans_Discord"

#include <relay_helper>

#pragma newdecls required

Global_Stuffs g_Sbpp;

public Plugin myinfo =
{
	name 		= PLUGIN_NAME,
	author 		= ".Rushaway, Dolly, koen",
	version 	= "1.0",
	description = "Send Sourcebans Punishments notifications to discord",
	url 		= "https://nide.gg"
};

public void OnPluginStart() {
	g_Sbpp.enable 	= CreateConVar("sbpp_discord_enable", "1", "Toggle sourcebans notification system", _, true, 0.0, true, 1.0);
	g_Sbpp.webhook 	= CreateConVar("sbpp_discord", "", "The webhook URL of your Discord channel. (Sourcebans)", FCVAR_PROTECTED);
	g_Sbpp.website	= CreateConVar("sbpp_website", "https://bans.nide.gg/index.php", "Your sourcebans link", FCVAR_PROTECTED);
	
	RelayHelper_PluginStart();
	
	AutoExecConfig(true, PLUGIN_NAME);
	
	/* Incase of a late load */
	for(int i = 1; i <= MaxClients; i++) {
		if(!IsClientInGame(i) || IsFakeClient(i) || IsClientSourceTV(i) || g_sClientAvatar[i][0]) {
			return;
		}
		
		OnClientPostAdminCheck(i);
	}
}

public void OnClientPostAdminCheck(int client) {
	if(IsFakeClient(client) || IsClientSourceTV(client)) {
		return;
	}
	
	GetClientSteamAvatar(client);
}

public void OnClientDisconnect(int client) {
	g_sClientAvatar[client][0] = '\0';
}

#if defined _sourcebanspp_included
public void SBPP_OnBanPlayer(int admin, int target, int length, const char[] reason) {
	if(!g_Sbpp.enable.BoolValue) {
		return;
	}
	
	if(admin < 1) {
		return;
	}
	
	int bansNumber = 0;
	int commsNumber = 0;
	
	#if defined _sourcebanschecker_included
	bansNumber = SBPP_CheckerGetClientsBans(target);
	commsNumber = SBPP_CheckerGetClientsComms(target);
	bansNumber++;
	#endif
	
	SendDiscordMessage(g_Sbpp, Message_Type_Ban, admin, target, length, reason, bansNumber, commsNumber, _, g_sClientAvatar[target]);
}
#endif

#if defined _sourcecomms_included
public void SourceComms_OnBlockAdded(int admin, int target, int length, int commType, char[] reason) {
	if(!g_Sbpp.enable.BoolValue) {
		return;
	}
	
	if(admin < 1) {
		return;
	}
	
	MessageType type = Message_Type_Ban;
	switch(commType) {
		case TYPE_MUTE: {
			type = Message_Type_Mute;
		}
		
		case TYPE_UNMUTE: {
			type = Message_Type_Unmute;
		}
		
		case TYPE_GAG: {
			type = Message_Type_Gag;
		}
		
		case TYPE_UNGAG: {
			type = Message_Type_Ungag;
		}
	}
	
	if(type == Message_Type_Ban) {
		return;
	}
	
	int bansNumber = 0;
	int commsNumber = 0;
	
	#if defined _sourcebanschecker_included
	bansNumber = SBPP_CheckerGetClientsBans(target);
	commsNumber = SBPP_CheckerGetClientsComms(target);
	commsNumber++;
	#endif
	
	SendDiscordMessage(g_Sbpp, type, admin, target, length, reason, bansNumber, commsNumber, _, g_sClientAvatar[target]);
}
#endif
