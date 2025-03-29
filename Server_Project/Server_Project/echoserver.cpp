/* Start Header
*****************************************************************/
/*!
\file echoserver.cpp
\author Gideon Francis (g.francis)
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

#include "Windows.h"		// Entire Win32 API...
#include "winsock2.h"		// ...or Winsock alone
#include "ws2tcpip.h"		// getaddrinfo()


// Tell the Visual Studio linker to include the following library in linking.
// Alternatively, we could add this file to the linker command-line parameters,
// but including it in the source code simplifies the configuration.
#pragma comment(lib, "ws2_32.lib")
#include <cstdio>
#include <iostream>			   // cout, cerr
#include <string>			     // string

#include "taskqueue.h"

//#define WINSOCK_VERSION     2
#define WINSOCK_SUBVERSION  2
#define MAX_STR_LEN         2048
#define RETURN_CODE_1       1
#define RETURN_CODE_2       2
#define RETURN_CODE_3       3
#define RETURN_CODE_4       4
#define REQ_UNKNOWN ((unsigned char)0x0)
#define REQ_QUIT       ((unsigned char)0x1)
#define REQ_ECHO       ((unsigned char)0x2)
#define RSP_ECHO       ((unsigned char)0x3)
#define REQ_LISTUSERS  ((unsigned char)0x4)
#define RSP_LISTUSERS  ((unsigned char)0x5)
#define CMD_TEST       ((unsigned char)0x20)
#define ECHO_ERROR     ((unsigned char)0x30)

void disconnect(SOCKET& listenerSocket);
bool execute(SOCKET clientSocket);
void HandleEchoMessage(SOCKET clientSocket, char* buffer, int length);
void HandleListUsers(SOCKET clientSocket);


std::vector<SOCKET> clientSockets;

int main()
{
	// Get Port Number
	std::string portNumber;
	std::cout << "Server Port Number: ";
	std::getline(std::cin, portNumber);

	std::string portString = portNumber;

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
	
	addrinfo* info = nullptr;
	errorCode = getaddrinfo(host, portString.c_str(), &hints, &info);
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
	std::cout << "Server Port Number: " << portString << std::endl;
	
	// -------------------------------------------------------------------------
	// Create a socket and bind it to own network interface controller.
	//
	// socket()
	// bind()
	// -------------------------------------------------------------------------

	SOCKET listenerSocket = socket(
		hints.ai_family,
		hints.ai_socktype,
		hints.ai_protocol);
	if (INVALID_SOCKET == listenerSocket)
	{
		std::cerr << "socket() failed." << std::endl;
		freeaddrinfo(info);
		WSACleanup();
		return RETURN_CODE_1;
	}

	errorCode = bind(
		listenerSocket,
		info->ai_addr,
		static_cast<int>(info->ai_addrlen));
	if (NO_ERROR != errorCode)
	{
		std::cerr << "bind() failed." << std::endl;
		closesocket(listenerSocket);
		listenerSocket = INVALID_SOCKET;
	}

	freeaddrinfo(info);

	if (INVALID_SOCKET == listenerSocket)
	{
		std::cerr << "bind() failed." << std::endl;
		WSACleanup();
		return RETURN_CODE_2;
	}


	// -------------------------------------------------------------------------
	// Set a socket in a listening mode and accept 1 incoming client.
	//
	// listen()
	// accept()
	// -------------------------------------------------------------------------

	errorCode = listen(listenerSocket, SOMAXCONN);
	if (NO_ERROR != errorCode)
	{
		std::cerr << "listen() failed." << std::endl;
		closesocket(listenerSocket);
		WSACleanup();
		return RETURN_CODE_3;
	}

	{
		const auto onDisconnect = [&]() { disconnect(listenerSocket); };

		auto tq = TaskQueue<SOCKET, decltype(execute), decltype(onDisconnect)>{ 10, 20, execute, onDisconnect };

		while (listenerSocket != INVALID_SOCKET)
		{
			sockaddr clientAddress{};
			SecureZeroMemory(&clientAddress, sizeof(clientAddress));
			int clientAddressSize = sizeof(clientAddress);
			SOCKET clientSocket = accept(
				listenerSocket,
				&clientAddress,
				&clientAddressSize);
			if (clientSocket == INVALID_SOCKET)
			{
				continue;
			}

			char clientIPAddr[MAX_STR_LEN];
			char clientPort[MAX_STR_LEN];
			getpeername(clientSocket, &clientAddress, &clientAddressSize);
			getnameinfo(&clientAddress, clientAddressSize, clientIPAddr, sizeof(clientIPAddr), clientPort, sizeof(clientPort), NI_NUMERICHOST);
			std::cout << std::endl;
			std::cout << "Client IP Address: " << clientIPAddr << std::endl;
			std::cout << "Client Port Number: " << clientPort << std::endl;
			//std::cout << "Connection established! Press 'Enter' to quit -";
			std::cout << std::endl;

			clientSockets.push_back(clientSocket);

			tq.produce(clientSocket);
		}
	}

	// -------------------------------------------------------------------------
	// Clean-up after Winsock.
	//
	// WSACleanup()
	// -------------------------------------------------------------------------

	WSACleanup();
}

void disconnect(SOCKET& listenerSocket)
{
	if (listenerSocket != INVALID_SOCKET)
	{
		for (auto it = clientSockets.begin(); it != clientSockets.end(); ++it)
		{
			//
			if (*it == listenerSocket)
			{
				clientSockets.erase(it);
				break;
			}
		}
		shutdown(listenerSocket, SD_BOTH);
		closesocket(listenerSocket);
		listenerSocket = INVALID_SOCKET;
	}
}

bool execute(SOCKET clientSocket)
{

	// -------------------------------------------------------------------------
	// Receive some text and send it back.
	//
	// recv()
	// send()
	// -------------------------------------------------------------------------

	while (true)
	{
		char buffer[MAX_STR_LEN] = { 0 };

		const int bytesReceived = recv(
			clientSocket,
			buffer,
			MAX_STR_LEN,
			0);
		if (bytesReceived == SOCKET_ERROR)
		{
			size_t errorCode = WSAGetLastError();
			if (errorCode == WSAEWOULDBLOCK)
			{
				// A non-blocking call returned no data; sleep and try again.
				//using namespace std::chrono_literals;
				//std::this_thread::sleep_for(200ms);

				//std::lock_guard<std::mutex> usersLock{ _stdoutMutex };
				std::cerr << "trying again..." << std::endl;
				continue;
			}
			//std::lock_guard<std::mutex> usersLock{ _stdoutMutex };
			std::cerr << "recv() failed." << std::endl;
			break;
		}
		if (bytesReceived == 0)
		{
			//std::lock_guard<std::mutex> usersLock{ _stdoutMutex };
			std::cerr << "Graceful shutdown." << std::endl;
			break;
		}

		// get command ID
		uint8_t commandID = static_cast<uint8_t>(buffer[0]);

		switch (commandID)
		{
		case REQ_QUIT:
			{
				sockaddr_in senderAddr;
				int addrSize = sizeof(senderAddr);
				// get the sender's ip and port
				getpeername(clientSocket, (sockaddr*)&senderAddr, &addrSize);
				char clientIPAddr[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, &senderAddr.sin_addr, clientIPAddr, INET_ADDRSTRLEN);
				int clientPort = ntohs(senderAddr.sin_port);

				std::lock_guard<std::mutex> usersLock{ _stdoutMutex };
				std::cout << "Client " << clientIPAddr << ":" << clientPort << " is shutting down" << std::endl;

				//std::cout << "Connection established! Press 'Enter' to quit -";
				//std::cout << std::endl;

				char message[MAX_STR_LEN];
				unsigned int messageSize = 0;

				// unknown command sent
			/*	message[0] = REQ_QUIT;
				messageSize += 1;*/

				send(clientSocket, buffer, bytesReceived, 0);

			}
			

			break;
		case REQ_ECHO:
			//std::cout << "Echo message" << std::endl;
			HandleEchoMessage(clientSocket, buffer, bytesReceived);
			break;
		case RSP_ECHO:
			//std::cout << "RSP_Echo" << std::endl;
			HandleEchoMessage(clientSocket, buffer, bytesReceived);
			break;
		case REQ_LISTUSERS:
			HandleListUsers(clientSocket);
			break;
		default:
		{
			char message[MAX_STR_LEN];
			unsigned int messageSize = 0;

			// unknown command sent
			message[0] = REQ_UNKNOWN;
			messageSize += 1;

			send(clientSocket, message, messageSize, 0);
		}

			break;
		}
	}

	// -------------------------------------------------------------------------
	// Shut down and close sockets.
	//
	// shutdown()
	// closesocket()
	// -------------------------------------------------------------------------

	for (auto it = clientSockets.begin(); it != clientSockets.end(); ++it)
	{
		//
		if (*it == clientSocket)
		{
			clientSockets.erase(it);
			break;
		}
	}
	shutdown(clientSocket, SD_BOTH);
	closesocket(clientSocket);
	return true;
}

