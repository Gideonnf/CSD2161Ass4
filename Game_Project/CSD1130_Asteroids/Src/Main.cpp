/******************************************************************************/
/*!
\file		Main.cpp
\author 	Gideon Francis, g.francis, 2301207
\par    	email: g.francis@digipen.edu
\date   February 09 2024
\brief	The main core game loop! Runs through the different states for each scene to help intialize, randomise, 
and move positions

Copyright (C) 2024 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
 */
/******************************************************************************/

#include "main.h"
#include <memory>
#include <string>
#include "Windows.h"		// Entire Win32 API...
#include "winsock2.h"		// ...or Winsock alone
#include "ws2tcpip.h"		// getaddrinfo()
#include <filesystem>
#include <unordered_map>
#include <map>
#include <thread>
#include <filesystem>
#include <map>
#include <mutex>
#include <iostream>			// cout, cerr
#include <string>			// string
#include <vector>
#include <sstream>
#include <atomic>
#include <fstream>

//#define WINSOCK_VERSION     2
#define WINSOCK_SUBVERSION  2
#define MAX_STR_LEN         2048
#define RETURN_CODE_1       1
#define RETURN_CODE_2       2
#define RETURN_CODE_3       3
#define RETURN_CODE_4       4

// ---------------------------------------------------------------------------
// Globals
float	 g_dt;
double	 g_appTime;

/******************************************************************************/
/*!
	Starting point of the application
*/
/******************************************************************************/
int WINAPI WinMain(HINSTANCE instanceH, HINSTANCE prevInstanceH, LPSTR command_line, int show)
{
	UNREFERENCED_PARAMETER(prevInstanceH);
	UNREFERENCED_PARAMETER(command_line);

	// Enable run-time memory check for debug builds.
	/*#if defined(DEBUG) | defined(_DEBUG)
		_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	#endif*/

	// Initialize the system
	AESysInit (instanceH, show, 1280, 720, 1, 60, false, NULL);

	// Changing the window title
	AESysSetWindowTitle("Asteroids Demo!");

	//set background color
	AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

	// Initialize the fade screen obj variables

	FadeObject fadeScreen;
	fadeScreen.goActive = true;
	AEVec2Set(&fadeScreen.goScale, 1280, 720);
	AEVec2Set(&fadeScreen.goPosition, 1280, 720);
	fadeScreen.alpha = 1.0f;
	fadeScreen.fadeDuration = 0.5f;
	fadeScreen.fadeTimer = 0.0f;
	fadeScreen.toFade = false;

	AEGfxMeshStart();
	AEGfxTriAdd(
		-0.5f, -0.5f, 0xFF000000, 0.0f, 1.0f,  // bottom-left: red
		0.5f, -0.5f, 0xFF000000, 1.0f, 1.0f,   // bottom-right: green
		-0.5f, 0.5f, 0xFF000000, 0.0f, 0.0f);  // top-left: blue

	AEGfxTriAdd(
		0.5f, -0.5f, 0xFF000000, 1.0f, 1.0f,   // bottom-right: green
		0.5f, 0.5f, 0xFF000000, 1.0f, 0.0f,    // top-right: white
		-0.5f, 0.5f, 0xFF000000, 0.0f, 0.0f);  // top-left: blue

	fadeScreen.mesh = AEGfxMeshEnd();

	// Initialize the game state manager to menu st ate
	GameStateMgrInit(GS_MENU);

	while(gGameStateCurr != GS_QUIT)
	{
		// reset the system modules
		AESysReset();

		// If not restarting, load the gamestate
		if(gGameStateCurr != GS_RESTART)
		{
			GameStateMgrUpdate();
			GameStateLoad();
		}
		else
			gGameStateNext = gGameStateCurr = gGameStatePrev;

		// Initialize the gamestate
		GameStateInit();

		while(gGameStateCurr == gGameStateNext)
		{
			AESysFrameStart();

			// Update the current game state
			GameStateUpdate();
			// Render the game state
			GameStateDraw();

			// Fade in effect is done here
			RenderObj(AEFrameRateControllerGetFrameTime(), &fadeScreen);
			
			AESysFrameEnd();

			// check if forcing the application to quit
			if ((AESysDoesWindowExist() == false) || AEInputCheckTriggered(AEVK_ESCAPE))
				gGameStateNext = GS_QUIT;

			g_dt = (f32)AEFrameRateControllerGetFrameTime();
			g_appTime += g_dt;
		}
		// If the game isnt quitting, then start a fade out effect
		if (gGameStateNext != GS_QUIT)
		{
			// Fade out effect is done here
			fadeScreen.toFade = true;
			fadeScreen.fadeTimer = 0.0f;
			// Keep a render of the previous state scene while fading out
			while (fadeScreen.toFade) // Render the previouse state first so that theres an image as it fades
			{
				AESysFrameStart();
				// Render the objects
				GameStateDraw();

				// Render the fade obj
				RenderObj(AEFrameRateControllerGetFrameTime(), &fadeScreen);

				AESysFrameEnd();
			}
		}
		
		// Free any variables that the current state is using
		GameStateFree();

		// If it isnt restarting, then unload the assets
		if(gGameStateNext != GS_RESTART)
			GameStateUnload();

		// Set the states
		gGameStatePrev = gGameStateCurr;
		gGameStateCurr = gGameStateNext;
	}

	//Free the screen mesh
	AEGfxMeshFree(fadeScreen.mesh);
	// free the system
	AESysExit();
}

