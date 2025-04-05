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
#include "Vec2.h"
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
const float			FIXED_DELTA_TIME = 0.01667f; // Fixed time step for 60 FPS
#define X_SIZE 640
#define Y_SIZE 360
const float         ASTEROID_ACCEL = 100.0f;
#define ASTEROID_SCORE 100;


// Add these new handler functions:
void HandleSubmitScore(char *buffer, SOCKET clientSocket);
void ProcessPlayerDisconnect(const char *buffer, int recvLen);
void ProcessPlayerJoin(const sockaddr_in &clientAddr, const char *buffer, int recvLen);
void HandleGetScores(SOCKET clientSocket);
void FixedUpdate();
void UDPSendingHandler();
void UDPReceiveHandler(SOCKET udpListenerSocket);
void ProcessShipMovement(const sockaddr_in& clientAddr, const char* buffer, int recvLen);
void ForwardPacket(const sockaddr_in& clientAddr, const char* buffer, int recvLen);
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

void ClientHandleHighscoreRequest(const sockaddr_in &clientAddr, const char *buffer, int recvLen);

void RespawnShip(uint32_t playerID);

static int userCount = 0;

std::mutex lockMutex;
std::queue<MessageData> messageQueue;

static ServerData serverData;
bool gameOver = false;

SOCKET udpListenerSocket = INVALID_SOCKET;
std::string filePath;
std::mt19937 generator;
std::uniform_real_distribution dis(0.0, 1.0);
double accumulatedTime = 0.0;

bool debugPrint = false;

float generateRandomFloat(float min, float max) {
	// Create a random engine (using the current time as a seed)
	std::random_device rd;
	std::mt19937 gen(rd());

	// Create a uniform distribution in the range [min, max]
	std::uniform_real_distribution<> dis(min, max);

	// Generate a random float
	return (float)dis(gen);
}