void HandleListUsers(SOCKET clientSocket)
{
	char message[MAX_STR_LEN];

	unsigned int messageSize = 0;

	message[0] = RSP_LISTUSERS;
	messageSize += 1;

	// get the number of users
	uint16_t numOfUsers = static_cast<uint16_t>(clientSockets.size());
	uint16_t numUsersNetworkOrder = htons(numOfUsers);
	memcpy(&message[messageSize], &numUsersNetworkOrder, sizeof(numUsersNetworkOrder));
	messageSize += sizeof(numUsersNetworkOrder);

	for (auto it = clientSockets.begin(); it != clientSockets.end(); ++it)
	{
		sockaddr_in clientAddr;
		int addrSize = sizeof(clientAddr);

		if (getpeername(*it, (sockaddr*)&clientAddr, &addrSize) == 0)
		{
			// store in 4 bytes
			uint32_t ipAddr = clientAddr.sin_addr.s_addr;
			memcpy(&message[messageSize], &ipAddr, sizeof(ipAddr));
			messageSize += sizeof(ipAddr);

			// Store in 2 bytes
			uint16_t port = clientAddr.sin_port;
			memcpy(&message[messageSize], &port, sizeof(port));
			messageSize += sizeof(port);
		}
	}
	//message[messageSize] = '\0';
	//messageSize++;
	//std::cout << message << std::endl;
	send(clientSocket, message, messageSize, 0);
}

