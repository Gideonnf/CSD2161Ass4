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
#define SLEEP_TIME 500

#define ASTEROID_SCORE 100;

// Add these new handler functions:
void HandleSubmitScore(char *buffer, SOCKET clientSocket);
void ProcessPlayerDisconnect(const char *buffer, int recvLen);
void ProcessPlayerJoin(const sockaddr_in &clientAddr, const char *buffer, int recvLen);
void HandleGetScores(SOCKET clientSocket);
void UDPSendingHandler();
void UDPReceiveHandler(SOCKET udpListenerSocket);

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

	const auto interval = std::chrono::duration<double>(0.01); // every 10 ms? idk for now
	auto lastSendTime = std::chrono::steady_clock::now();

	while (true)
	{
		auto currTime = std::chrono::steady_clock::now();
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

					// reset the message
					std::memset(buffer, 0, sizeof(buffer));
					offset = 0;

					std::vector<int32_t> playerIDs;
					// use new player join packet

					// Send any existing connected player to the new client as well
					for (int i = 0; i < MAX_CONNECTION; ++i)
					{
						if (i == msg.sessionID) continue; // skip the new player's id

						if (!serverData.totalClients[i].connected) continue; // skip unconnected slots

						playerIDs.push_back(i);
						//newPlayer << i; // pack the client's ID into it
					}

					if (!playerIDs.empty())
					{
						Packet newPlayer(NEW_PLAYER_JOIN);

						for (int i = 0; i < playerIDs.size(); ++i)
						{
							// add all the player ids
							newPlayer << playerIDs[i];
						}


						char msgID = newPlayer.id; // either msg.commandID or msg.data.id

						//std::string messageBody = msg.data.substr(1); // get rid of the 1st char as it's the commandID
						// ID of the message
						buffer[0] = msgID;
						offset++;

						// any other header stuff here

						// add the length of the message
						uint32_t messageLength = static_cast<uint32_t>(msg.data.writePos); // writePos represents how much was written
						messageLength = htonl(messageLength);
						std::memcpy(buffer + offset, &messageLength, sizeof(messageLength));
						offset += sizeof(messageLength);

						// this msg contains a lsit of every connected client's IDs to the newest joined player
						// body of the message
						std::memcpy(buffer + offset, newPlayer.body, newPlayer.writePos);
						offset += newPlayer.writePos;
						sendto(udpListenerSocket, buffer, offset, 0, (sockaddr*)&otherAddr, sizeof(otherAddr));

					}


					break;
				}
				case NEW_PLAYER_JOIN:
				{
					// create the message buffer ifrst
					std::memcpy(buffer + offset, msg.data.body, msg.data.writePos);
					offset += sizeof(msg.data.writePos);

					// loop through every client to send this msg to them
					for (int i = 0; i < MAX_CONNECTION; ++i)
					{
						// in this case, sessionID of message is used to represent who is joining
						if (i == msg.sessionID) continue; // dont send to the new player joining

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
				case BULLET_COLLIDE:
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
				case BULLET_CREATED:
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
				case ASTEROID_CREATED:
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
				case ASTEROID_DESTROYED:
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
				case SHIP_RESPAWN:
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
				case SHIP_COLLIDE:
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
				case REQ_HIGHSCORE:
					sockaddr_in otherAddr;
					memset(&otherAddr, 0, sizeof(otherAddr));
					otherAddr.sin_family = AF_INET;
					otherAddr.sin_port = htons(serverData.totalClients[msg.sessionID].port);
					inet_pton(AF_INET, serverData.totalClients[msg.sessionID].ip.c_str(), &otherAddr.sin_addr);

					std::memcpy(buffer + offset, msg.data.body, msg.data.writePos);
					offset += msg.data.writePos;
					sendto(udpListenerSocket, buffer, offset, 0, (sockaddr *)&otherAddr, sizeof(otherAddr));
					break;
				case NEW_HIGHSCORE:
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
				case GAME_START:
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
					sockaddr_in otherAddr;
					memset(&otherAddr, 0, sizeof(otherAddr));
					otherAddr.sin_family = AF_INET;
					otherAddr.sin_port = htons(serverData.totalClients[msg.sessionID].port);
					inet_pton(AF_INET, serverData.totalClients[msg.sessionID].ip.c_str(), &otherAddr.sin_addr);

					std::memcpy(buffer + offset, msg.data.body, msg.data.writePos);
					offset += msg.data.writePos;
					sendto(udpListenerSocket, buffer, offset, 0, (sockaddr *)&otherAddr, sizeof(otherAddr));
					break;
				}
				// pop the message im using
				messages.pop();
			}
		}

		Sleep(SLEEP_TIME);
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

