/* Start Header
*****************************************************************/
/*!
\file echoserver.cpp
\author Gideon Francis (g.francis)
\co-author Lee Yong Yee (l.yongyee)
\par g.francis@digipen.edu
\date 22 Feb 2025
\brief
This file implements the server file which will be used to implement a
simple echo server.
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header
*******************************************************************/

/*******************************************************************************
 * A simple TCP/IP server application
 ******************************************************************************/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "Windows.h"		// Entire Win32 API...
#include "winsock2.h"		// ...or Winsock alone
#include "ws2tcpip.h"		// getaddrinfo()
#include <filesystem>
#include <unordered_map>
#include <random>
#include <mutex>
#include <queue>

 // Tell the Visual Studio linker to include the following library in linking.
 // Alternatively, we could add this file to the linker command-line parameters,
 // but including it in the source code simplifies the configuration.
#pragma comment(lib, "ws2_32.lib")
#include <cstdio>
#include <iostream>			   // cout, cerr
#include <string>			     // string
#include "Packet.h"
#include "Network.h"
#include "taskqueue.h"
#include "highscores.h"

//#define WINSOCK_VERSION     2
#define WINSOCK_SUBVERSION  2
#define MAX_STR_LEN         2048
#define RETURN_CODE_1       1
#define RETURN_CODE_2       2
#define RETURN_CODE_3       3
#define RETURN_CODE_4       4
#define REQ_UNKNOWN ((unsigned char)0x0)
#define REQ_QUIT       ((unsigned char)0x1)
#define REQ_DOWNLOAD       ((unsigned char)0x2)
#define RSP_DOWNLOAD       ((unsigned char)0x3)
#define REQ_LISTFILES  ((unsigned char)0x4)
#define RSP_LISTFILES  ((unsigned char)0x5)


#define CMD_TEST       ((unsigned char)0x20)
#define DOWNLOAD_ERROR     ((unsigned char)0x30)
#define WINDOW_SIZE 5
#define TIMEOUT_MS 500
#define PRINTOUT_MS 1500
#define MAX_RETRIES 5
#define PACKET_LOSS_RATE 0.02
#define SIMULATE_PACKET_LOSS false
// these scores defines may need to be commented out
#define REQ_SUBMIT_SCORE ((unsigned char)0x6)
#define RSP_SUBMIT_SCORE ((unsigned char)0x7)
#define REQ_GET_SCORES ((unsigned char)0x8)
#define RSP_GET_SCORES ((unsigned char)0x9)
#define SLEEP_TIME 0

#define ASTEROID_SCORE 100;

// Add these new handler functions:
void HandleSubmitScore(char *buffer, SOCKET clientSocket);
void ProcessPlayerDisconnect(const char *buffer, int recvLen);
void ProcessPlayerJoin(const sockaddr_in &clientAddr, const char *buffer, int recvLen);
void HandleGetScores(SOCKET clientSocket);
void UDPSendingHandler();
void UDPReceiveHandler(SOCKET udpListenerSocket);
void ProcessShipMovement(const sockaddr_in& clientAddr, const char* buffer, int recvLen);
void ProcessBulletFired(const sockaddr_in &clientAddr, const char *buffer, int recvLen);
void ProcessBulletCollision(uint32_t bulletID, uint32_t targetID, uint8_t targetType);
void ProcessAsteroidCreated(const sockaddr_in &clientAddr, const char *buffer, int recvLen);
void ProcessAsteroidDestroyed(const char *buffer, int recvLen);
void ProcessShipCollision(const char *buffer, int recvLen);

void HandleHighscoreRequest(const sockaddr_in &clientAddr);
void HandleNewHighscore(const char *buffer, int recvLen, const sockaddr_in &clientAddr);
void BroadcastHighScores();

void ProcessReplyPlayerJoin(const sockaddr_in &clientAddr, const char *buffer, int recvLen);
void ProcessNewPlayerJoin(const sockaddr_in &clientAddr, const char *buffer, int recvLen);
void ProcessGameStart(const sockaddr_in &clientAddr, const char *buffer, int recvLen);
void ProcessPacketError(const sockaddr_in &clientAddr, const char *buffer, int recvLen);

void RespawnShip(uint32_t playerID);

static int userCount = 0;