void HandleEchoMessage(SOCKET clientSocket, char* buffer, int length)
{
	// start at 1 because 0 is the command ID
	size_t messageOffset = 1;
	char message[MAX_STR_LEN];
	size_t messageSize = 0;

	// check length of message first to make sure it contains destination IP and Port
	// has to be minimum 7 because ID(1), IP(4), Port(2)
	if (length < 7)
	{
		memset(message, 0, MAX_STR_LEN);
		message[0] = ECHO_ERROR; // error message
		messageSize = 1;

		int byteSent = send(clientSocket, message, (int)messageSize, 0);
		if (byteSent <= 0)
		{
			std::cout << "Error?" << std::endl;
		}
		return;
	}
	// command ip and port
	//02c0a8010dd161
	// msg
	// 0000000568656c6c6f
	// 
	// get the destination ip
	uint32_t destinationIP;
	memcpy(&destinationIP, &buffer[messageOffset], sizeof(destinationIP));
	//destinationIP = ntohl(destinationIP); // convert network byte order
	messageOffset += sizeof(destinationIP); // increase size by 4 bytes

	// get the destination port
	uint16_t destinationPort;
	memcpy(&destinationPort, &buffer[messageOffset], sizeof(destinationPort));
	destinationPort = ntohs(destinationPort); // convert network byte order
	messageOffset += sizeof(destinationPort);


	// copy the original message
	memcpy(message, buffer, MAX_STR_LEN);

	// Replace the command id
	//message[0] = 3;
	messageSize++;

	bool found = false;

	for (auto it = clientSockets.begin(); it != clientSockets.end(); ++it)
	{
		sockaddr_in clientAddr;
		int addrSize = sizeof(clientAddr);

		if (getpeername(*it, (sockaddr*)&clientAddr, &addrSize) == 0)
		{
			// matches
			// it found the target
			//std::cout << clientAddr.sin_addr.s_addr << "vs" << destinationIP << std::endl;
			if (clientAddr.sin_addr.s_addr == destinationIP && ntohs(clientAddr.sin_port) == destinationPort)
			{
				//{
				//	std::lock_guard<std::mutex> usersLock{ _stdoutMutex };
				//	// send back to sender
				//	int byteSent = send(clientSocket, buffer, length, 0);
				//	if (byteSent <= 0)
				//	{
				//		std::cout << "Error?" << std::endl;
				//	}
				//}

				sockaddr_in senderAddr;
				int addrSize = sizeof(senderAddr);
				// get the sender's ip and port
				getpeername(clientSocket, (sockaddr*)&senderAddr, &addrSize);

				// replace the ip address in message
				memcpy(&buffer[1], &senderAddr.sin_addr, sizeof(senderAddr.sin_addr));
				messageSize += sizeof(senderAddr.sin_addr);

				// replace the port address in message
				memcpy(&buffer[5], &senderAddr.sin_port, sizeof(senderAddr.sin_port));
				messageSize += sizeof(senderAddr.sin_port);

				{
					std::lock_guard<std::mutex> usersLock{ _stdoutMutex };
					// forward the message to client
					int bytesSent = send(*it, buffer, length, 0);

					if (bytesSent <= 0)
					{
						std::cout << "error?" << std::endl;
					}
				}

				return;
			}
		}
	}

	// if it reaches here means destination doesn't exist
	memset(message, 0, MAX_STR_LEN);
	message[0] = ECHO_ERROR; // error message
	messageSize = 1;

	int byteSent = send(clientSocket, message, (int)messageSize, 0);
	if (byteSent <= 0)
	{
		std::cout << "Error?" << std::endl;
	}

	// get the text message
	/*uint32_t textLength;
	memcpy(&textLength, &buffer[messageOffset], sizeof(textLength));
	textLength = ntohl(textLength);
	messageOffset += sizeof(textLength);*/

	
}