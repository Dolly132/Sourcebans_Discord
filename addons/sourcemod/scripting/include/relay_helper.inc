#pragma semicolon 1
#pragma newdecls required

#include <discordWebhookAPI>

ConVar g_cvSteamAPI;

char g_sClientAvatar[MAXPLAYERS + 1][PLATFORM_MAX_PATH];

enum MessageType {
    Message_Type_Eban 	= 0,
    Message_Type_Eunban = 1,
    Message_Type_Lilac 	= 2,
    Message_Type_Kban 	= 3,
    Message_Type_Kunban = 4,
    Message_Type_Ban 	= 5,
    Message_Type_Mute 	= 6,
    Message_Type_Unmute = 7,
    Message_Type_Gag 	= 8,
    Message_Type_Ungag 	= 9
}

enum struct Global_Stuffs {
	ConVar enable;
	ConVar webhook;
	ConVar website;
}

stock void RelayHelper_PluginStart() {
	g_cvSteamAPI = CreateConVar(STEAM_API_CVAR, "", "API Web Steam. Get your own https://steamcommunity.com/dev/apikey", FCVAR_PROTECTED);
}

stock void GetClientSteamAvatar(int client) {
    char steamID[64];
    if(!GetClientAuthId(client, AuthId_SteamID64, steamID, sizeof(steamID))) {
        return;
    }
    
    char apiKey[PLATFORM_MAX_PATH];
    g_cvSteamAPI.GetString(apiKey, sizeof(apiKey));
    if(!apiKey[0]) {
        LogError("[%s] Invalid or no STEAM API Key specified in cfg/sourcemod/%s.cfg", PLUGIN_NAME, PLUGIN_NAME);
        return;
    }
    
    HTTPRequest request = new HTTPRequest("https://api.steampowered.com/ISteamUser/GetPlayerSummaries/v2/?");
    request.SetHeader("Accept", "application/json");
    request.AppendQueryParam("key", apiKey);
    request.AppendQueryParam("format", "json");
    request.AppendQueryParam("steamids", steamID);
    request.Get(OnSummaryReceived, GetClientUserId(client));
}

void OnSummaryReceived(HTTPResponse response, int userid) {
	if(response.Status != HTTPStatus_OK) {
		PrintToServer("Status is not ok, status is: %d", view_as<int>(response.Status));
		return;
	}
	
	JSONObject json = view_as<JSONObject>(response.Data); 
	if(json == null) {
		return;
	}
	
	JSONObject responseObj = view_as<JSONObject>(json.Get("response"));
	delete json;
	
	if(responseObj == null) {
		return;
	}
	
	JSONArray players = view_as<JSONArray>(responseObj.Get("players"));
	delete responseObj;
	
	if(players == null) {
		return;
	}
	
	int client = GetClientOfUserId(userid);
	if(client < 1) {
		delete players;
		return;
	}
		
	JSONObject player = view_as<JSONObject>(players.Get(0));
	delete players;
	
	player.GetString("avatarfull", g_sClientAvatar[client], sizeof(g_sClientAvatar[]));
	
	delete player;
}

stock void SendLilacDiscordMessage(int client, char[] header, char[] details, char[] cheat, char[] line, char[] connect, char[] webhookURL) {
//----------------------------------------------------------------------------------------------------
/* Generate the Webhook */
//----------------------------------------------------------------------------------------------------
	Webhook webhook = new Webhook("");
	webhook.SetUsername("ZE Little Anti-Cheat");
	webhook.SetAvatarURL("https://avatars.githubusercontent.com/u/110772618?s=200&v=4");
	
	Embed Embed_1 = new Embed(header, details);
	Embed_1.SetTimeStampNow();
	Embed_1.SetColor(0xf79337);
	
	EmbedThumbnail Thumbnail = new EmbedThumbnail();
	Thumbnail.SetURL(g_sClientAvatar[client]);
	Embed_1.SetThumbnail(Thumbnail);
	delete Thumbnail;
	
	EmbedField Field_2 = new EmbedField("Reason", cheat, true);
	Embed_1.AddField(Field_2);

	EmbedField Infos = new EmbedField("Extra Infos", line, false);
	Embed_1.AddField(Infos);

	EmbedField Connect = new EmbedField("Quick Connect", connect, true);
	Embed_1.AddField(Connect);
	
	EmbedFooter Footer = new EmbedFooter("");
	Footer.SetIconURL("https://github.githubassets.com/images/icons/emoji/unicode/1f440.png");
	Embed_1.SetFooter(Footer);
	delete Footer;

	// Generate the Embed
	webhook.AddEmbed(Embed_1);
	
	// Push the message
	DataPack pack = new DataPack();
	pack.WriteCell(view_as<int>(webhook));
	pack.WriteString(webhookURL);
	
	webhook.Execute(webhookURL, OnWebHookExecuted, pack);
}

