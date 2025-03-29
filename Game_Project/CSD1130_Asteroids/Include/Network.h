#pragma once
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
#include <queue>

//#define WINSOCK_VERSION     2
#define WINSOCK_SUBVERSION  2
#define MAX_STR_LEN         2048
#define RETURN_CODE_1       1
#define RETURN_CODE_2       2
#define RETURN_CODE_3       3
#define RETURN_CODE_4       4

class NetworkClient
{
public:
	static NetworkClient& Instance() {
		static NetworkClient instance;
		return instance;
	}

	NetworkClient(const NetworkClient&) = delete;
	NetworkClient& operator=(const NetworkClient&) = delete;
	NetworkClient() = default;

	int Init();
	void Shutdown();
	void SendMessages(SOCKET clientSocket);
	void ReceiveMessages(SOCKET udpSocket);
	void ProcessMessages(std::string message);

private:
	SOCKET udpSocket;
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