std::mutex lockMutex;
std::queue<MessageData> messageQueue;

static ServerData serverData;

SOCKET udpListenerSocket = INVALID_SOCKET;
std::string filePath;
std::mt19937 generator;
std::uniform_real_distribution dis(0.0, 1.0);

bool debugPrint = false;

int main()
{
	// Get Port Number
	std::string input;
	// comment out for now cause idk how to do the UDP part yet

	std::cout << "Server UDP Port Number: ";
	std::getline(std::cin, input);
	std::string portStringUDP = input;

	unsigned seed = (unsigned int)std::chrono::system_clock::now().time_since_epoch().count();
	generator = std::mt19937(seed);

	//srand((unsigned int)time(NULL));

	// -------------------------------------------------------------------------
	// Start up Winsock, asking for version 2.2.
	//
	// WSAStartup()
	// -------------------------------------------------------------------------

	// This object holds the information about the version of Winsock that we
	// are using, which is not necessarily the version that we requested.
	WSADATA wsaData{};

	// Initialize Winsock. You must call WSACleanup when you are finished.
	// As this function uses a reference counter, for each call to WSAStartup,
	// you must call WSACleanup or suffer memory issues.
	int errorCode = WSAStartup(MAKEWORD(WINSOCK_VERSION, WINSOCK_SUBVERSION), &wsaData);
	if (NO_ERROR != errorCode)
	{
		std::cerr << "WSAStartup() failed." << std::endl;
		return errorCode;
	}

	// -------------------------------------------------------------------------
	// Resolve own host name into IP addresses (in a singly-linked list).
	//
	// getaddrinfo()
	// -------------------------------------------------------------------------
	// Object hints indicates which protocols to use to fill in the info.
	addrinfo hints{};
	SecureZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;			// IPv4
	// For UDP use SOCK_DGRAM instead of SOCK_STREAM.
	hints.ai_socktype = SOCK_STREAM;	// Reliable delivery
	// Could be 0 for autodetect, but reliable delivery over IPv4 is always TCP.
	hints.ai_protocol = IPPROTO_TCP;	// TCP
	// Create a passive socket that is suitable for bind() and listen().
	hints.ai_flags = AI_PASSIVE;

	char host[MAX_STR_LEN];
	gethostname(host, MAX_STR_LEN);

	// -------------------------------------------------------------------------
	// Create a UDP socket and bind it to own network interface controller.
	//
	// socket()
	// bind()
	// -------------------------------------------------------------------------

	//addrinfo hintsUDP{};
	SecureZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;			// IPv4
	// For UDP use SOCK_DGRAM instead of SOCK_STREAM.
	hints.ai_socktype = SOCK_DGRAM;	// Reliable delivery
	// Could be 0 for autodetect, but reliable delivery over IPv4 is always TCP.
	hints.ai_protocol = IPPROTO_UDP;	// TCP
	// Create a passive socket that is suitable for bind() and listen().
	hints.ai_flags = AI_PASSIVE;

	//char host[MAX_STR_LEN];
	memset(host, 0, sizeof(host));

	gethostname(host, MAX_STR_LEN);

	addrinfo *info = nullptr;

	info = nullptr;
	errorCode = getaddrinfo(host, portStringUDP.c_str(), &hints, &info);
	if ((NO_ERROR != errorCode) || (nullptr == info))
	{
		std::cerr << "getaddrinfo() failed." << std::endl;
		WSACleanup();
		return errorCode;
	}

	/* PRINT SERVER IP ADDRESS AND PORT NUMBER */
	char serverIPAddr[MAX_STR_LEN];
	struct sockaddr_in *serverAddress = reinterpret_cast<struct sockaddr_in *> (info->ai_addr);
	inet_ntop(AF_INET, &(serverAddress->sin_addr), serverIPAddr, INET_ADDRSTRLEN);
	getnameinfo(info->ai_addr, static_cast <socklen_t> (info->ai_addrlen), serverIPAddr, sizeof(serverIPAddr), nullptr, 0, NI_NUMERICHOST);
	std::cout << std::endl;
	std::cout << "Server IP Address: " << serverIPAddr << std::endl;
	std::cout << "Server UDP Port Number: " << portStringUDP << std::endl;

	udpListenerSocket = socket(
		hints.ai_family,
		hints.ai_socktype,
		hints.ai_protocol);
	if (INVALID_SOCKET == udpListenerSocket)
	{
		std::cerr << "socket() failed." << std::endl;
		freeaddrinfo(info);
		WSACleanup();
		return RETURN_CODE_1;
	}

	errorCode = bind(udpListenerSocket, info->ai_addr, static_cast<int>(info->ai_addrlen));
	if (NO_ERROR != errorCode)
	{
		std::cerr << "bind() failed." << std::endl;
		closesocket(udpListenerSocket);
		udpListenerSocket = INVALID_SOCKET;
	}

	freeaddrinfo(info);

	if (INVALID_SOCKET == udpListenerSocket)
	{
		std::cerr << "bind() failed." << std::endl;
		WSACleanup();
		return RETURN_CODE_2;
	}
	std::thread udpThread(UDPReceiveHandler, udpListenerSocket);
	//std::thread udpSendThread(UDPSendingHandler);

	const auto interval = std::chrono::duration<double>(0.005); // every 10 ms? idk for now
	auto lastSendTime = std::chrono::steady_clock::now();

	auto lastPrintTime = std::chrono::steady_clock::now();
	const auto printInterval = std::chrono::duration<double>(1.0); // every 10 ms? idk for now

	while (true)
	{
		auto currTime = std::chrono::steady_clock::now();

		if (currTime - lastPrintTime >= printInterval)
		{
			lastPrintTime = currTime;
			for (int i = 0; i < MAX_CONNECTION; ++i)
			{
				if (!serverData.totalClients[i].connected) continue;

				ClientInfo& client = serverData.totalClients[i];

				std::cout << "Ship " << i << "\n"
					<< "Curr Pos : " << client.playerShip.xPos << ", " << client.playerShip.yPos << "\n"
					<< "Curr Vel : " << client.playerShip.vel_x << ", " << client.playerShip.vel_y << "\n";

			}
		}

		// so that I only send out every interval time 
		// timer
		// every 0.01 maybe, Clear the msg queue by sending the message to every client
		if (currTime - lastSendTime >= interval)
		{
			// set to the curr time it is resolving all messages
			lastSendTime = currTime;
			std::queue<MessageData> messages;
			{
				std::lock_guard<std::mutex> lock(lockMutex);
				// swap to the local queue of messages so I dont have to keep doing lock_guard shit
				std::swap(messages, messageQueue);
			}

			// loop through every message to send out
			while (!messages.empty())
			{
				char buffer[MAX_STR_LEN];
				int offset = 0;
				std::memset(buffer, 0, sizeof(buffer));

				const MessageData &msg = messages.front();
				char msgID = msg.data.id; // either msg.commandID or msg.data.id

				//std::string messageBody = msg.data.substr(1); // get rid of the 1st char as it's the commandID
				// ID of the message
				buffer[0] = msgID;
				offset++;

				// Any other header stuff do here

				// add the length of the message
				uint32_t messageLength = static_cast<uint32_t>(msg.data.writePos); // writePos represents how much was written
				messageLength = htonl(messageLength);
				std::memcpy(buffer + offset, &messageLength, sizeof(messageLength));
				offset += sizeof(messageLength);

				switch (msgID)
				{
				case REPLY_PLAYER_JOIN:
				{
					// this only sends to 1 client

					// START OF GETTING CLIENT SOCKET
					sockaddr_in otherAddr;
					memset(&otherAddr, 0, sizeof(otherAddr));
					otherAddr.sin_family = AF_INET;
					// get the port of the target client
					otherAddr.sin_port = htons(serverData.totalClients[msg.sessionID].port);
					// get the ip of the target client
					inet_pton(AF_INET, serverData.totalClients[msg.sessionID].ip.c_str(), &otherAddr.sin_addr);
					// END OF GETTING CLIENT SOCKET

					// body of the message
					std::memcpy(buffer + offset, msg.data.body, msg.data.writePos);
					offset += msg.data.writePos;
					sendto(udpListenerSocket, buffer, offset, 0, (sockaddr*)&otherAddr, sizeof(otherAddr));
					break;
				}
				case NEW_PLAYER_JOIN:
				{
					// this packet contains every player data
					// create the message buffer ifrst
					std::memcpy(buffer + offset, msg.data.body, msg.data.writePos);
					offset += msg.data.writePos;

					// loop through every client to send this msg to them
					for (int i = 0; i < MAX_CONNECTION; ++i)
					{
						ClientInfo &client = serverData.totalClients[i];
						if (!client.connected) continue; // skip unconnected client slots

						sockaddr_in clientAddr;
						memset(&clientAddr, 0, sizeof(clientAddr));
						clientAddr.sin_family = AF_INET;
						clientAddr.sin_port = htons(client.port);

						// convert IP string to binary format
						if (inet_pton(AF_INET, client.ip.c_str(), &clientAddr.sin_addr) <= 0)
						{
							// invalid IP
							continue;
						}

						// send it
						sendto(udpListenerSocket, buffer, offset, 0, (sockaddr *)&clientAddr, sizeof(clientAddr));
					}
					break;
				}
				case SHIP_MOVE:

					//msg.data << msg.sessionID;
					std::memcpy(buffer + offset, msg.data.body, msg.data.writePos);
					offset += msg.data.writePos;
					for (int i = 0; i < MAX_CONNECTION; ++i)
					{
						ClientInfo& client = serverData.totalClients[i];
						if (!client.connected) continue; // skip unconnected client slots
						if (client.sessionID == msg.sessionID) continue; // dont update for hte client thats moving

						sockaddr_in clientAddr;
						memset(&clientAddr, 0, sizeof(clientAddr));
						clientAddr.sin_family = AF_INET;
						clientAddr.sin_port = htons(client.port);

						// convert IP string to binary format
						if (inet_pton(AF_INET, client.ip.c_str(), &clientAddr.sin_addr) <= 0)
						{
							// invalid IP
							continue;
						}

						// send it
						sendto(udpListenerSocket, buffer, offset, 0, (sockaddr*)&clientAddr, sizeof(clientAddr));
					}
					
					break;

				case PLAYER_DC:
					std::memcpy(buffer + offset, msg.data.body, msg.data.writePos);
					offset += msg.data.writePos;

					for (int i = 0; i < MAX_CONNECTION; ++i)
					{
						ClientInfo &client = serverData.totalClients[i];
						if (!client.connected) continue;

						sockaddr_in clientAddr;
						memset(&clientAddr, 0, sizeof(clientAddr));
						clientAddr.sin_family = AF_INET;
						clientAddr.sin_port = htons(client.port);
						inet_pton(AF_INET, client.ip.c_str(), &clientAddr.sin_addr);

						sendto(udpListenerSocket, buffer, offset, 0, (sockaddr *)&clientAddr, sizeof(clientAddr));
					}
					break;
				case PLAYER_JOIN:
					// This would be used when the server initiates a player join (not common)
					// Typically player join is client-initiated and handled in UDPReceiveHandler
					std::memcpy(buffer + offset, msg.data.body, msg.data.writePos);
					offset += msg.data.writePos;

					for (int i = 0; i < MAX_CONNECTION; ++i)
					{
						ClientInfo &client = serverData.totalClients[i];
						if (!client.connected) continue;

						sockaddr_in clientAddr;
						memset(&clientAddr, 0, sizeof(clientAddr));
						clientAddr.sin_family = AF_INET;
						clientAddr.sin_port = htons(client.port);
						inet_pton(AF_INET, client.ip.c_str(), &clientAddr.sin_addr);

						sendto(udpListenerSocket, buffer, offset, 0, (sockaddr *)&clientAddr, sizeof(clientAddr));
					}
					break;
				case PACKET_ERROR:
					// Send error response to specific client
					/*sockaddr_in otherAddr;
					memset(&otherAddr, 0, sizeof(otherAddr));
					otherAddr.sin_family = AF_INET;
					otherAddr.sin_port = htons(serverData.totalClients[msg.sessionID].port);
					inet_pton(AF_INET, serverData.totalClients[msg.sessionID].ip.c_str(), &otherAddr.sin_addr);

					std::memcpy(buffer + offset, msg.data.body, msg.data.writePos);
					offset += msg.data.writePos;
					sendto(udpListenerSocket, buffer, offset, 0, (sockaddr *)&otherAddr, sizeof(otherAddr));*/
					break;
				}
				// pop the message im using
				messages.pop();
			}
		
#pragma region STATE_UPDATE

			//// i should really store total connected clients os i dont have to keep looping this shit
			//// loop through every ship
			//std::vector<int> connectedClients;
			//for (int i = 0; i < MAX_CONNECTION; ++i)
			//{
			//	if (serverData.totalClients[i].connected)
			//		connectedClients.push_back(i);
			//}

			//// sending positions only really needed if theres more than 1
			//if (connectedClients.size() > 1)
			//{
			//	// send current game state to all clients (i.e all ship's current positions etc, and later on asteroids/bullets as well)
			//	Packet stateUpdate(STATE_UPDATE);

			//	stateUpdate << (uint32_t)connectedClients.size();

			//	for (int i = 0; i < connectedClients.size(); ++i)
			//	{
			//		stateUpdate << connectedClients[i];
			//		ClientInfo& info = serverData.totalClients[connectedClients[i]];
			//		stateUpdate << info.playerShip.xPos;
			//		stateUpdate << info.playerShip.yPos;
			//		stateUpdate << info.playerShip.vel_x;
			//		stateUpdate << info.playerShip.vel_y;
			//		stateUpdate << info.playerShip.dirCur;
			//	}


			//	char updateBuffer[MAX_STR_LEN];
			//	int updateOffset = 0;
			//	std::memset(updateBuffer, 0, MAX_STR_LEN);

			//	updateBuffer[0] = stateUpdate.id;
			//	updateOffset++;

			//	// Any other header stuff do here

			//	// add the length of the message
			//	uint32_t messageLength = static_cast<uint32_t>(stateUpdate.writePos); // writePos represents how much was written
			//	messageLength = htonl(messageLength);
			//	std::memcpy(updateBuffer + updateOffset, &messageLength, sizeof(messageLength));
			//	updateOffset += sizeof(messageLength);

			//	// now add the body of the packet
			//	std::memcpy(updateBuffer + updateOffset, stateUpdate.body, stateUpdate.writePos);
			//	updateOffset += stateUpdate.writePos;


			//	for (int i = 0; i < MAX_CONNECTION; ++i)
			//	{
			//		ClientInfo& client = serverData.totalClients[i];
			//		if (!client.connected) continue; // skip unconnected client slots

			//		sockaddr_in clientAddr;
			//		memset(&clientAddr, 0, sizeof(clientAddr));
			//		clientAddr.sin_family = AF_INET;
			//		clientAddr.sin_port = htons(client.port);

			//		// convert IP string to binary format
			//		if (inet_pton(AF_INET, client.ip.c_str(), &clientAddr.sin_addr) <= 0)
			//		{
			//			// invalid IP
			//			continue;
			//		}

			//		// send it
			//		sendto(udpListenerSocket, updateBuffer, updateOffset, 0, (sockaddr*)&clientAddr, sizeof(clientAddr));
			//	}
			//}
#pragma endregion

		}

		//Sleep(SLEEP_TIME);
	}
	// -------------------------------------------------------------------------
	// Clean-up after Winsock.
	//
	// WSACleanup()
	// -------------------------------------------------------------------------

	WSACleanup();
}