stock void SendDiscordMessage(Global_Stuffs plugin, MessageType type, int admin, int target, int length, const char[] reason, int bansNumber, int commsNumber, const char[] targetName = "", const char[] avatar = "") {
	char steamID[20];
	char steamID64[32];
	
	// Admin Information
	if(!GetClientAuthId(admin, AuthId_Steam2, steamID, sizeof(steamID))) { // should never happen
		return;
	}
	
	if(!GetClientAuthId(admin, AuthId_SteamID64, steamID64, sizeof(steamID64))) { // should never happen
		return;
	}
	
	bool invalidTarget = false;
	if(target > MaxClients || target < 1 || !IsClientInGame(target)) {
		invalidTarget = true;
	}

	char targetName2[32];
	if(!invalidTarget && !GetClientName(target, targetName2, sizeof(targetName2))) {
		return;
	}
	
	char title[32];
	GetTypeTitle(type, title, sizeof(title));
	
	char embedHeader[68+32];
	FormatEx(embedHeader, sizeof(embedHeader), "%s for `%s`", title, (invalidTarget) ? targetName : targetName2);
	
	int color = GetTypeColor(type); 
	
	Embed Embed1 = new Embed(embedHeader);
	Embed1.SetColor(color);
	Embed1.SetTitle(embedHeader);
	Embed1.SetTimeStampNow();
	
	EmbedThumbnail Thumbnail = new EmbedThumbnail();
	Thumbnail.SetURL(avatar);
	Embed1.SetThumbnail(Thumbnail);
	delete Thumbnail;
	
	char adminInfo[PLATFORM_MAX_PATH * 2];
	Format(adminInfo, sizeof(adminInfo), "`%N` ([%s](https://steamcommunity.com/profiles/%s))", admin, steamID, steamID64);
	EmbedField field1 = new EmbedField("Admin:", adminInfo, false);
	Embed1.AddField(field1);
	
	// Player Information
	if(!GetClientAuthId(target, AuthId_Steam2, steamID, sizeof(steamID))) {
		strcopy(steamID, sizeof(steamID), "No SteamID");
	}
	
	if(!GetClientAuthId(target, AuthId_SteamID64, steamID64, sizeof(steamID64))) {
		strcopy(steamID64, sizeof(steamID64), "No SteamID");
	}
	
	char playerInfo[PLATFORM_MAX_PATH * 2];
	if(StrContains(steamID, "STEAM_") != -1) {
		Format(playerInfo, sizeof(playerInfo), "`%N` ([%s](https://steamcommunity.com/profiles/%s))", target, steamID, steamID64);
	} else {
		Format(playerInfo, sizeof(playerInfo), "`%N` (No SteamID)", target);
	}
	
	EmbedField field2 = new EmbedField("Player:", playerInfo, false);
	Embed1.AddField(field2);
	
	// Reason
	EmbedField field3 = new EmbedField("Reason:", reason, false);
	Embed1.AddField(field3);
 
	char webhookURL[PLATFORM_MAX_PATH];
	if(!GetWebhook(plugin, webhookURL, sizeof(webhookURL))) {
	    LogError("[Extended-Discord-Relay] Invalid or no webhook specified in plugin config! for %s", title);
	    return;
	}
	
	/* Duration */
	if(type != Message_Type_Kunban && type != Message_Type_Eunban && type != Message_Type_Unmute && type != Message_Type_Ungag) {
		char timeBuffer[128];
		switch (length)
		{
		    case -1:
		    {
		        FormatEx(timeBuffer, sizeof(timeBuffer), "Temporary");
		    }
		    case 0:
		    {
		        FormatEx(timeBuffer, sizeof(timeBuffer), "Permanent");
		    }
		    default:
		    {
		        int ctime = GetTime();
		        int finaltime = ctime + (length * 60);
		        FormatEx(timeBuffer, sizeof(timeBuffer), "%d Minute%s \n(to <t:%d:f>)", length, length > 1 ? "s" : "", finaltime);
		    }
		}
		
		EmbedField fieldDuration = new EmbedField("Duration:", timeBuffer, true);
		Embed1.AddField(fieldDuration);
	}
	
	/* History Field */
	if(StrContains(steamID, "STEAM_") != -1) {
		char history[PLATFORM_MAX_PATH * 4];
		FormatTypeHistory(plugin, type, steamID, bansNumber, commsNumber, history, sizeof(history));
		
		EmbedField field5 = new EmbedField("History:", history, false);
		Embed1.AddField(field5);
	}
	
	Webhook hook = new Webhook("");
	hook.AddEmbed(Embed1);
	
	DataPack pack = new DataPack();
	pack.WriteCell(view_as<int>(hook));
	pack.WriteString(webhookURL);
	hook.Execute(webhookURL, OnWebHookExecuted, pack);
}

