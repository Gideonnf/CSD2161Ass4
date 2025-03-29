/* Start Header
*****************************************************************/
/*!
\file echoclient.cpp
\author Gideon Francis (g.francis)
\par g.francis@digipen.edu
\date 22 Feb 2025
\brief
This file implements the client file which will be used to implement a 
simple echo server.
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header
*******************************************************************/


/*******************************************************************************
 * A simple TCP/IP client application
 ******************************************************************************/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "Windows.h"		// Entire Win32 API...
#include "winsock2.h"		// ...or Winsock alone
#include "ws2tcpip.h"		// getaddrinfo()
#include <thread>

// Tell the Visual Studio linker to include the following library in linking.
// Alternatively, we could add this file to the linker command-line parameters,
// but including it in the source code simplifies the configuration.
#pragma comment(lib, "ws2_32.lib")

#include <iostream>			// cout, cerr
#include <string>			// string
#include <vector>
#include <sstream>
#include <atomic>

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
#define SLEEP_TIME 500

std::atomic_bool connected{ true };

void SendMessages(SOCKET clientSocket);

// This program requires one extra command-line parameter: a server hostname.
int main(int argc, char** argv)
{
	constexpr uint16_t port = 2048;

	// Get IP Address
	std::string host{};
	std::cout << "Server IP Address: ";
	std::getline(std::cin, host);

	std::cout << std::endl;

	// Get Port Number
	std::string portNumber;
	std::cout << "Server Port Number: ";
	std::getline(std::cin, portNumber);

	std::cout << std::endl;

	std::string portString = portNumber;


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


	// -------------------------------------------------------------------------
	// Resolve a server host name into IP addresses (in a singly-linked list).
	//
	// getaddrinfo()
	// -------------------------------------------------------------------------

	// Object hints indicates which protocols to use to fill in the info.
	addrinfo hints{};
	SecureZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;			// IPv4
	hints.ai_socktype = SOCK_STREAM;	// Reliable delivery
	// Could be 0 to autodetect, but reliable delivery over IPv4 is always TCP.
	hints.ai_protocol = IPPROTO_TCP;	// TCP

	addrinfo* info = nullptr;
	errorCode = getaddrinfo(host.c_str(), portString.c_str(), &hints, &info);
	if ((NO_ERROR != errorCode) || (nullptr == info))
	{
		std::cerr << "getaddrinfo() failed." << std::endl;
		WSACleanup();
		return errorCode;
	}


	// -------------------------------------------------------------------------
	// Create a socket and attempt to connect to the first resolved address.
	//
	// socket()
	// connect()
	// -------------------------------------------------------------------------

	SOCKET clientSocket = socket(
		info->ai_family,
		info->ai_socktype,
		info->ai_protocol);
	if (INVALID_SOCKET == clientSocket)
	{
		std::cerr << "socket() failed." << std::endl;
		freeaddrinfo(info);
		WSACleanup();
		return RETURN_CODE_2;
	}

	errorCode = connect(
		clientSocket,
		info->ai_addr,
		static_cast<int>(info->ai_addrlen));
	if (SOCKET_ERROR == errorCode)
	{
		std::cerr << "connect() failed." << std::endl;
		freeaddrinfo(info);
		closesocket(clientSocket);
		WSACleanup();
		return RETURN_CODE_3;
	}

	// Enable non-blocking I/O on a socket.
	//u_long socket_args = 1;
	//ioctlsocket(clientSocket, FIONBIO, &socket_args);
	// Create a thread to handle receiving of messages from server
	std::thread sendMessageThread(SendMessages, clientSocket);
	// detach it so it runs in the background while we loop for input from client
	sendMessageThread.detach();

	while (connected)
	{
		Sleep(SLEEP_TIME);

		char buffer[MAX_STR_LEN];
		memset(buffer, 0, MAX_STR_LEN);
		int receivedBytes = recv(clientSocket, buffer, MAX_STR_LEN, 0);

		// disconnect
		if (receivedBytes <= 0)
		{
			std::cout << "Disconnecting..." << std::endl;
			connected = false;
			break;
		}

		int offset = 0;
		// get command ID
		uint8_t commandID = static_cast<uint8_t>(buffer[0]);
		offset += 1;

		if (commandID == REQ_ECHO || commandID == RSP_ECHO)
		{
			// extract out the ip address
			uint8_t ipBytes[4];
			memcpy(&ipBytes, buffer + offset, sizeof(ipBytes));
			offset += sizeof(ipBytes);

			uint16_t ipPort;
			memcpy(&ipPort, buffer + offset, sizeof(ipPort));
			offset += sizeof(ipPort);
			ipPort = ntohs(ipPort);

			std::string ipAddress = std::to_string(ipBytes[0]) + '.' +
				std::to_string(ipBytes[1]) + '.' +
				std::to_string(ipBytes[2]) + '.' +
				std::to_string(ipBytes[3]);

			// if its more than just an empty ip and port echo msg
			if (receivedBytes > 7)
			{
				uint32_t textLength;
				memcpy(&textLength, buffer + offset, sizeof(textLength));
				offset += sizeof(textLength);
				textLength = ntohl(textLength);


				std::string message(buffer + offset, textLength);

				std::cout << "==========RECV START=========" << std::endl;
				std::cout << ipAddress << ":" << ipPort << std::endl;
				std::cout << message << std::endl;
				std::cout << "==========RECV END=========" << std::endl;
			}
			// it only contains ip and port
			else
			{
				std::string message{};

				std::cout << "==========RECV START=========" << std::endl;
				std::cout << ipAddress << ":" << ipPort << std::endl;
				std::cout << message << std::endl;
				std::cout << "==========RECV END=========" << std::endl;

			}


			if (commandID == REQ_ECHO)
			{
				// send back to server RSP_ECHO
				buffer[0] = RSP_ECHO;
				send(clientSocket, buffer, receivedBytes, 0);
			}
		}
		else if (commandID == RSP_LISTUSERS)
		{
			uint16_t numOfUsers;
			memcpy(&numOfUsers, buffer + offset, sizeof(numOfUsers));
			offset += sizeof(numOfUsers);
			numOfUsers = ntohs(numOfUsers);

			std::cout << "==========RECV START==========" << std::endl;
			std::cout << "Users:" << std::endl;

			for (int i = 0; i < numOfUsers; ++i)
			{
				uint8_t ipBytes[4];
				uint16_t userPort;

				memcpy(&ipBytes, buffer + offset, sizeof(ipBytes));
				offset += sizeof(ipBytes);

				memcpy(&userPort, buffer + offset, sizeof(userPort));
				offset += sizeof(userPort);
				userPort = ntohs(userPort);

				std::string ipAddress = std::to_string(ipBytes[0]) + '.' +
					std::to_string(ipBytes[1]) + '.' +
					std::to_string(ipBytes[2]) + '.' +
					std::to_string(ipBytes[3]);

				std::cout << ipAddress << ":" << userPort << std::endl;
			}

			std::cout << "============RECV END===========" << std::endl;
		}
		else if (commandID == ECHO_ERROR)
		{
			std::cout << "==========RECV START=========" << std::endl;
			std::cout << "Echo Error" << std::endl;
			std::cout << "==========RECV END=========" << std::endl;

		}
		else if (commandID == REQ_UNKNOWN)
		{
			std::cout << "==========RECV START=========" << std::endl;
			std::cout << "Unknown Command" << std::endl;
			std::cout << "==========RECV END=========" << std::endl;
		}
		// client has to quit
		if (commandID == REQ_QUIT)
		{
			std::cout << "Disconnecting..." << std::endl;

			break;
		}
	}


	errorCode = shutdown(clientSocket, SD_SEND);
	if (SOCKET_ERROR == errorCode)
	{
		std::cerr << "shutdown() failed." << std::endl;
	}
	closesocket(clientSocket);


	// -------------------------------------------------------------------------
	// Clean-up after Winsock.
	//
	// WSACleanup()
	// -------------------------------------------------------------------------

	WSACleanup();
}