int InitClient()
{
	// read from text file
	std::string host{};
	std::string udpPortString;
	std::string clientUDPPortString;

	// -------------------------------------------------------------------------
	// Start up Winsock, asking for version 2.2.
	//
	// WSAStartup()
	// -------------------------------------------------------------------------

	// This object holds the information about the version of Winsock that we
	// are using, which is not necessarily the version that we requested.
	WSADATA wsaData{};
	SecureZeroMemory(&wsaData, sizeof(wsaData));

	// Initialize Winsock. You must call WSACleanup when you are finished.
	// As this function uses a reference counter, for each call to WSAStartup,
	// you must call WSACleanup or suffer memory issues.
	int errorCode = WSAStartup(MAKEWORD(WINSOCK_VERSION, WINSOCK_SUBVERSION), &wsaData);
	if (NO_ERROR != errorCode)
	{
		std::cerr << "WSAStartup() failed." << std::endl;
		return errorCode;
	}

	addrinfo udpHints{};
	SecureZeroMemory(&udpHints, sizeof(udpHints));
	udpHints.ai_family = AF_INET;		//IPv4
	udpHints.ai_socktype = SOCK_DGRAM;


	udpHints.ai_protocol = IPPROTO_UDP;

	addrinfo* udpInfo = nullptr;
	errorCode = getaddrinfo(host.c_str(), udpPortString.c_str(), &udpHints, &udpInfo);
	if ((NO_ERROR != errorCode) || (nullptr == udpInfo))
	{
		std::cerr << "UDP getaddrinfo() failed." << std::endl;
		WSACleanup();
		return errorCode;
	}

	SOCKET udpSocket = socket(udpInfo->ai_family, udpInfo->ai_socktype, udpInfo->ai_protocol);
	if (INVALID_SOCKET == udpSocket)
	{
		std::cerr << "UDP socket() failed." << std::endl;
		freeaddrinfo(udpInfo);
		WSACleanup();
		return RETURN_CODE_4;
	}

	sockaddr_in udpClientAddress{};
	udpClientAddress.sin_family = AF_INET;
	udpClientAddress.sin_addr.s_addr = INADDR_ANY;
	udpClientAddress.sin_port = htons(std::stoi(clientUDPPortString));

	if (bind(udpSocket, reinterpret_cast<sockaddr*>(&udpClientAddress), sizeof(udpClientAddress)) == SOCKET_ERROR)
	{
		std::cerr << "UDP bind() failed." << std::endl;
		closesocket(udpSocket);
		WSACleanup();
		return RETURN_CODE_4;
	}

	sockaddr_in udpServerAddress{};
	udpServerAddress.sin_family = AF_INET;
	udpServerAddress.sin_port = htons(std::stoi(udpPortString));
	inet_pton(udpServerAddress.sin_family, host.c_str(), &udpServerAddress.sin_addr);
}