void GetTypeTitle(MessageType type, char[] title, int maxlen) {
	switch(type) {
		case Message_Type_Kban: {
			strcopy(title, maxlen, "Kban");
		}
		
		case Message_Type_Kunban: {
			strcopy(title, maxlen, "Kunban");
		}
		
		case Message_Type_Eban: {
			strcopy(title, maxlen, "Eban");
		}
		
		case Message_Type_Eunban: {
			strcopy(title, maxlen, "Eunban");
		}
		
		case Message_Type_Ban: {
			strcopy(title, maxlen, "Ban");
		}
		
		case Message_Type_Mute: {
			strcopy(title, maxlen, "Mute");
		}
		
		case Message_Type_Unmute: {
			strcopy(title, maxlen, "Unmute");
		}
		
		case Message_Type_Gag: {
			strcopy(title, maxlen, "Gag");
		}
		
		default: {
			strcopy(title, maxlen, "Ungag");
		}
	}
	
	FormatEx(title, maxlen, "%s Notification", title);
}

int GetTypeColor(MessageType type) {
	switch(type) {
		case Message_Type_Kban: {
			return 0xffff00;
		}
		
		case Message_Type_Kunban: {
			return 0x0000ff;
		}
		
		case Message_Type_Eban: {
			return 0xffffff;
		}
		
		case Message_Type_Eunban: {
			return 0x00ffff;
		}
		
		case Message_Type_Ban: {
			return 0xff0000;
		}
		
		case Message_Type_Mute: {
			return 0x00ff00;
		}
		
		case Message_Type_Unmute: {
			return 0xff0fA0;
		}
		
		case Message_Type_Gag: {
			return 0xffA200;
		}
		
		default: {
			return 0xffC2A0;
		}
	}
}

bool GetWebhook(Global_Stuffs plugin, char[] url, int maxlen) {
	plugin.webhook.GetString(url, maxlen);
	
	if(!url[0]) {
		return false;
	}
		
	return true;
}

void FormatTypeHistory(Global_Stuffs plugin, MessageType type, const char[] steamID, int bansNumber, int commsNumber, char[] history, int maxlen) {
	if(StrContains(steamID, "STEAM_") == -1) {
		return;
	}
	
	char webURL[PLATFORM_MAX_PATH];
	GetTypeWebsiteURL(plugin, webURL, sizeof(webURL));
	
	// View History link
	if(type == Message_Type_Kban || type == Message_Type_Kunban) {
		FormatEx(webURL, sizeof(webURL), "%s?all=true&s=%s&m=1", webURL, steamID);
		FormatEx(history, maxlen, "%d Kbans ([View History](%s))", bansNumber, webURL);
		return;
	} else if(type == Message_Type_Eban || type == Message_Type_Eunban) {
		FormatEx(webURL, sizeof(webURL), "%s?all=true&s=%s&m=1", webURL, steamID);
		FormatEx(history, maxlen, "%d Ebans ([View History](%s))", bansNumber, webURL);
		return;
	} else {
		char webURL1[PLATFORM_MAX_PATH];
		FormatEx(webURL1, sizeof(webURL1), "%s?p=banlist&searchText=%s&Submit", webURL, steamID);
		FormatEx(history, maxlen, "%d Bans ([View Bans](%s))", bansNumber, webURL1);

		ReplaceString(webURL1, sizeof(webURL1), "banlist", "commslist", false);
		FormatEx(history, maxlen, "%s\n%d comms ([View Comms](%s))", history, commsNumber, webURL1);
		return;
	}
}

void GetTypeWebsiteURL(Global_Stuffs plugin, char[] url, int maxlen) {
	plugin.website.GetString(url, maxlen);
}

public void OnWebHookExecuted(HTTPResponse response, DataPack pack)
{
	static int retries;
	pack.Reset();
	Webhook hook = view_as<Webhook>(pack.ReadCell());
	
	if(response.Status != HTTPStatus_OK) {
		retries++;
		if(retries >= 3) {
			delete pack;
			delete hook;
			retries = 0;
			return;
		}
		
		PrintToServer("[%s] An error has occured while sending the webhook. resending the webhook again.", PLUGIN_NAME);
		
		char webhookURL[PLATFORM_MAX_PATH];
		pack.ReadString(webhookURL, sizeof(webhookURL));
		
		DataPack newPack;
		CreateDataTimer(0.5, ExecuteWebhook_Timer, newPack, TIMER_FLAG_NO_MAPCHANGE);
		newPack.WriteCell(view_as<int>(hook));
		newPack.WriteString(webhookURL);
		delete pack;
		return;
	}
	
	delete pack;
	delete hook;
}

Action ExecuteWebhook_Timer(Handle timer, DataPack pack) {
	pack.Reset();
	Webhook hook = view_as<Webhook>(pack.ReadCell());
	
	char webhookURL[PLATFORM_MAX_PATH];
	pack.ReadString(webhookURL, sizeof(webhookURL));
	
	DataPack newPack = new DataPack();
	newPack.WriteCell(view_as<int>(hook));
	newPack.WriteString(webhookURL);	
	hook.Execute(webhookURL, OnWebHookExecuted, newPack);
	return Plugin_Continue;
}