Asteroid RandomiseAsteroid(float min_xPos, float max_xPos, float min_yPos, float max_yPos)
{
	Carmicah::Vec2f pos, vel, scale;
	pos.x = generateRandomFloat(min_xPos, max_xPos);//AERandFloat() * (max_xPos - min_xPos) + min_xPos;
	pos.y = generateRandomFloat(min_yPos, max_yPos);//AERandFloat() * (max_yPos - min_yPos) + min_yPos;
	float randAng = generateRandomFloat(0, 360);//AERandFloat() * 360;

	Carmicah::Vec2f dir(cosf(randAng), sinf(randAng));
	//AEVec2Set(&dir, cosf(randAng), sinf(randAng));
	dir = dir.normalize();
	//AEVec2Normalize(&dir, &dir);
	vel = Carmicah::Vec2f(0, 0);
	//AEVec2Zero(&ve
	dir *= ASTEROID_ACCEL;
	//AEVec2Scale(&dir, &dir, ASTEROID_ACCEL);
	//AEVec2Add(&vel, &vel, &dir);
	vel += dir;

	float randScaleX = 20.0f;// AERandFloat() * (ASTEROID_MAX_SCALE_X - ASTEROID_MIN_SCALE_X) + ASTEROID_MIN_SCALE_X;
	float randScaleY = 20.0f; //AERandFloat() * (ASTEROID_MAX_SCALE_Y - ASTEROID_MIN_SCALE_Y) + ASTEROID_MIN_SCALE_Y;
	//AEVec2Set(&scale, randScaleX, randScaleY);
	scale = Carmicah::Vec2f(randScaleX, randScaleY);

	Asteroid newAsteroid;
	newAsteroid.xPos = pos.x;
	newAsteroid.yPos = pos.y;
	newAsteroid.dirCur = randAng;
	newAsteroid.vel_x = vel.x;
	newAsteroid.vel_y = vel.y;
	newAsteroid.xScale = randScaleX;
	newAsteroid.yScale = randScaleY;
	newAsteroid.active = true;
	newAsteroid.creationTime = std::chrono::steady_clock::now();

	//for (int i = 0; i < MAX_ASTEROIDS; ++i)
	//{
	//	if (serverData.totalAsteroids[i].active == false)
	//	{
	//		newAsteroid.ID = i;
	//		serverData.totalAsteroids[i] = newAsteroid;
	//		break;
	//	}
	//}

	newAsteroid.ID = serverData.activeAsteroids;
	serverData.totalAsteroids[serverData.activeAsteroids] = newAsteroid;
	std::cout << "Creating Asteroid " << newAsteroid.ID << std::endl;

	serverData.activeAsteroids++;
	serverData.numOfAsteroids++;

	return newAsteroid;

	// Create after randomising
	//CreateAsteroid(pos, vel, scale);
}

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
	//std::thread fixedUpdateThread(FixedUpdate);
	//std::thread udpSendThread(UDPSendingHandler);

	const auto interval = std::chrono::duration<double>(0.005); // every 5 ms? idk for now
	auto lastSendTime = std::chrono::steady_clock::now();

	auto lastPrintTime = std::chrono::steady_clock::now();
	const auto printInterval = std::chrono::duration<double>(1.0); // every 1 s? idk for now

	auto lastWaveTime = std::chrono::steady_clock::now();
	const auto waveInterval = std::chrono::duration<double>(10.0); // every 10?

	auto startTime = std::chrono::steady_clock::now();
	const auto gameTime = std::chrono::duration<double>(120.0);

	serverData.gameRunning = false;

	while (true)
	{
		auto currTime = std::chrono::steady_clock::now();

		if (currTime - lastPrintTime >= printInterval)
		{
			lastPrintTime = currTime;

			// if no more asteroids can spawn and all are destroyed
			if (serverData.activeAsteroids >= (MAX_ASTEROIDS - 1) && serverData.numOfAsteroids <= 1)
			{
				// game over
				Packet gameOverPkt(GAME_OVER);
				int winnerID = 0;
				//get the highest score player
				for (int i = 1; i < MAX_CONNECTION; ++i)
				{
					if (serverData.totalClients[i].playerShip.score > serverData.totalClients[winnerID].playerShip.score)
					{
						winnerID = i;
					}
				}

				gameOverPkt << winnerID;

				LoadHighScores();
				if (gameOver == false)
				{

					std::string playerName = "Player_" + std::to_string(winnerID);

					auto now = std::chrono::system_clock::now();
					std::time_t now_time = std::chrono::system_clock::to_time_t(now);

					struct tm time_info;
					// Use localtime_s for safer date-time conversion
					localtime_s(&time_info, &now_time);

					std::stringstream ss;
					ss << std::put_time(&time_info, "%Y-%m-%d %H:%M:%S");

					std::string time = ss.str();
					UpdateHighScores(playerName, serverData.totalClients[winnerID].playerShip.score, time);
					SaveHighScores();
					gameOver = true;
				}
				// send it to client
				{
					MessageData newMessage;
					newMessage.commandID = gameOverPkt.id;
					newMessage.sessionID = -1;// sending to the current client's id which is i 
					newMessage.data = gameOverPkt;

					std::lock_guard<std::mutex> lock(lockMutex);
					messageQueue.push(newMessage);
				}


				// Create response packet
				Packet highscorePacket(CLIENT_REQ_HIGHSCORE);

				// Pack number of scores
				uint16_t numScores = static_cast<uint16_t>(topScores.size());
				highscorePacket << numScores;

				// Pack each score
				for (const auto& score : topScores)
				{
					highscorePacket << score.playerName << score.score << score.time;
				}

				{
					MessageData highScoreMsg;
					highScoreMsg.commandID = highscorePacket.id;
					highScoreMsg.sessionID = -1;//client.sessionID; // Broadcast to all
					highScoreMsg.data = highscorePacket;

					std::lock_guard<std::mutex> lock(lockMutex);
					messageQueue.push(highScoreMsg);
				}


			}
		}


		if (currTime - lastWaveTime >= waveInterval)
		{
			lastWaveTime = currTime;

			
			if (serverData.gameRunning && (serverData.activeAsteroids + 8) <= MAX_ASTEROIDS )
			{
				std::vector<Asteroid> newAsteroids;

				for (int i = 0; i < 2; ++i)
				{
					// Spawn from bottom of screen
					newAsteroids.push_back(RandomiseAsteroid(-X_SIZE, X_SIZE, -Y_SIZE, -Y_SIZE));
					// Spawn from top of screen
					newAsteroids.push_back(RandomiseAsteroid(-X_SIZE, X_SIZE, Y_SIZE, Y_SIZE));
					//// Spawn from Left of screen
					newAsteroids.push_back(RandomiseAsteroid(-X_SIZE, -X_SIZE, -Y_SIZE, Y_SIZE));
					//// Spawn from Right of screen
					newAsteroids.push_back(RandomiseAsteroid(X_SIZE, X_SIZE, -Y_SIZE, Y_SIZE));
				}

				Packet asteroidPacket(ASTEROID_CREATED);

				asteroidPacket << (int)newAsteroids.size();

				// pack all the new asteroids into the packet
				for (int i = 0; i < newAsteroids.size(); ++i)
				{
					asteroidPacket << newAsteroids[i].ID;
					asteroidPacket << newAsteroids[i].xPos;
					asteroidPacket << newAsteroids[i].yPos;
					asteroidPacket << newAsteroids[i].vel_x;
					asteroidPacket << newAsteroids[i].vel_y;
					asteroidPacket << newAsteroids[i].dirCur;
				}

				// send it to client
				{
					MessageData newMessage;
					newMessage.commandID = asteroidPacket.id;
					//newMessage.sessionID = newClient.sessionID;// sending to the current client's id which is i 
					newMessage.data = asteroidPacket;

					std::lock_guard<std::mutex> lock(lockMutex);
					messageQueue.push(newMessage);
				}
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
				//case CLIENT_REQ_HIGHSCORE:
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
					offset += (int)msg.data.writePos;
					sendto(udpListenerSocket, buffer, offset, 0, (sockaddr*)&otherAddr, sizeof(otherAddr));
					break;
				}
				case NEW_PLAYER_JOIN:
				{
					// this packet contains every player data
					// create the message buffer ifrst
					std::memcpy(buffer + offset, msg.data.body, msg.data.writePos);
					offset += (int)msg.data.writePos;

					// loop through every client to send this msg to them
					for (int i = 0; i < MAX_CONNECTION; ++i)
					{
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
				{
					//msg.data << msg.sessionID;
					std::memcpy(buffer + offset, msg.data.body, msg.data.writePos);
					offset += (int)msg.data.writePos;
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
				}
				case ASTEROID_UPDATE:
				case ASTEROID_CREATED:
				{
					std::memcpy(buffer + offset, msg.data.body, msg.data.writePos);
					offset += (int)msg.data.writePos;
					for (int i = 0; i < MAX_CONNECTION; ++i)
					{
						ClientInfo& client = serverData.totalClients[i];
						if (!client.connected) continue; // skip unconnected client slots
						//if (client.sessionID == msg.sessionID) continue; // dont update for hte client thats moving

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
				case PLAYER_DC:
					std::memcpy(buffer + offset, msg.data.body, msg.data.writePos);
					offset += (int)msg.data.writePos;

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
					offset += (int)msg.data.writePos;

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
				default:
					std::memcpy(buffer + offset, msg.data.body, msg.data.writePos);
					offset += (int)msg.data.writePos;

					for (int i = 0; i < MAX_CONNECTION; ++i)
					{
						ClientInfo& client = serverData.totalClients[i];
						if (!client.connected) continue;
						if (i == msg.sessionID) continue;

						sockaddr_in clientAddr;
						memset(&clientAddr, 0, sizeof(clientAddr));
						clientAddr.sin_family = AF_INET;
						clientAddr.sin_port = htons(client.port);
						inet_pton(AF_INET, client.ip.c_str(), &clientAddr.sin_addr);

						sendto(udpListenerSocket, buffer, offset, 0, (sockaddr*)&clientAddr, sizeof(clientAddr));
					}
					break;
				}
				// pop the message im using
				messages.pop();
			}
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
			case CLIENT_REQ_HIGHSCORE:
				ClientHandleHighscoreRequest(recvAddr, buffer, recvLen);
				break;
			case ASTEROID_DESTROYED:
			{
				int offset = 1;
				// get rid of header data
				uint32_t msgLength;
				std::memcpy(&msgLength, buffer + offset, sizeof(msgLength));
				msgLength = ntohl(msgLength);
				offset += sizeof(msgLength);

				Packet asteroidPkt(ASTEROID_DESTROYED);
				asteroidPkt.writePos += msgLength;
				std::memcpy(asteroidPkt.body, buffer + offset, msgLength);

				int asteroidID;
				asteroidPkt >> asteroidID;

				if (asteroidID > MAX_ASTEROIDS) break; // not suppose to be more

				if (serverData.totalAsteroids[asteroidID].active)
				{
					std::cout << "Destroying Asteroid " << asteroidID << std::endl;
					serverData.totalAsteroids[asteroidID].active = false;
					serverData.numOfAsteroids--;
				}

				//std::cout << "Total Asteroid : " << serverData.activeAsteroids << std::endl;

				break;
			}
			case BULLET_COLLIDE:
			{
				int offset = 1;
				// get rid of header data
				uint32_t msgLength;
				std::memcpy(&msgLength, buffer + offset, sizeof(msgLength));
				msgLength = ntohl(msgLength);
				offset += sizeof(msgLength);

				Packet bulletCollide(BULLET_COLLIDE);
				bulletCollide.writePos += msgLength;
				std::memcpy(bulletCollide.body, buffer + offset, msgLength);
				uint64_t timeDiff;
				int shipID;
				uint32_t score;
				uint32_t j, i;

				bulletCollide >> shipID >> timeDiff >> j >> i >> score;
				serverData.totalClients[shipID].playerShip.score = score;

				ForwardPacket(recvAddr, buffer, recvLen);
				break;
			}
			case SHIP_SCORE:
			{
				int offset = 1;
				// get rid of header data
				uint32_t msgLength;
				std::memcpy(&msgLength, buffer + offset, sizeof(msgLength));
				msgLength = ntohl(msgLength);
				offset += sizeof(msgLength);

				Packet pck(SHIP_SCORE);
				pck.writePos += msgLength;
				std::memcpy(pck.body, buffer + offset, msgLength);

				int shipID;
				uint32_t score;
				pck >> shipID >> score;
				serverData.totalClients[shipID].playerShip.score = score;

				ForwardPacket(recvAddr, buffer, recvLen);
				break;
			}
			default:
				ForwardPacket(recvAddr, buffer, recvLen);
				break;
			}
		}
	}
}