bool SimulatePacketLost()
{
	double rand = dis(generator);
	//std::cout << rand << std::endl;
	return rand < PACKET_LOSS_RATE;
}

void UDPReceiveHandler(SOCKET udpListenerSocket)
{

	u_long enable = 1;
	ioctlsocket(udpListenerSocket, FIONBIO, &enable);

	while (true)
	{
		char buffer[MAX_STR_LEN];
		struct sockaddr_in recvAddr;
		int recvAddrLen = sizeof(recvAddr);

		// receive from client
		int recvLen = recvfrom(udpListenerSocket, buffer, sizeof(buffer), 0, (struct sockaddr *)&recvAddr, &recvAddrLen);

		if (recvLen == SOCKET_ERROR)
		{
			size_t errCode = WSAGetLastError();
			if (errCode == WSAEWOULDBLOCK)
			{
				Sleep(50);
				continue;
			}
		}

		if (recvLen == 0)
		{
			// mutex lock
			std::cout << "Shutdown" << std::endl;
			break;
		}

		if (recvLen > 0)
		{
			char msgID = buffer[0];

			// i only do this one for now
			switch (msgID)
			{
			case PLAYER_DC:
				ProcessPlayerDisconnect(buffer, recvLen);
				break;
			case PLAYER_JOIN:
				ProcessPlayerJoin(recvAddr, buffer, recvLen);
				break;
			case SHIP_MOVE:
				ProcessShipMovement(recvAddr, buffer, recvLen);
				break;
			}
		}
	}
}