void UDPSendingHandler()
{
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
				Sleep(200);
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
				break;
			case REPLY_PLAYER_JOIN:
				ProcessReplyPlayerJoin(recvAddr, buffer, recvLen);
				break;
			case NEW_PLAYER_JOIN:
				ProcessNewPlayerJoin(recvAddr, buffer, recvLen);
				break;
			case BULLET_COLLIDE:
				if (recvLen < 9) break; // Ensure buffer contains enough bytes (1 byte msgID + 8 bytes data)

				uint32_t bulletID, targetID;
				uint8_t targetType;

				std::memcpy(&bulletID, buffer + 1, sizeof(uint32_t));
				std::memcpy(&targetID, buffer + 5, sizeof(uint32_t));
				std::memcpy(&targetType, buffer + 9, sizeof(uint8_t));

				ProcessBulletCollision(bulletID, targetID, targetType);
				break;
			case BULLET_CREATED:
				ProcessBulletFired(recvAddr, buffer, recvLen);
				break;
			case ASTEROID_CREATED:
				ProcessAsteroidCreated(recvAddr, buffer, recvLen);
				break;
			case ASTEROID_DESTROYED:
				ProcessAsteroidDestroyed(buffer, recvLen);
				break;
			case SHIP_RESPAWN:
				if (recvLen < 5) break; // Ensure buffer has enough data

				uint32_t playerID;
				std::memcpy(&playerID, buffer + 1, sizeof(uint32_t));

				RespawnShip(playerID);
				break;
			case SHIP_COLLIDE:
				ProcessShipCollision(buffer, recvLen);
				break;
			case REQ_HIGHSCORE:
				HandleHighscoreRequest(recvAddr);
				break;
			case NEW_HIGHSCORE:
				HandleNewHighscore(buffer, recvLen, recvAddr);
				break;
			case GAME_START:
				ProcessGameStart(recvAddr, buffer, recvLen);
				break;
			case PACKET_ERROR:
				ProcessPacketError(recvAddr, buffer, recvLen);
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
		uint32_t nameLength = static_cast<uint32_t>(score.playerName.length());
		uint32_t nameLengthNetworkOrder = htonl(nameLength);
		memcpy(&message[messageSize], &nameLengthNetworkOrder, sizeof(nameLengthNetworkOrder));
		messageSize += sizeof(nameLengthNetworkOrder);

		// Add player name
		memcpy(&message[messageSize], score.playerName.c_str(), nameLength);
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
	newPlayerPacket << newClient.sessionID; // i think i should be packing the ID of the new client??
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
void ProcessBulletFired(const sockaddr_in &clientAddr, const char *buffer, int recvLen)
{
	// Extract the player ID
	int offset = 1; // Skip message ID
	uint32_t playerID;
	memcpy(&playerID, buffer + offset, sizeof(playerID));
	playerID = ntohl(playerID);
	offset += sizeof(playerID);

	// Extract bullet position and velocity
	float xPos, yPos, velX, velY;
	memcpy(&xPos, buffer + offset, sizeof(float));
	offset += sizeof(float);
	memcpy(&yPos, buffer + offset, sizeof(float));
	offset += sizeof(float);
	memcpy(&velX, buffer + offset, sizeof(float));
	offset += sizeof(float);
	memcpy(&velY, buffer + offset, sizeof(float));
	offset += sizeof(float);

	// Create a new bullet
	Bullet newBullet;
	newBullet.ownerID = playerID;
	newBullet.xPos = xPos;
	newBullet.yPos = yPos;
	newBullet.vel_x = velX;
	newBullet.vel_y = velY;
	newBullet.vel_server_x = velX; // Server can adjust if needed for physics
	newBullet.vel_server_y = velY;
	newBullet.active = true;

	// Add to server's list of active bullets
	int bulletID = serverData.nextBulletID++;
	serverData.activeBullets[bulletID] = newBullet;

	// Create a message to broadcast to all clients that a bullet was fired
	Packet bulletPacket(BULLET_CREATED);
	bulletPacket << bulletID;
	bulletPacket << playerID;
	bulletPacket << xPos << yPos << velX << velY;

	// Queue the message
	MessageData newMessage;
	newMessage.commandID = bulletPacket.id;
	newMessage.sessionID = -1; // Broadcast to all
	newMessage.data = bulletPacket;

	std::lock_guard<std::mutex> lock(lockMutex);
	messageQueue.push(newMessage);
}
void ProcessBulletCollision(uint32_t bulletID, uint32_t targetID, uint8_t targetType)
{
	// Validate IDs exist
	if (serverData.activeBullets.find(bulletID) == serverData.activeBullets.end())
	{
		return; // Bullet doesn't exist
	}

	Bullet &bullet = serverData.activeBullets[bulletID];
	uint32_t shooterID = bullet.ownerID;

	// Handle collision based on target type
	if (targetType == TARGET_TYPE_ASTEROID)
	{
		// Check if asteroid exists
		auto it = std::find_if(serverData.asteroids.begin(), serverData.asteroids.end(),
			[targetID](const Asteroid &asteroid) { return asteroid.ID == targetID; });

		if (it == serverData.asteroids.end())
		{
			return; // Asteroid doesn't exist
		}


		Asteroid &asteroid = serverData.asteroids[targetID];

		// Mark bullet as inactive
		bullet.active = false;

		// Handle asteroid destruction

			// Asteroid destroyed
		asteroid.active = false;

		// Award points to the shooter
		if (shooterID < MAX_CONNECTION && serverData.totalClients[shooterID].connected)
		{
			serverData.totalClients[shooterID].playerShip.score += ASTEROID_SCORE;
		}


		// Create asteroid destroyed message
		Packet asteroidDestroyedPacket(ASTEROID_DESTROYED);
		asteroidDestroyedPacket << targetID;
		asteroidDestroyedPacket << shooterID;  // Who destroyed it

		// Queue the message
		MessageData asteroidMsg;
		asteroidMsg.commandID = asteroidDestroyedPacket.id;
		asteroidMsg.sessionID = -1; // Broadcast to all
		asteroidMsg.data = asteroidDestroyedPacket;

		std::lock_guard<std::mutex> lock(lockMutex);
		messageQueue.push(asteroidMsg);
	}

	// Create bullet collision message in all cases
	Packet bulletCollidePacket(BULLET_COLLIDE);
	bulletCollidePacket << static_cast<int32_t>(bulletID);
	bulletCollidePacket << static_cast<int32_t>(targetID);
	bulletCollidePacket << static_cast<int32_t>(targetType);

	// Queue the message
	MessageData bulletMsg;
	bulletMsg.commandID = bulletCollidePacket.id;
	bulletMsg.sessionID = -1; // Broadcast to all
	bulletMsg.data = bulletCollidePacket;

	std::lock_guard<std::mutex> lock(lockMutex);
	messageQueue.push(bulletMsg);
}

// Helper function to respawn a ship
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
void ProcessAsteroidCreated(const sockaddr_in &clientAddr, const char *buffer, int recvLen)
{
	if (recvLen < 1 + sizeof(uint32_t) + 4 * sizeof(float))
	{
		return; // Not enough data
	}

	int offset = 1; // Skip message ID

	// Extract asteroid data
	uint32_t asteroidID;
	float xPos, yPos, velX, velY;

	memcpy(&asteroidID, buffer + offset, sizeof(asteroidID));
	offset += sizeof(asteroidID);
	memcpy(&xPos, buffer + offset, sizeof(float));
	offset += sizeof(float);
	memcpy(&yPos, buffer + offset, sizeof(float));
	offset += sizeof(float);
	memcpy(&velX, buffer + offset, sizeof(float));
	offset += sizeof(float);
	memcpy(&velY, buffer + offset, sizeof(float));
	offset += sizeof(float);

	// Create new asteroid
	Asteroid newAsteroid;
	newAsteroid.ID = asteroidID;
	newAsteroid.xPos = xPos;
	newAsteroid.yPos = yPos;
	newAsteroid.vel_x = velX;
	newAsteroid.vel_y = velY;
	newAsteroid.active = true;

	// Add to server's asteroid list
	serverData.asteroids.push_back(newAsteroid);

	// Broadcast asteroid creation to all clients
	Packet asteroidPacket(ASTEROID_CREATED);
	asteroidPacket << asteroidID << xPos << yPos << velX << velY;

	MessageData newMessage;
	newMessage.commandID = asteroidPacket.id;
	newMessage.sessionID = -1; // Broadcast to all
	newMessage.data = asteroidPacket;

	std::lock_guard<std::mutex> lock(lockMutex);
	messageQueue.push(newMessage);
}
void ProcessAsteroidDestroyed(const char *buffer, int recvLen)
{
	if (recvLen < 1 + sizeof(uint32_t))
	{
		return; // Not enough data
	}

	uint32_t asteroidID;
	memcpy(&asteroidID, buffer + 1, sizeof(asteroidID));
	asteroidID = ntohl(asteroidID);

	// Find and mark asteroid as inactive
	auto it = std::find_if(serverData.asteroids.begin(), serverData.asteroids.end(),
		[asteroidID](const Asteroid &a) { return a.ID == asteroidID; });

	if (it != serverData.asteroids.end())
	{
		it->active = false;
	}

	// Broadcast destruction to all clients
	Packet asteroidPacket(ASTEROID_DESTROYED);
	asteroidPacket << asteroidID;

	MessageData newMessage;
	newMessage.commandID = asteroidPacket.id;
	newMessage.sessionID = -1; // Broadcast to all
	newMessage.data = asteroidPacket;

	std::lock_guard<std::mutex> lock(lockMutex);
	messageQueue.push(newMessage);
}
void ProcessShipCollision(const char *buffer, int recvLen)
{
	if (recvLen < 1 + 2 * sizeof(uint32_t) + sizeof(uint8_t))
	{
		return; // Not enough data
	}

	int offset = 1; // Skip message ID

	// Extract ship ID and target info
	uint32_t shipID;
	uint32_t targetID;
	uint8_t targetType;

	memcpy(&shipID, buffer + offset, sizeof(shipID));
	shipID = ntohl(shipID);
	offset += sizeof(shipID);

	memcpy(&targetID, buffer + offset, sizeof(targetID));
	targetID = ntohl(targetID);
	offset += sizeof(targetID);

	memcpy(&targetType, buffer + offset, sizeof(targetType));
	offset += sizeof(targetType);

	// Validate ship exists and is connected
	if (shipID >= MAX_CONNECTION || !serverData.totalClients[shipID].connected)
	{
		return;
	}

	// Only process ship-asteroid collisions
	if (targetType == TARGET_TYPE_ASTEROID)
	{
		// Find the asteroid
		auto it = std::find_if(serverData.asteroids.begin(), serverData.asteroids.end(),
			[targetID](const Asteroid &a) { return a.ID == targetID && a.active; });

		if (it != serverData.asteroids.end())
		{
			// Mark asteroid as inactive
			it->active = false;

			// Respawn the ship (since it hit an asteroid)
			RespawnShip(shipID);

			// Broadcast asteroid destruction
			Packet asteroidPacket(ASTEROID_DESTROYED);
			asteroidPacket << targetID;
			asteroidPacket << shipID;  // Who destroyed it (the ship that collided)

			MessageData asteroidMsg;
			asteroidMsg.commandID = asteroidPacket.id;
			asteroidMsg.sessionID = -1;
			asteroidMsg.data = asteroidPacket;

			std::lock_guard<std::mutex> lock(lockMutex);
			messageQueue.push(asteroidMsg);
		}
	}
	// Ignore all other collision types (ship-ship, ship-bullet)

	// Always broadcast the collision event (even if we didn't process it)
	// Clients can decide how to handle it visually
	Packet collisionPacket(SHIP_COLLIDE);
	collisionPacket << shipID << targetID << targetType;

	MessageData collisionMsg;
	collisionMsg.commandID = collisionPacket.id;
	collisionMsg.sessionID = -1;
	collisionMsg.data = collisionPacket;

	std::lock_guard<std::mutex> lock(lockMutex);
	messageQueue.push(collisionMsg);
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
	std::string playerName(buffer + offset, nameLength);
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

void ProcessReplyPlayerJoin(const sockaddr_in &clientAddr, const char *buffer, int recvLen)
{
	if (recvLen < 1 + sizeof(uint32_t))
	{
		return; // Not enough data for player ID
	}

	uint32_t playerID;
	memcpy(&playerID, buffer + 1, sizeof(playerID));
	playerID = ntohl(playerID);

	// Validate player ID
	if (playerID >= MAX_CONNECTION || !serverData.totalClients[playerID].connected)
	{
		return;
	}

	// Typically this would be a client->server acknowledgement
	// Could update connection status or resend join info if needed
	if (debugPrint)
	{
		std::cout << "Player " << playerID << " acknowledged join" << std::endl;
	}
}
void ProcessNewPlayerJoin(const sockaddr_in &clientAddr, const char *buffer, int recvLen)
{
	if (recvLen < 1 + sizeof(uint32_t))
	{
		return; // Not enough data for player ID
	}

	uint32_t newPlayerID;
	memcpy(&newPlayerID, buffer + 1, sizeof(newPlayerID));
	newPlayerID = ntohl(newPlayerID);

	// Validate new player exists
	if (newPlayerID >= MAX_CONNECTION || !serverData.totalClients[newPlayerID].connected)
	{
		return;
	}

	// Broadcast ship data to all other players
	ClientInfo &newClient = serverData.totalClients[newPlayerID];

	Packet shipPacket(NEW_PLAYER_JOIN);
	shipPacket << newPlayerID
		<< newClient.playerShip.xPos << newClient.playerShip.yPos
		<< newClient.playerShip.vel_x << newClient.playerShip.vel_y;

	MessageData msg;
	msg.commandID = shipPacket.id;
	msg.sessionID = -1; // Broadcast to all
	msg.data = shipPacket;

	std::lock_guard<std::mutex> lock(lockMutex);
	messageQueue.push(msg);
}
void ProcessGameStart(const sockaddr_in &clientAddr, const char *buffer, int recvLen)
{
	// Typically initiated by server, but could handle client-ready signals
	if (serverData.gameRunning)
	{
		return; // Game already running
	}

	serverData.gameRunning = true;

	// Broadcast game start to all clients
	Packet startPacket(GAME_START);

	MessageData msg;
	msg.commandID = startPacket.id;
	msg.sessionID = -1;
	msg.data = startPacket;

	std::lock_guard<std::mutex> lock(lockMutex);
	messageQueue.push(msg);

	if (debugPrint)
	{
		std::cout << "Game started!" << std::endl;
	}
}
void ProcessPacketError(const sockaddr_in &clientAddr, const char *buffer, int recvLen)
{
	if (recvLen < 1 + sizeof(uint32_t))
	{
		return; // Not enough data for error code
	}

	uint32_t errorCode;
	memcpy(&errorCode, buffer + 1, sizeof(errorCode));
	errorCode = ntohl(errorCode);

	// Log client-reported errors
	std::cerr << "Client reported error: " << errorCode << std::endl;

	// Optionally: Resend last packet to that client
	// ...
}