//void FixedUpdate()
//{
//	accumulatedTime += 
//}

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

	//if (availID == -1)
	//{
	bool clientExist = false;
	// check if the client connected before
	std::string ip = inet_ntoa(clientAddr.sin_addr);
	std::string port = std::to_string(ntohs(clientAddr.sin_port));
	std::string mapKey = ip + ":" + port;
	if (serverData.playerMap.count(mapKey) > 0)
	{
		availID = serverData.playerMap[mapKey];
		clientExist = true;
	}

		// send a rej packet but i lazy do that now
		//return;
	//}

	if (availID < 0) return; // shouldn't hit tho

	ClientInfo &newClient = serverData.totalClients[availID];

	newClient.sessionID = availID;
	newClient.ip = inet_ntoa(clientAddr.sin_addr);
	newClient.port = ntohs(clientAddr.sin_port);
	newClient.connected = true;

	std::string key = newClient.ip + ":" + std::to_string(newClient.port);

	serverData.playerMap[key] = availID;
	// the moment the first client joins, i set game running to true
	serverData.gameRunning = true;

	// default initialize ship data
	// send back to the connecting player the reply
	Packet replyPacket(REPLY_PLAYER_JOIN);
	replyPacket << availID; // pack the ship's ID in 
	replyPacket << (uint8_t)clientExist;
	if (clientExist)
	{
		ClientInfo& info = serverData.totalClients[availID];
		//replyPacket << playerIDs[i];
		replyPacket << info.playerShip.xPos;
		replyPacket << info.playerShip.yPos;
		replyPacket << info.playerShip.vel_x;
		replyPacket << info.playerShip.vel_y;
		replyPacket << info.playerShip.dirCur;
		replyPacket << info.playerShip.score;
	}
	/*
	
					asteroidPacket << newAsteroids[i].ID;
					asteroidPacket << newAsteroids[i].xPos;
					asteroidPacket << newAsteroids[i].yPos;
					asteroidPacket << newAsteroids[i].vel_x;
					asteroidPacket << newAsteroids[i].vel_y;
					asteroidPacket << newAsteroids[i].dirCur;
*/

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

	//if (playerIDs.size() <= 1) return; // if onyl 1 player has been made then dont send anything

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
		newPlayerPacket << info.playerShip.score;
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

	Packet updateAsteroids(ASTEROID_UPDATE);

	auto currTime = std::chrono::steady_clock::now();
	for (int i = 0; i < MAX_ASTEROIDS; ++i)
	{
		if (serverData.totalAsteroids[i].active == false) continue;

		auto elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(currTime - serverData.totalAsteroids[i].creationTime).count();
		serverData.totalAsteroids[i].xPos += serverData.totalAsteroids[i].vel_x * elapsedTime;
		serverData.totalAsteroids[i].yPos += serverData.totalAsteroids[i].vel_y * elapsedTime;

	}

	updateAsteroids << serverData.numOfAsteroids;
	// send any asteroids that exist in the server
	for (int i = 0; i < MAX_ASTEROIDS; ++i)
	{
		if (serverData.totalAsteroids[i].active == false) continue;

		updateAsteroids << i;
		updateAsteroids << serverData.totalAsteroids[i].xPos;
		updateAsteroids << serverData.totalAsteroids[i].yPos;
		updateAsteroids << serverData.totalAsteroids[i].vel_x;
		updateAsteroids << serverData.totalAsteroids[i].vel_y;
		updateAsteroids << serverData.totalAsteroids[i].dirCur;

	}


	{
		MessageData newMessage;
		newMessage.commandID = updateAsteroids.id;
		//newMessage.sessionID = newClient.sessionID;// sending to the current client's id which is i 
		newMessage.data = updateAsteroids;

		std::lock_guard<std::mutex> lock(lockMutex);
		messageQueue.push(newMessage);
	}
}
void ProcessShipMovement(const sockaddr_in& clientAddr, const char* buffer, int recvLen)
{
	//std::string ip = inet_ntoa(clientAddr.sin_addr);
	// get the id of the ship thats moving
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
	ClientInfo& client = serverData.totalClients[sessionID];

	shipMovement >> timeDiff;
	shipMovement >> playerInput;
	shipMovement >> client.playerShip.xPos;
	shipMovement >> client.playerShip.yPos;
	shipMovement >> client.playerShip.vel_x;
	shipMovement >> client.playerShip.vel_y;
	shipMovement >> client.playerShip.dirCur;
	shipMovement >> client.playerShip.score;


	{
		MessageData newMessage;
		newMessage.commandID = shipMovement.id;
		newMessage.sessionID = client.sessionID;// sending to the current client's id which is i 
		newMessage.data = shipMovement;

		std::lock_guard<std::mutex> lock(lockMutex);
		messageQueue.push(newMessage);
	}

}
void ForwardPacket(const sockaddr_in& clientAddr, const char* buffer, int recvLen)
{
	int offset = 1;

	// get rid of header data
	uint32_t msgLength;
	std::memcpy(&msgLength, buffer + offset, sizeof(msgLength));
	msgLength = ntohl(msgLength);
	offset += sizeof(msgLength);

	Packet returnPacket(static_cast<CMDID>(buffer[0]));
	returnPacket.writePos += msgLength;
	std::memcpy(returnPacket.body, buffer + offset, msgLength);

	int sessionID;
	returnPacket >> sessionID;
	ClientInfo& client = serverData.totalClients[sessionID];

	{
		MessageData newMessage;
		newMessage.commandID = returnPacket.id;
		newMessage.sessionID = client.sessionID;// sending to the current client's id which is i 
		newMessage.data = returnPacket;

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
void ClientHandleHighscoreRequest(const sockaddr_in &clientAddr, const char *buffer, int recvLen)
{

	int offset = 1;

	// get rid of header data
	uint32_t msgLength;
	std::memcpy(&msgLength, buffer + offset, sizeof(msgLength));
	msgLength = ntohl(msgLength);
	offset += sizeof(msgLength);

	Packet returnPacket(static_cast<CMDID>(buffer[0]));
	returnPacket.writePos += msgLength;
	std::memcpy(returnPacket.body, buffer + offset, msgLength);

	int sessionID;
	returnPacket >> sessionID;
	ClientInfo &client = serverData.totalClients[sessionID];

	LoadHighScores();

	// Create response packet
	Packet highscorePacket(CLIENT_REQ_HIGHSCORE);

	// Pack number of scores
	uint16_t numScores = static_cast<uint16_t>(topScores.size());
	highscorePacket << numScores;

	// Pack each score
	for (const auto &score : topScores)
	{
		highscorePacket << score.playerName << score.score << score.time;
	}

	{
		MessageData highScoreMsg;
		highScoreMsg.commandID = highscorePacket.id;
		highScoreMsg.sessionID = client.sessionID; // Broadcast to all
		highScoreMsg.data = highscorePacket;

		std::lock_guard<std::mutex> lock(lockMutex);
		messageQueue.push(highScoreMsg);
	}
	//// Send the response directly to the requesting client
	//sendto(udpListenerSocket, highscorePacket.body, highscorePacket.writePos, 0,
	//	(struct sockaddr *)&clientAddr, sizeof(clientAddr));
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
