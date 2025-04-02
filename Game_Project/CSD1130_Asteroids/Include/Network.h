#ifndef NETWORK_H
#define NETWORK_H


#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Packet.h>
#pragma comment(lib, "Ws2_32.lib")
#endif

#include <string>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>

#define WINSOCK_SUBVERSION  2
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
	~NetworkClient();
	NetworkClient() = default;

	int Init();
	void Shutdown();
	void SendMessages(SOCKET clientSocket);
	void ReceiveMessages(SOCKET udpSocket);
	Packet GetIncomingMessage();
	void CreateMessage(Packet msg);
	uint64_t GetTimeDiff();

private:
	SOCKET udpSocket;
	std::string serverIP;
	std::string serverPort;
	std::atomic_bool connected{ false };

	std::thread senderThread;
	std::thread recvThread;

	std::queue<Packet> incomingMessages;
	std::queue<Packet> outgoingMessages;
	std::mutex inMutex;
	std::mutex outMutex;

	std::chrono::steady_clock::time_point gameStartTime;


	// use mutex to share a queue between game loop and threads
	/*
	
	{
		std::lock_guard<std::mutex> lock(in/outmutex);
		then push any incoming messages or check if there is any outgoing messages
	}
	*/

};
#endif