void HandleGetScores(SOCKET clientSocket)
{
	char message[MAX_STR_LEN];
	unsigned int messageSize = 0;

	message[0] = RSP_GET_SCORES;
	messageSize += 1;

	// Get number of scores
	uint16_t numScores = static_cast<uint16_t>(topScores.size());
	uint16_t numScoresNetworkOrder = htons(numScores);
	memcpy(&message[messageSize], &numScoresNetworkOrder, sizeof(numScoresNetworkOrder));
	messageSize += sizeof(numScoresNetworkOrder);

	// Pack each score into the message
	for (const auto &score : topScores)
	{
		// Add player name length
		uint32_t nameLength = static_cast<uint32_t>(score.playerName.size()); // Use string size
		uint32_t nameLengthNetworkOrder = htonl(nameLength);
		memcpy(&message[messageSize], &nameLengthNetworkOrder, sizeof(nameLengthNetworkOrder));
		messageSize += sizeof(nameLengthNetworkOrder);

		// Add player name
		memcpy(&message[messageSize], score.playerName.c_str(), nameLength); // Use c_str() to get raw pointer
		messageSize += nameLength;

		// Add score
		uint32_t scoreNetworkOrder = htonl(score.score);
		memcpy(&message[messageSize], &scoreNetworkOrder, sizeof(scoreNetworkOrder));
		messageSize += sizeof(scoreNetworkOrder);
	}

	// Send high scores to client
	send(clientSocket, message, messageSize, 0);
}
void HandleSubmitScore(char *buffer, SOCKET clientSocket)
{
	int offset = 1;  // Skip command ID

	// Get player name length
	uint32_t nameLength;
	memcpy(&nameLength, &buffer[offset], 4);
	nameLength = ntohl(nameLength);
	offset += 4;

	// Extract the player name
	std::string playerName(buffer + offset, nameLength);
	offset += nameLength;

	// Get the score
	uint32_t score;
	memcpy(&score, &buffer[offset], 4);
	score = ntohl(score);

	std::cout << "Received score from " << playerName << ": " << score << std::endl;

	// Update high scores
	bool addedToHighScores = UpdateHighScores(playerName, score);

	// Send response
	char message[MAX_STR_LEN];
	unsigned int messageSize = 0;

	message[0] = RSP_SUBMIT_SCORE;
	messageSize += 1;

	// Add success flag
	uint8_t success = addedToHighScores ? 1 : 0;
	memcpy(&message[messageSize], &success, 1);
	messageSize += 1;

	// Send response to client
	send(clientSocket, message, messageSize, 0);
}
void ProcessPlayerDisconnect(const char *buffer, int recvLen)
{
	if (recvLen < 5) return; // Ensure buffer contains at least 5 bytes (1 for msgID + 4 for playerID)

	uint32_t playerID;
	std::memcpy(&playerID, buffer + 1, sizeof(uint32_t));

	// Check if player is valid
	if (playerID >= MAX_CONNECTION || !serverData.totalClients[playerID].connected)
	{
		std::cerr << "Invalid disconnect request for player " << playerID << std::endl;
		return;
	}

	// Mark player as disconnected
	serverData.totalClients[playerID].connected = false;
	std::cout << "Player " << playerID << " has disconnected." << std::endl;

	// Remove player's bullets
	for (auto it = serverData.activeBullets.begin(); it != serverData.activeBullets.end();)
	{
		if (it->second.ownerID == playerID)
		{
			it = serverData.activeBullets.erase(it);
		}
		else
		{
			++it;
		}
	}

	// Send player disconnect message to all clients
	Packet playerDCMsg(PLAYER_DC);
	playerDCMsg << playerID;

	MessageData msg;
	msg.commandID = playerDCMsg.id;
	msg.sessionID = -1; // Broadcast to all
	msg.data = playerDCMsg;

	std::lock_guard<std::mutex> lock(lockMutex);
	messageQueue.push(msg);
}
void ProcessPlayerJoin(const sockaddr_in &clientAddr, const char *buffer, int recvLen)
{
	int32_t availID = -1;
	for (int i = 0; i < MAX_CONNECTION; ++i)
	{
		if (!serverData.totalClients[i].connected)
		{
			// this client is not connected
			availID = i;
			break;
		}
	}

	if (availID == -1)
	{
		// send a rej packet but i lazy do that now
		return;
	}

	ClientInfo &newClient = serverData.totalClients[availID];

	newClient.sessionID = availID;
	newClient.ip = inet_ntoa(clientAddr.sin_addr);
	newClient.port = ntohs(clientAddr.sin_port);
	newClient.connected = true;

	serverData.playerMap[newClient.ip] = availID;

	// default initialize ship data
	// send back to the connecting player the reply
	Packet replyPacket(REPLY_PLAYER_JOIN);
	replyPacket << availID; // pack the ship's ID in 
	//std::string message = replyPacket.ToString(); 
	// send to the client
	{
		MessageData newMessage;
		newMessage.commandID = replyPacket.id;
		newMessage.sessionID = newClient.sessionID;
		newMessage.data = replyPacket;

		std::lock_guard<std::mutex> lock(lockMutex);
		messageQueue.push(newMessage);
	}


	Packet newPlayerPacket(NEW_PLAYER_JOIN);

	std::vector<int> playerIDs;

	for (int i = 0; i < MAX_CONNECTION; ++i)
	{
		//if (i == availID) continue;

		if (!serverData.totalClients[i].connected) continue;

		playerIDs.push_back(i);
	}

	if (playerIDs.size() <= 1) return; // if onyl 1 player has been made then dont send anything

	newPlayerPacket << (uint32_t)playerIDs.size(); // push back the number of active players

	for (int i = 0; i < playerIDs.size(); ++i)
	{
		ClientInfo& info = serverData.totalClients[playerIDs[i]];
		newPlayerPacket << playerIDs[i];
		newPlayerPacket << info.playerShip.xPos;
		newPlayerPacket << info.playerShip.yPos;
		newPlayerPacket << info.playerShip.vel_x;
		newPlayerPacket << info.playerShip.vel_y;
		newPlayerPacket << info.playerShip.dirCur;
	}
	//newPlayerPacket << 1; // only 1 new player 
	//newPlayerPacket << newClient.sessionID; // i think i should be packing the ID of the new client??

	//message = newPlayerPacket.ToString();

	{
		MessageData newMessage;
		newMessage.commandID = newPlayerPacket.id;
		newMessage.sessionID = newClient.sessionID;// sending to the current client's id which is i 
		newMessage.data = newPlayerPacket;

		std::lock_guard<std::mutex> lock(lockMutex);
		messageQueue.push(newMessage);
	}
}
void ProcessShipMovement(const sockaddr_in& clientAddr, const char* buffer, int recvLen)
{
	std::string ip = inet_ntoa(clientAddr.sin_addr);
	// get the id of the ship thats moving
	ClientInfo& client = serverData.totalClients[serverData.playerMap[ip]];
	int offset = 1;

	// get rid of header data
	uint32_t msgLength;
	std::memcpy(&msgLength, buffer + offset, sizeof(msgLength));
	msgLength = ntohl(msgLength);
	offset += sizeof(msgLength);

	Packet shipMovement(SHIP_MOVE);
	shipMovement.writePos += msgLength;
	std::memcpy(shipMovement.body, buffer + offset, msgLength);

	int sessionID;
	int playerInput;
	uint64_t timeDiff;
	shipMovement >> sessionID;
	shipMovement >> timeDiff;
	shipMovement >> playerInput;
	shipMovement >> client.playerShip.xPos;
	shipMovement >> client.playerShip.yPos;
	shipMovement >> client.playerShip.vel_x;
	shipMovement >> client.playerShip.vel_y;
	shipMovement >> client.playerShip.dirCur;


	{
		MessageData newMessage;
		newMessage.commandID = shipMovement.id;
		newMessage.sessionID = client.sessionID;// sending to the current client's id which is i 
		newMessage.data = shipMovement;

		std::lock_guard<std::mutex> lock(lockMutex);
		messageQueue.push(newMessage);
	}

}
void RespawnShip(uint32_t playerID)
{
	if (playerID >= MAX_CONNECTION || !serverData.totalClients[playerID].connected)
	{
		return;
	}

	ClientInfo &player = serverData.totalClients[playerID];

	// Reset ship properties
	player.playerShip.xPos = 0.0f;  // Spawn in center
	player.playerShip.yPos = 0.0f;
	player.playerShip.vel_x = 0.0f;
	player.playerShip.vel_y = 0.0f;
	player.playerShip.vel_server_x = 0.0f;
	player.playerShip.vel_server_y = 0.0f;

	// Create and broadcast ship respawn message
	Packet respawnPacket(SHIP_RESPAWN);
	respawnPacket << playerID;
	respawnPacket << player.playerShip.xPos << player.playerShip.yPos;

	// Queue the message
	MessageData respawnMsg;
	respawnMsg.commandID = respawnPacket.id;
	respawnMsg.sessionID = -1; // Broadcast to all
	respawnMsg.data = respawnPacket;

	std::lock_guard<std::mutex> lock(lockMutex);
	messageQueue.push(respawnMsg);
}
void HandleHighscoreRequest(const sockaddr_in &clientAddr)
{
	// Create response packet
	Packet highscorePacket(REQ_HIGHSCORE);

	// Pack number of scores
	uint16_t numScores = static_cast<uint16_t>(topScores.size());
	highscorePacket << numScores;

	// Pack each score
	for (const auto &score : topScores)
	{
		highscorePacket << score.playerName << score.score;
	}

	// Prepare message for queue
	MessageData newMessage;
	newMessage.commandID = highscorePacket.id;
	newMessage.sessionID = -1; // Specific response, not broadcast

	// Create target address (respond to requester)
	sockaddr_in targetAddr = clientAddr;

	// Queue the message
	{
		std::lock_guard<std::mutex> lock(lockMutex);
		messageQueue.push(newMessage);
	}
}
void HandleNewHighscore(const char *buffer, int recvLen, const sockaddr_in &clientAddr)
{
	if (recvLen < 1 + sizeof(uint32_t)) // Minimum: message ID + score
	{
		return; // Not enough data
	}

	int offset = 1; // Skip message ID

	// Extract player name length
	uint32_t nameLength;
	memcpy(&nameLength, buffer + offset, sizeof(nameLength));
	nameLength = ntohl(nameLength);
	offset += sizeof(nameLength);

	// Validate remaining length
	if (recvLen < offset + nameLength + sizeof(uint32_t))
	{
		return; // Not enough data for name + score
	}

	// Extract player name
	char playerName[20] = {};
	std::memcpy(playerName, buffer + offset, 20);
	offset += 20;
	offset += nameLength;

	// Extract score
	uint32_t score;
	memcpy(&score, buffer + offset, sizeof(score));
	score = ntohl(score);

	// Update high scores
	bool added = UpdateHighScores(playerName, score);

	// Create response packet
	Packet responsePacket(NEW_HIGHSCORE);
	responsePacket << static_cast<uint8_t>(added ? 1 : 0); // Success flag

	// Prepare message for queue
	MessageData newMessage;
	newMessage.commandID = responsePacket.id;
	newMessage.sessionID = -1; // Specific response, not broadcast
	newMessage.data = responsePacket;

	// Create target address (respond to submitter)
	sockaddr_in targetAddr = clientAddr;

	// Queue the message
	{
		std::lock_guard<std::mutex> lock(lockMutex);
		messageQueue.push(newMessage);
	}

	// If score was added, broadcast updated high scores to all clients
	if (added)
	{
		BroadcastHighScores();
	}
}
void BroadcastHighScores()
{
	Packet highscorePacket(REQ_HIGHSCORE);

	// Pack number of scores
	uint16_t numScores = static_cast<uint16_t>(topScores.size());
	highscorePacket << numScores;

	// Pack each score
	for (const auto &score : topScores)
	{
		highscorePacket << score.playerName << score.score;
	}

	// Prepare broadcast message
	MessageData newMessage;
	newMessage.commandID = highscorePacket.id;
	newMessage.sessionID = -1; // Broadcast to all
	newMessage.data = highscorePacket;

	// Queue the message
	{
		std::lock_guard<std::mutex> lock(lockMutex);
		messageQueue.push(newMessage);
	}
}
