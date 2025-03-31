#include "Network.h"
#include <filesystem>
#include <unordered_map>
#include <map>

#include <filesystem>
#include <map>
#include <iostream>			// cout, cerr
#include <string>			// string
#include <vector>
#include <sstream>

#include <fstream>


//#define WINSOCK_VERSION     2


#define SLEEP_TIME 500


int NetworkClient::Init()
{
	// read from text file
	std::string host = "192.168.1.13"; // Server IP
	std::string udpPortString = "9999"; // Server Port
	std::string clientUDPPortString = "8888";

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

	udpSocket = socket(udpInfo->ai_family, udpInfo->ai_socktype, udpInfo->ai_protocol);
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

	connected = true;

	sockaddr_in udpServerAddress{};
	udpServerAddress.sin_family = AF_INET;
	udpServerAddress.sin_port = htons(std::stoi(udpPortString));
	inet_pton(udpServerAddress.sin_family, host.c_str(), &udpServerAddress.sin_addr);

	recvThread = std::thread(&NetworkClient::ReceiveMessages, this, udpSocket);
	recvThread.detach();
	senderThread = std::thread(&NetworkClient::SendMessages, this, udpSocket);
	senderThread.detach();
}

NetworkClient::~NetworkClient()
{
	Shutdown();
}

void NetworkClient::Shutdown()
{

	if (connected)
	{
		connected = false;

		// join back the two threads
		//if (senderThread.joinable())
		//	senderThread.join();
		//if (recvThread.joinable())
		//	recvThread.join();

		int errorCode = shutdown(udpSocket, SD_SEND);
		if (SOCKET_ERROR == errorCode)
		{
			std::cerr << "shutdown() failed" << std::endl;
		}

		closesocket(udpSocket);
		WSACleanup();

	}
}

//Reading and sending the message
void NetworkClient::SendMessages(SOCKET clientSocket)
{
	while (connected)
	{
		std::string outMsg;
		{
			std::lock_guard<std::mutex> lock(outMutex);
			if (!outgoingMessages.empty())
			{
				outMsg = outgoingMessages.front();
				outgoingMessages.pop();
			}
		}

		// if its not an empty msg
		if (!outMsg.empty())
		{
			// send it?? process it?? idk
			//Read command ID and then construct the message 
			//if()
		}

		Sleep(SLEEP_TIME);
	}
}
//Take out the header and parse the message before adding it into the queue
//Queue msg should be the commandID and the message
void NetworkClient::ReceiveMessages(SOCKET udpSocket)
{
	sockaddr_in senderAddr;
	int senderAddrSize = sizeof(senderAddr);
	
	while (connected)
	{
		char buffer[MAX_STR_LEN];
		memset(buffer, 0, MAX_STR_LEN);
		int receivedBytes = recvfrom(udpSocket, buffer, MAX_STR_LEN, 0,
			reinterpret_cast<sockaddr*>(&senderAddr), &senderAddrSize);

		if (receivedBytes != SOCKET_ERROR)
		{
			buffer[receivedBytes] = '\0';

			// idk if we're going to process the string here into a message struct
			// or leave it as a string then process it in gamestate_asteroids or smth

			//{
			//std::lock_guard<std::mutex> lock(inMutex);
			//incomingMessages.push(buffer);
			//}
		}

		Sleep(SLEEP_TIME);
	}
}

std::string NetworkClient::GetIncomingMessage()
{
	std::string outMsg{};
	{
		std::lock_guard<std::mutex> lock(inMutex);
		if (!incomingMessages.empty())
		{
			outMsg = incomingMessages.front();
			incomingMessages.pop();
		}
	}

	// will return empty string if nth in incoming message
	return outMsg;
}

void NetworkClient::CreateMessage(std::string msg)
{
	{
		std::lock_guard<std::mutex> lock(outMutex);
		outgoingMessages.push(msg);
	}
}