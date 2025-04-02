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
#define REQ_SUBMIT_SCORE ((unsigned char)0x6)
#define RSP_SUBMIT_SCORE ((unsigned char)0x7)
#define REQ_GET_SCORES ((unsigned char)0x8)
#define RSP_GET_SCORES ((unsigned char)0x9)
#define SLEEP_TIME 500



// Add these new handler functions:
void HandleSubmitScore(char *buffer, SOCKET clientSocket);
void ProcessPlayerJoin(const sockaddr_in& clientAddr, const char* buffer, int recvLen);
void HandleGetScores(SOCKET clientSocket);
void UDPSendingHandler();
void UDPReceiveHandler(SOCKET udpListenerSocket);

void ProcessBulletFired(const sockaddr_in &clientAddr, const char *buffer, int recvLen);

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

	addrinfo* info = nullptr;

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
	struct sockaddr_in* serverAddress = reinterpret_cast<struct sockaddr_in*> (info->ai_addr);
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

				const MessageData& msg = messages.front();
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
					// create the message buffer ifrst
					std::memcpy(buffer + offset, msg.data.body, msg.data.writePos);
					offset += sizeof(msg.data.writePos);

					// loop through every client to send this msg to them
					for (int i = 0; i < MAX_CONNECTION; ++i)
					{
						// in this case, sessionID of message is used to represent who is joining
						if (i == msg.sessionID) continue; // dont send to the new player joining

						ClientInfo& client = serverData.totalClients[i];
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
						sendto(udpListenerSocket, buffer, offset, 0, (sockaddr*)&clientAddr, sizeof(clientAddr));
					}
					break;
				}
				case SHIP_MOVE:
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
		int recvLen = recvfrom(udpListenerSocket, buffer, sizeof(buffer), 0, (struct sockaddr*)&recvAddr, &recvAddrLen);

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
			case PLAYER_JOIN:
				ProcessPlayerJoin(recvAddr, buffer, recvLen);
				break;
			case SHIP_MOVE:
				break;

			case REPLY_PLAYER_JOIN:
				break;
			case NEW_PLAYER_JOIN:
				break;
			case BULLET_COLLIDE:
				break;
			case BULLET_CREATED:
				break;
			case ASTEROID_CREATED:
				break;
			case ASTEROID_DESTROYED:
				break;
			case SHIP_COLLIDE:
				break;
			case REQ_HIGHSCORE:
				break;
			case NEW_HIGHSCORE:
				break;
			case GAME_START:
				break;
			case PACKET_ERROR:		
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
void ProcessPlayerJoin( const sockaddr_in& clientAddr, const char* buffer,  int recvLen)
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

	ClientInfo& newClient = serverData.totalClients[availID];

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