void SendMessages(SOCKET clientSocket)
{
	char buffer[MAX_STR_LEN];

	while (std::cin.getline(buffer, MAX_STR_LEN))
	{

		// get the user input
		Sleep(SLEEP_TIME);

		char message[MAX_STR_LEN] = { 0 };
		unsigned int messageSize = 0;

		// use this for testing as the pdf one isn't formatted correctly
		// 02c0a8010Bc6ca0000000568656c6c6f
		// if the user key in a /t command
		if (buffer[0] == '/' && buffer[1] == 't')
		{
			bool validMsg = true;

			// skip the "/t "
			const char* hexMessage = buffer + 3;
			size_t messageLength = strlen(hexMessage);
			unsigned int offset = 0;

			// check for command id

			// increment message size
			//messageSize++;
			if (validMsg)
			{
				// keep an offset for hexmessage
				//offset += 2;

				for (size_t i = offset; i < messageLength; i += 2)
				{
					// its not multiples of 2, so just skip the last character
					if (i + 1 >= messageLength) break;

					char byteVal[3] = { hexMessage[i], hexMessage[i + 1], '\0' };
					message[messageSize] = static_cast<char>(std::stoi(byteVal, nullptr, 16)); // 16 cause hexadecimal
					messageSize++;
				}

				// get length of message
				//unsigned int textLength;
				//size_t messageHexLength = strlen(message + 5);
				//memcpy(&textLength, &message[1], sizeof(textLength));
				//textLength = ntohl(textLength);

				//if (textLength != messageHexLength)
				//{
				//	std::cerr << "Mismatch in text length and message length" << std::endl;
				//	break;
				//}

			}

		}
		// if user key in a quit message
		else if (buffer[0] == '/' && buffer[1] == 'q')
		{
			// 1 is to quit
			message[0] = REQ_QUIT;
			messageSize += 1;
			/*break;*/
		}
		// list users
		else if (buffer[0] == '/' && buffer[1] == 'l')
		{
			message[0] = REQ_LISTUSERS;
			messageSize += 1;

		}
		else if (buffer[0] == '/' && buffer[1] == 'e')
		{
			message[0] = REQ_ECHO;
			messageSize += 1;

			std::stringstream inputStream(buffer + 3);
			std::string ipMessage, messageStr;

			inputStream >> ipMessage;

			std::getline(inputStream, messageStr);
			// erase the leading 0
			if (!messageStr.empty() && messageStr[0] == ' ')
			{
				messageStr.erase(0, 1);
			}

			size_t colonPosition = ipMessage.find(':');
			if (colonPosition == std::string::npos)
			{
				// error msg

				send(clientSocket, message, messageSize, 0);
				continue;
			}

			std::string ipAddress = ipMessage.substr(0, colonPosition);
			std::string portStr = ipMessage.substr(colonPosition + 1);

			// convert IP to 4 bytes for message
			std::stringstream ipStream(ipAddress);
			std::string byte;
			int i = 0;
			uint8_t ipBytes[4];
			// only 4 parts to an ip address, anymore and itll be an invalid message
			while (std::getline(ipStream, byte, '.') && i < 4)
			{
				ipBytes[i] = static_cast<uint8_t>(std::stoi(byte));
				i++;
			}

			// its not a valid ip address
			if (i != 4)
			{
				// error message
				send(clientSocket, message, messageSize, 0);
				continue;
			}

			// port to 2 bytes
			uint16_t port = static_cast<uint16_t>(std::stoi(portStr));
			port = htons(port); // convert to network byte order

			// 4 bytes for ip address
			memcpy(message + messageSize, ipBytes, 4);
			messageSize += 4;

			memcpy(message + messageSize, &port, 2);
			messageSize += 2;

			size_t messageLength = messageStr.size();
			uint32_t msgLen = htonl(static_cast<uint32_t>(messageLength));
			memcpy(message + messageSize, &msgLen, 4);
			messageSize += 4;

			memcpy(message + messageSize, messageStr.c_str(), messageLength);
			messageSize += (unsigned int)messageLength;
		}
		// send a normal message 
		else
		{
			// 2 is to echo
			message[0] = static_cast<char>(2);
			messageSize += 1;

			// copy the message length
			unsigned int messageLength = (unsigned int)htonl((u_long)strlen(buffer));
			memcpy(message + messageSize, &messageLength, sizeof(messageLength));
			messageSize += sizeof(messageLength);

			// copy the message
			memcpy(message + messageSize, &buffer, strlen(buffer));
			messageSize += (unsigned int)strlen(buffer);
		}

		send(clientSocket, message, messageSize, 0);

		memset(buffer, 0, MAX_STR_LEN);
		//// if the client is quitting
		//if (message[0] == static_cast<char>(1))
		//{
		//	//receiveMessageThread.join();
		//	// quitting
		//	break;
		//}
	}


	//int errorCode = shutdown(clientSocket, SD_SEND);
	//if (SOCKET_ERROR == errorCode)
	//{
	//	std::cerr << "shutdown() failed." << std::endl;
	//}
	//closesocket(clientSocket);
	//WSACleanup();
	//exit(EXIT_FAILURE);
}
