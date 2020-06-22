/*-------------------------------------------------------------------------------

BARONY
File: lobbies.cpp
Desc: contains code for matchmaking handlers

Copyright 2013-2020 (c) Turning Wheel LLC, all rights reserved.
See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "menu.hpp"
#include "game.hpp"
#include "lobbies.hpp"
#include "eos.hpp"
#include "steam.hpp"
#include "draw.hpp"
#include "player.hpp"

LobbyHandler_t LobbyHandler;

void LobbyHandler_t::handleLobbyListRequests()
{
	if ( !intro || connectionType == LOBBY_DISABLE )
	{
		return;
	}

#if !defined STEAMWORKS && !defined USE_EOS
	return;
#endif


	if ( connectionType == LOBBY_STEAM )
	{
#ifdef STEAMWORKS
		// lobby list request succeeded
		if ( !requestingLobbies && !strcmp(subtext, language[1132]) )
		{
			openSteamLobbyBrowserWindow(NULL);
		}

		// lobby entered
		if ( !connectingToLobby && connectingToLobbyWindow )
		{
			connectingToLobbyWindow = false;
			connectingToLobby = false;

			// close current window
			buttonCloseSubwindow(NULL);
			list_FreeAll(&button_l);
			deleteallbuttons = true;

			// we are assuming here that the lobby join was successful
			// otherwise, the callback would've flipped off the connectingToLobbyWindow and opened an error window

			// get number of lobby members (capped to game limit)

			// record CSteamID of lobby owner (and nobody else)
			int lobbyMembers = SteamMatchmaking()->GetNumLobbyMembers(*static_cast<CSteamID*>(currentLobby));
			if ( steamIDRemote[0] )
			{
				cpp_Free_CSteamID(steamIDRemote[0]);
			}
			steamIDRemote[0] = cpp_SteamMatchmaking_GetLobbyOwner(currentLobby); //TODO: Bugger void pointers!
			int c;
			for ( c = 1; c < MAXPLAYERS; c++ )
			{
				if ( steamIDRemote[c] )
				{
					cpp_Free_CSteamID(steamIDRemote[c]);
					steamIDRemote[c] = NULL;
				}
			}
			for ( c = 1; c < lobbyMembers; ++c )
			{
				steamIDRemote[c] = cpp_SteamMatchmaking_GetLobbyMember(currentLobby, c);
			}
			buttonJoinLobby(NULL);
			// TODO - what if the server never replies? hangs indefinitely.
		}
#endif
	}
	else if ( connectionType == LOBBY_CROSSPLAY )
	{
#ifdef USE_EOS
		// lobby list request succeeded
		if ( !EOS.bRequestingLobbies && !strcmp(subtext, language[1132]) )
		{
			openSteamLobbyBrowserWindow(NULL);
		}

		// lobby entered
		if ( EOS.ConnectingToLobbyStatus != static_cast<int>(EOS_EResult::EOS_Success) )
		{
			// close current window
			buttonCloseSubwindow(NULL);
			list_FreeAll(&button_l);
			deleteallbuttons = true;

			openFailedConnectionWindow(CLIENT);
			strcpy(subtext, EOSFuncs::getLobbyJoinFailedConnectString(EOS.ConnectingToLobbyStatus).c_str());
			EOS.ConnectingToLobbyStatus = static_cast<int>(EOS_EResult::EOS_Success);
		}
		else if ( !EOS.bConnectingToLobby && EOS.bConnectingToLobbyWindow )
		{
			EOS.bConnectingToLobbyWindow = false;
			EOS.bConnectingToLobby = false;

			// close current window
			buttonCloseSubwindow(NULL);
			list_FreeAll(&button_l);
			deleteallbuttons = true;

			// we are assuming here that the lobby join was successful
			// otherwise, the callback would've flipped off the connectingToLobbyWindow and opened an error window
			buttonJoinLobby(NULL);
		}
	}
#endif
}

void LobbyHandler_t::updateSearchResults()
{
	numLobbyDisplaySearchResults = 0;
	if ( searchType == LOBBY_STEAM )
	{
#ifdef STEAMWORKS
		Uint32 steamLobbyIndex = 0;
		for ( auto& result : lobbyDisplayedSearchResults )
		{
			if ( steamLobbyIndex >= MAX_STEAM_LOBBIES || steamLobbyIndex >= numSteamLobbies )
			{
				result.first = -1;
				result.second = LOBBY_DISABLE;
				continue;
			}
			if ( lobbyIDs[steamLobbyIndex] )
			{
				result.first = steamLobbyIndex;
				result.second = LOBBY_STEAM;
				++numLobbyDisplaySearchResults;
			}
			++steamLobbyIndex;
		}
#else
		for ( auto& result : lobbyDisplayedSearchResults )
		{
			result.first = -1;
			result.second = LOBBY_DISABLE;
		}
#endif
	}
	else if ( searchType == LOBBY_CROSSPLAY )
	{
#ifdef USE_EOS
		Uint32 eosLobbyIndex = 0;
		const Uint32 numEOSLobbies = EOS.LobbySearchResults.resultsSortedForDisplay.size();
		for ( auto& result : lobbyDisplayedSearchResults )
		{
			if ( eosLobbyIndex >= numEOSLobbies )
			{
				result.first = -1;
				result.second = LOBBY_DISABLE;
				continue;
			}
			result.first = eosLobbyIndex;
			result.second = LOBBY_CROSSPLAY;
			++numLobbyDisplaySearchResults;
			++eosLobbyIndex;
		}
#else
		for ( auto& result : lobbyDisplayedSearchResults )
		{
			result.first = -1;
			result.second = LOBBY_DISABLE;
		}
#endif // USE_EOS
	}
	else if ( searchType == LOBBY_COMBINED )
	{
#if defined STEAMWORKS && defined USE_EOS
		Uint32 steamLobbyIndex = 0;
		Uint32 eosLobbyIndex = 0;
		const Uint32 numEOSLobbies = EOS.LobbySearchResults.resultsSortedForDisplay.size();
		for ( auto& result : lobbyDisplayedSearchResults )
		{
			if ( steamLobbyIndex >= MAX_STEAM_LOBBIES || steamLobbyIndex >= numSteamLobbies )
			{
				if ( eosLobbyIndex >= numEOSLobbies )
				{
					result.first = -1;
					result.second = LOBBY_DISABLE;
					continue;
				}
				else
				{
					result.first = eosLobbyIndex;
					result.second = LOBBY_CROSSPLAY;
					++numLobbyDisplaySearchResults;
					++eosLobbyIndex;
				}
			}
			else
			{
				if ( lobbyIDs[steamLobbyIndex] )
				{
					result.first = steamLobbyIndex;
					result.second = LOBBY_STEAM;
					++numLobbyDisplaySearchResults;
				}
			}
			++steamLobbyIndex;
		}
#else
		for ( auto& result : lobbyDisplayedSearchResults )
		{
			result.first = -1;
			result.second = LOBBY_DISABLE;
		}
#endif
	}
}

LobbyHandler_t::LobbyServiceType LobbyHandler_t::getDisplayedResultLobbyType(int selection)
{
	if ( selection < 0 || selection >= kNumSearchResults )
	{
		return LOBBY_DISABLE;
	}
	return lobbyDisplayedSearchResults.at(selection).second;
}
Sint32 LobbyHandler_t::getDisplayedResultLobbyIndex(int selection)
{
	if ( selection < 0 || selection >= kNumSearchResults )
	{
		return -1;
	}
	return lobbyDisplayedSearchResults.at(selection).first;
}

void LobbyHandler_t::handleLobbyBrowser()
{
	updateSearchResults();

	// epic/steam lobby browser
	if ( subwindow && !strcmp(subtext, language[1334]) )
	{
		// draw backdrop for main list and slider

		SDL_Rect listExtents;
		listExtents.x = subx1 + 8;
		listExtents.y = suby1 + 24;
		listExtents.w = (subx2 - 32) - listExtents.x;
		listExtents.h = (suby2 - 64) - listExtents.y;

		SDL_Rect sliderExtents;
		sliderExtents.x = subx2 - 32;
		sliderExtents.y = suby1 + 24;
		sliderExtents.w = (subx2 - 8) - sliderExtents.x;
		sliderExtents.h = (suby2 - 64) - sliderExtents.y;

		drawDepressed(listExtents.x, listExtents.y,
			listExtents.x + listExtents.w, listExtents.y + listExtents.h);
		drawDepressed(sliderExtents.x, sliderExtents.y,
			sliderExtents.x + sliderExtents.w, sliderExtents.y + sliderExtents.h);

		int numSearchResults = numLobbyDisplaySearchResults;
		int maxLobbyResults = kNumSearchResults;
//#ifdef STEAMWORKS
//		numSearchResults += numSearchResults;
//		maxLobbyResults += MAX_STEAM_LOBBIES;
//#endif // STEAMWORKS
//#ifdef USE_EOS
//		numSearchResults += EOS.LobbySearchResults.resultsSortedForDisplay.size();
//		maxLobbyResults += EOS.kMaxLobbiesToSearch;
//#endif // USE_EOS


		// slider
		slidersize = std::min<int>(((suby2 - 65) - (suby1 + 25)), ((suby2 - 65) - (suby1 + 25)) / ((real_t)std::max(numSearchResults + 1, 1) / 20));
		slidery = std::min(std::max(suby1 + 25, slidery), suby2 - 65 - slidersize);
		drawWindowFancy(subx2 - 31, slidery, subx2 - 9, slidery + slidersize);

		// directory list offset from slider
		Sint32 y2 = ((real_t)(slidery - suby1 - 20) / ((suby2 - 52) - (suby1 + 20))) * (numSearchResults + 1);
		if ( mousestatus[SDL_BUTTON_LEFT] 
			&& (omousex >= sliderExtents.x && omousex < sliderExtents.x + sliderExtents.w)
			&& (omousey >= sliderExtents.y && omousey < sliderExtents.y + sliderExtents.h) )
		{
			slidery = oslidery + mousey - omousey;
		}
		else if ( mousestatus[SDL_BUTTON_WHEELUP] || mousestatus[SDL_BUTTON_WHEELDOWN] )
		{
			slidery += 16 * mousestatus[SDL_BUTTON_WHEELDOWN] - 16 * mousestatus[SDL_BUTTON_WHEELUP];
			mousestatus[SDL_BUTTON_WHEELUP] = 0;
			mousestatus[SDL_BUTTON_WHEELDOWN] = 0;
		}
		else
		{
			oslidery = slidery;
		}
		slidery = std::min(std::max(suby1 + 25, slidery), suby2 - 65 - slidersize);
		y2 = ((real_t)(slidery - suby1 - 20) / ((suby2 - 52) - (suby1 + 20))) * (numSearchResults + 1);

		// server flags tooltip variables
		SDL_Rect flagsBox;
		char flagsBoxText[256];
		int hoveringSelection = -1;

		// select/inspect lobbies
		if ( (omousex >= listExtents.x && omousex < listExtents.x + listExtents.w)
			&& (omousey >= listExtents.y + 2 && omousey < listExtents.y + listExtents.h) )
		{
			//Something is flawed somewhere in here, because commit 1bad2c5d9f67e0a503ca79f93b03101fbcc7c7ba had to fix the game using an inappropriate hoveringSelection.
			//Perhaps it's as simple as setting hoveringSelection back to -1 if lobbyIDs[hoveringSelection] is in-fact null.
			hoveringSelection = std::min(std::max(0, y2 + ((omousey - suby1 - 24) >> 4)), maxLobbyResults);

			LobbyServiceType lobbyType = getDisplayedResultLobbyType(hoveringSelection);
			// lobby info tooltip
			if ( lobbyType == LOBBY_STEAM )
			{
				Sint32 lobbyIndex = getDisplayedResultLobbyIndex(hoveringSelection);
				// lobby info tooltip
				if ( lobbyIndex >= 0 && lobbyIDs[lobbyIndex] )
				{
					const char* lobbySvFlagsChar = SteamMatchmaking()->GetLobbyData(*static_cast<CSteamID*>(lobbyIDs[lobbyIndex]), "svFlags");
					Uint32 lobbySvFlags = atoi(lobbySvFlagsChar);

					int numSvFlags = 0;
					for ( int c = 0; c < NUM_SERVER_FLAGS; ++c )
					{
						if ( lobbySvFlags & power(2, c) )
						{
							++numSvFlags;
						}
					}

					const char* serverNumModsChar = SteamMatchmaking()->GetLobbyData(*static_cast<CSteamID*>(lobbyIDs[lobbyIndex]), "svNumMods");
					int serverNumModsLoaded = atoi(serverNumModsChar);

					flagsBox.x = mousex + 8;
					flagsBox.y = mousey + 8;
					flagsBox.w = strlen(language[2919]) * 10 + 4;
					flagsBox.h = 4 + (TTF_FontHeight(ttf12) * (std::max(2, numSvFlags + 2)));
					if ( serverNumModsLoaded > 0 )
					{
						flagsBox.h += TTF12_HEIGHT;
						flagsBox.w += 16;
					}
					strcpy(flagsBoxText, language[1335]);
					strcat(flagsBoxText, "\n");

					if ( !numSvFlags )
					{
						strcat(flagsBoxText, language[1336]);
					}
					else
					{
						int y = 2;
						for ( int c = 0; c < NUM_SERVER_FLAGS; c++ )
						{
							if ( lobbySvFlags & power(2, c) )
							{
								y += TTF_FontHeight(ttf12);
								strcat(flagsBoxText, "\n");
								char flagStringBuffer[256] = "";
								if ( c < 5 )
								{
									strcpy(flagStringBuffer, language[153 + c]);
								}
								else
								{
									strcpy(flagStringBuffer, language[2917 - 5 + c]);
								}
								strcat(flagsBoxText, flagStringBuffer);
							}
						}
					}
					if ( serverNumModsLoaded > 0 )
					{
						strcat(flagsBoxText, "\n");
						char numModsBuffer[32];
						snprintf(numModsBuffer, 32, "%2d mod(s) loaded", serverNumModsLoaded);
						strcat(flagsBoxText, numModsBuffer);
					}
				}
				// selecting lobby
				if ( mousestatus[SDL_BUTTON_LEFT] )
				{
					mousestatus[SDL_BUTTON_LEFT] = 0;
					//selectedSteamLobby = lobbyIndex;
					this->selectedLobbyInList = hoveringSelection;
				}
			}
			else if ( lobbyType == LOBBY_CROSSPLAY )
			{
				Sint32 lobbyIndex = getDisplayedResultLobbyIndex(hoveringSelection);
				// lobby info tooltip
				if ( lobbyIndex >= 0 )
				{
					//const char* lobbySvFlagsChar = SteamMatchmaking()->GetLobbyData(*static_cast<CSteamID*>(lobbyIDs[lobbyIndex]), "svFlags");
					Uint32 lobbySvFlags = EOS.LobbySearchResults.getResultFromDisplayedIndex(lobbyIndex)->LobbyAttributes.serverFlags;

					int numSvFlags = 0;
					for ( int c = 0; c < NUM_SERVER_FLAGS; ++c )
					{
						if ( lobbySvFlags & power(2, c) )
						{
							++numSvFlags;
						}
					}

					//const char* serverNumModsChar = SteamMatchmaking()->GetLobbyData(*static_cast<CSteamID*>(lobbyIDs[lobbyIndex]), "svNumMods");
					int serverNumModsLoaded = EOS.LobbySearchResults.getResultFromDisplayedIndex(lobbyIndex)->LobbyAttributes.numServerMods;

					flagsBox.x = mousex + 8;
					flagsBox.y = mousey + 8;
					flagsBox.w = strlen(language[2919]) * 10 + 4;
					flagsBox.h = 4 + (TTF_FontHeight(ttf12) * (std::max(2, numSvFlags + 2)));
					if ( serverNumModsLoaded > 0 )
					{
						flagsBox.h += TTF12_HEIGHT;
						flagsBox.w += 16;
					}
					strcpy(flagsBoxText, language[1335]);
					strcat(flagsBoxText, "\n");

					if ( !numSvFlags )
					{
						strcat(flagsBoxText, language[1336]);
					}
					else
					{
						int y = 2;
						for ( int c = 0; c < NUM_SERVER_FLAGS; c++ )
						{
							if ( lobbySvFlags & power(2, c) )
							{
								y += TTF_FontHeight(ttf12);
								strcat(flagsBoxText, "\n");
								char flagStringBuffer[256] = "";
								if ( c < 5 )
								{
									strcpy(flagStringBuffer, language[153 + c]);
								}
								else
								{
									strcpy(flagStringBuffer, language[2917 - 5 + c]);
								}
								strcat(flagsBoxText, flagStringBuffer);
							}
						}
					}
					if ( serverNumModsLoaded > 0 )
					{
						strcat(flagsBoxText, "\n");
						char numModsBuffer[32];
						snprintf(numModsBuffer, 32, "%2d mod(s) loaded", serverNumModsLoaded);
						strcat(flagsBoxText, numModsBuffer);
					}
				}
				// selecting lobby
				if ( mousestatus[SDL_BUTTON_LEFT] )
				{
					mousestatus[SDL_BUTTON_LEFT] = 0;
					//EOS.LobbySearchResults.selectedLobby = lobbyIndex;
					this->selectedLobbyInList = hoveringSelection;
				}
			}
		}

		// draw lobby list and selected window
		if ( this->selectedLobbyInList >= 0 )
		{
			LobbyServiceType lobbyType = getDisplayedResultLobbyType(this->selectedLobbyInList);
			if ( lobbyType == LOBBY_STEAM )
			{
#ifdef STEAMWORKS
				selectedSteamLobby = std::max(0, getDisplayedResultLobbyIndex(this->selectedLobbyInList));
				selectedSteamLobby = std::min(std::max(y2, selectedSteamLobby), std::min(std::max(numSearchResults - 1, 0), y2 + 17));
#endif
			}
			else if ( lobbyType == LOBBY_CROSSPLAY )
			{
				EOS.LobbySearchResults.selectedLobby = std::max(0, getDisplayedResultLobbyIndex(this->selectedLobbyInList));
				EOS.LobbySearchResults.selectedLobby = std::min(std::max(y2, EOS.LobbySearchResults.selectedLobby), std::min(std::max(static_cast<int>(numSearchResults) - 1, 0), y2 + 17));
			}
			SDL_Rect pos;
			pos.x = subx1 + 10;
			pos.y = suby1 + 26 + (this->selectedLobbyInList - y2) * 16;
			pos.w = subx2 - subx1 - 44;
			pos.h = 16;
			drawRect(&pos, SDL_MapRGB(mainsurface->format, 64, 64, 64), 255);
		}

		// print all lobby entries
		Sint32 x = subx1 + 10;
		Sint32 y = suby1 + 28;
		if ( numLobbyDisplaySearchResults > 0 )
		{
			Sint32 z = 0;
			int searchResultLowestVisibleEntry = std::min(numLobbyDisplaySearchResults, static_cast<Uint32>(18 + y2));
			for ( z = y2; z < searchResultLowestVisibleEntry; ++z )
			{
				LobbyServiceType lobbyType = getDisplayedResultLobbyType(z);
				Sint32 lobbyIndex = getDisplayedResultLobbyIndex(z);
				if ( lobbyType == LOBBY_STEAM )
				{
					ttfPrintTextFormatted(ttf12, x, y, lobbyText[lobbyIndex]); // name
					ttfPrintTextFormatted(ttf12, subx2 - 72, y, "%d/4", lobbyPlayers[lobbyIndex]); // player count
				}
				else if ( lobbyType == LOBBY_CROSSPLAY )
				{
					char buf[1024] = "";
					strcpy(buf, EOS.LobbySearchResults.getResultFromDisplayedIndex(lobbyIndex)->LobbyAttributes.lobbyName.c_str());
					ttfPrintTextFormatted(ttf12, x, y, buf); // name
					ttfPrintTextFormatted(ttf12, subx2 - 72, y, "%d/%d",
						EOS.LobbySearchResults.getResultFromDisplayedIndex(lobbyIndex)->playersInLobby.size(),
						EOS.LobbySearchResults.getResultFromDisplayedIndex(lobbyIndex)->MaxPlayers); // player count
				}
				y += 16;
			}
		}
		else
		{
			ttfPrintText(ttf12, x, y, language[1337]);
		}

		// draw server flags tooltip (if applicable)
		if ( hoveringSelection >= 0 && numLobbyDisplaySearchResults > 0 && (hoveringSelection < numLobbyDisplaySearchResults) )
		{
			drawTooltip(&flagsBox);
			ttfPrintTextFormatted(ttf12, flagsBox.x + 2, flagsBox.y + 4, flagsBoxText);
		}
	}
//
//#ifdef STEAMWORKS
//	if ( subwindow && !strcmp(subtext, language[1334]) )
//	{
//		
//	}
//#elif defined USE_EOS
//	if ( subwindow && !strcmp(subtext, language[1334]) )
//	{
//		// select/inspect lobbies
//		if ( omousex >= subx1 + 8 && omousex < subx2 - 32 && omousey >= suby1 + 26 && omousey < suby2 - 64 )
//		{
//			hoveringSelection = std::min(std::max(0, y2 + ((omousey - suby1 - 24) >> 4)), EOS.kMaxLobbiesToSearch);
//
//			
//
//			// selecting lobby
//			if ( mousestatus[SDL_BUTTON_LEFT] )
//			{
//				mousestatus[SDL_BUTTON_LEFT] = 0;
//				EOS.LobbySearchResults.selectedLobby = hoveringSelection;
//			}
//		}
//		EOS.LobbySearchResults.selectedLobby = std::min(std::max(y2, EOS.LobbySearchResults.selectedLobby), std::min(std::max(static_cast<int>(numSearchResults) - 1, 0), y2 + 17));
//		SDL_Rect pos;
//		pos.x = subx1 + 10;
//		pos.y = suby1 + 26 + (EOS.LobbySearchResults.selectedLobby - y2) * 16;
//		pos.w = subx2 - subx1 - 44;
//		pos.h = 16;
//		drawRect(&pos, SDL_MapRGB(mainsurface->format, 64, 64, 64), 255);
//
//		// print all lobby entries
//		Sint32 x = subx1 + 10;
//		Sint32 y = suby1 + 28;
//		if ( numSearchResults > 0 )
//		{
//			Sint32 z;
//			int searchResultLowestVisibleEntry = std::min(static_cast<int>(numSearchResults), 18 + y2);
//			for ( z = y2; z < searchResultLowestVisibleEntry; z++ )
//			{
//				char temporary[1024] = "";
//				strcpy(temporary, EOS.LobbySearchResults.getResultFromDisplayedIndex(z)->LobbyAttributes.lobbyName.c_str());
//				ttfPrintTextFormatted(ttf12, x, y, temporary); // name
//				ttfPrintTextFormatted(ttf12, subx2 - 72, y, "%d/%d",
//					EOS.LobbySearchResults.getResultFromDisplayedIndex(z)->playersInLobby.size(),
//					EOS.LobbySearchResults.getResultFromDisplayedIndex(z)->MaxPlayers); // player count
//				y += 16;
//			}
//		}
//		else
//		{
//			ttfPrintText(ttf12, x, y, language[1337]);
//		}
//
//		// draw server flags tooltip (if applicable)
//		if ( hoveringSelection >= 0
//			&& numSearchResults > 0
//			&& hoveringSelection < numSearchResults )
//		{
//			drawTooltip(&flagsBox);
//			ttfPrintTextFormatted(ttf12, flagsBox.x + 2, flagsBox.y + 4, flagsBoxText);
//		}
//	}
//#endif
}