#ifndef NETWORK_H
#define NETWORK_H

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#endif

#include <string>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>

#define WINSOCK_SUBVERSION  2
#define MAX_STR_LEN         2048
#define RETURN_CODE_1       1
#define RETURN_CODE_2       2
#define RETURN_CODE_3       3
#define RETURN_CODE_4       4

// Command ID stuff
enum CMDID {
	PLAYER_DC = (unsigned char)0x1,
	PLAYER_JOIN = (unsigned char)0x2,
	BULLET_COLLIDE = (unsigned char)0x3,
	BULLET_CREATED = (unsigned char)0x4,
	ASTEROID_CREATED = (unsigned char)0x5,
	ASTEROID_DESTROYED = (unsigned char)0x6,
	SHIP_MOVE = (unsigned char)0x7,
	SHIP_COLLIDE = (unsigned char)0x8,
	REQ_HIGHSCORE = (unsigned char)0x9,
	NEW_HIGHSCORE = (unsigned char)0x10,
	GAME_START = (unsigned char)0x20,
	DOWNLOAD_ERROR = (unsigned char)0x30,
};

struct MessageData
{
	// header stuff
	// idk?
	CMDID commandID;
	int sessionID{};
	int seqNum{};
};

class NetworkClient
{
public:
	static NetworkClient& Instance() {
		static NetworkClient instance;
		return instance;
	}

	NetworkClient(const NetworkClient&) = delete;
	NetworkClient& operator=(const NetworkClient&) = delete;
	~NetworkClient();
	NetworkClient() = default;

	int Init();
	void Shutdown();
	void SendMessages(SOCKET clientSocket);
	void ReceiveMessages(SOCKET udpSocket);
	std::string GetIncomingMessage();

private:
	SOCKET udpSocket;
	std::string serverIP;
	std::string serverPort;
	std::atomic_bool connected{ false };

	std::thread senderThread;
	std::thread recvThread;

	std::queue<std::string> incomingMessages;
	std::queue<std::string> outgoingMessages;
	std::mutex inMutex;
	std::mutex outMutex;

	// use mutex to share a queue between game loop and threads
	/*
	
	{
		std::lock_guard<std::mutex> lock(in/outmutex);
		then push any incoming messages or check if there is any outgoing messages
	}
	*/

};

#endif