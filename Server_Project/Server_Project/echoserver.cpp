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

#include "Windows.h"		// Entire Win32 API...
#include "winsock2.h"		// ...or Winsock alone
#include "ws2tcpip.h"		// getaddrinfo()
#include <filesystem>
#include <unordered_map>
#include <random>

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

void disconnect(SOCKET& listenerSocket);
bool execute(SOCKET clientSocket);
//void HandleEchoMessage(SOCKET clientSocket, char* buffer, int length);
void HandleListFiles(SOCKET clientSocket);
void HandleDownloadRequest(char* buffer, SOCKET clientSocket);
void UDPSendingHandler(uint32_t sessionID);
void UDPReceiveHandler(SOCKET udpListenerSocket);

static int userCount = 0;

struct ClientInfo
{
	std::string ip;
	uint16_t port;

	int sessionID{};
	//uint32_t seqNum{};
	std::unordered_map<int, bool> ackReceived;

	uint32_t fileSize{};
	uint32_t numPackets{};
	uint32_t currPackets{};
	uint32_t retryCount{};

	std::chrono::steady_clock::time_point startTime{};
	double totalTime;

	bool completed{ false };
};

// <sessionID, fileName>
std::unordered_map<uint32_t, std::string> sessionMapFiles;
std::unordered_map<uint32_t, ClientInfo> sessionClientMap;
std::vector<SOCKET> clientSockets;

SOCKET udpListenerSocket = INVALID_SOCKET;
std::string filePath; 
std::mt19937 generator;
std::uniform_real_distribution dis(0.0, 1.0);

bool debugPrint = false;

int main()
{
	// Get Port Number
	std::string input;
	std::cout << "Server TCP Port Number: ";
	std::getline(std::cin, input);
	std::string portStringTCP = input;

	// comment out for now cause idk how to do the UDP part yet
	
	std::cout << "Server UDP Port Number: ";
	std::getline(std::cin, input);
	std::string portStringUDP = input;

	std::cout << "File Path: ";
	std::getline(std::cin, input);
	filePath = input;

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
	
	addrinfo* info = nullptr;
	errorCode = getaddrinfo(host, portStringTCP.c_str(), &hints, &info);
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
	std::cout << "Server TCP Port Number: " << portStringTCP << std::endl;
	
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

	//if (info != nullptr)
	//{
	//	freeaddrinfo(info);
	//
	//}
	info = nullptr;
	errorCode = getaddrinfo(host, portStringUDP.c_str(), &hints, &info);
	if ((NO_ERROR != errorCode) || (nullptr == info))
	{
		std::cerr << "getaddrinfo() failed." << std::endl;
		WSACleanup();
		return errorCode;
	}


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

	//UDPReceiveHandler(udpListenerSocket);

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

	//std::thread tcpThread(TCPServerHandler, listenerSocket);



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

				/*char message[MAX_STR_LEN];
				unsigned int messageSize = 0;*/

				// unknown command sent
				/*	message[0] = REQ_QUIT;
				messageSize += 1;*/

				send(clientSocket, buffer, bytesReceived, 0);

			}
			

			break;
		case REQ_DOWNLOAD:
			HandleDownloadRequest(buffer, clientSocket);
			break;
		case RSP_DOWNLOAD:
			break;
		case REQ_LISTFILES:
			HandleListFiles(clientSocket);
			break;
		case RSP_LISTFILES:
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

void HandleListFiles(SOCKET clientSocket)
{
	char message[MAX_STR_LEN];
	memset(message, 0, sizeof(message));
	// Get the file path
	std::filesystem::path currentPath = std::filesystem::current_path();
	// all files stored in ServerProject/Files
	std::filesystem::path combineFilePath = currentPath / filePath;
	std::vector<std::string> fileList;
	if (std::filesystem::exists(combineFilePath) && std::filesystem::is_directory(combineFilePath))
	{
		for (const auto& entry : std::filesystem::directory_iterator(combineFilePath))
		{
			fileList.push_back(entry.path().filename().string());
			//std::cout << entry.path().filename().string() << std::endl;
		}
	}

	unsigned int messageSize = 0;

	message[0] = RSP_LISTFILES;
	messageSize += 1;

	// get the number of files
	uint16_t numOfFiles = static_cast<uint16_t>(fileList.size());
	uint16_t numFilesNetworkOrder = htons(numOfFiles);
	memcpy(&message[messageSize], &numFilesNetworkOrder, sizeof(numFilesNetworkOrder));
	messageSize += sizeof(numFilesNetworkOrder);

	
	// skip the message length first until after
	messageSize += 4; // 4 bytes of length of list
	unsigned int fileMessageSize = 0;

	for (auto it : fileList)
	{
		// get size of the file name and copy to msg
		uint32_t fileSize = static_cast<uint32_t>(it.length());
		uint32_t fileSizeNetworkOrder = htonl(fileSize);
		memcpy(&message[messageSize], &fileSizeNetworkOrder, sizeof(fileSizeNetworkOrder));
		messageSize += sizeof(fileSizeNetworkOrder);
		fileMessageSize += sizeof(fileSizeNetworkOrder);
		
		// now get the actual file name
		memcpy(&message[messageSize], it.c_str(), it.size());
		messageSize += (unsigned int)it.size();
		fileMessageSize += (unsigned int)it.size();
	}
	
	// go back to the message length
	uint32_t messageLengthNetworkOrder = htonl(fileMessageSize);
	memcpy(&message[3], &messageLengthNetworkOrder, sizeof(messageLengthNetworkOrder));
	
	send(clientSocket, message, messageSize, 0);
}

bool DoesFileExist(std::string fileName)
{
	std::filesystem::path currentPath = std::filesystem::current_path();
	// all files stored in ServerProject/Files
	std::filesystem::path combineFilePath = currentPath / filePath;
	
	if (std::filesystem::exists(combineFilePath) && std::filesystem::is_directory(combineFilePath))
	{
		for (const auto& entry : std::filesystem::directory_iterator(combineFilePath))
		{
			if (entry.path().filename().string() == fileName)
				return true;
			//fileList.push_back(entry.path().filename().string());
			//std::cout << entry.path().filename().string() << std::endl;
		}
	}

	return false;
}

bool SimulatePacketLost()
{
	double rand = dis(generator);
	//std::cout << rand << std::endl;
	return rand < PACKET_LOSS_RATE;
}

void HandleDownloadRequest(char* buffer, SOCKET clientSocket)
{
	char message[MAX_STR_LEN];
	memset(message, 0, sizeof(message));
	int offset = 1;

	// get the ip addr
	char clientIP[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &buffer[offset], clientIP, sizeof(clientIP));
	offset += 4;

	// get the port
	uint16_t clientPort;
	memcpy(&clientPort, &buffer[offset], 2);
	clientPort = ntohs(clientPort); // convert
	offset += 2;

	// get the length of file name
	uint32_t filenameLength;
	memcpy(&filenameLength, &buffer[offset], 4);
	filenameLength = ntohl(filenameLength); // convert
	offset += 4;

	// extract the file name after
	std::string fileName(buffer + offset, filenameLength);

	printf("Client requesting file: %s\n", fileName.c_str());

	if (!DoesFileExist(fileName))
	{
		printf("Client requested file that does not exist");
		unsigned int messageSize = 0;
		message[0] = DOWNLOAD_ERROR;
		messageSize++;

		// send RSP_DOWNLOAD msg to client
		send(clientSocket, message, messageSize, 0);

		return;
	}

	// get length of file
	std::filesystem::path currentPath = std::filesystem::current_path();
	// all files stored in ServerProject/Files
	std::filesystem::path combineFilePath = currentPath / filePath / fileName;
	
	FILE* file = nullptr;
	errno_t errorCode = fopen_s(&file, combineFilePath.string().c_str(), "rb");
	if (errorCode != 0)
	{
		std::cout << "Unable to open file " << combineFilePath.string() << std::endl;
		//printf("Unable to open file %s\n", combineFilePath.string());
	}

	fseek(file, 0, SEEK_END);
	uint32_t fileSize = ftell(file);
	rewind(file);

	fclose(file);

	// randomise a session ID for each client
	uint32_t sessionID = userCount;
	userCount++;


	// send RSP back
	unsigned int messageSize = 0;

	message[0] = RSP_DOWNLOAD;
	messageSize += 1;

	// copy the ip address
	memcpy(message + messageSize, buffer + messageSize, 4);
	messageSize += 4;

	// copy the port
	memcpy(message + messageSize, buffer + messageSize, 2);
	messageSize += 2;

	uint32_t sessionIDNetworkOrder = htonl(sessionID);
	memcpy(message + messageSize, &sessionIDNetworkOrder, sizeof(sessionIDNetworkOrder));
	messageSize += sizeof(sessionIDNetworkOrder);

	uint32_t fileSizeNetworkOrder = htonl(fileSize);
	memcpy(message + messageSize, &fileSizeNetworkOrder, sizeof(fileSizeNetworkOrder));
	messageSize += sizeof(fileSizeNetworkOrder);

	// send RSP_DOWNLOAD msg to client
	send(clientSocket, message, messageSize, 0);

	// start file transfer

	sessionMapFiles[sessionID] = fileName;
	sessionClientMap[sessionID] = { clientIP, clientPort };
	sessionClientMap[sessionID].sessionID = sessionID;

	std::cout << "New client with session ID: " << sessionID << std::endl;


	std::thread udpThread(UDPSendingHandler, sessionID);
	udpThread.detach(); // detach it cause it can end on its own once its done sending
}

void UDPSendingHandler(uint32_t sessionID)
{
	if (sessionMapFiles.count(sessionID) == 0)
	{
		// session ID not found
		printf("SessionID not found");
		return;
	}

	
	

	std::string fileName = sessionMapFiles[sessionID];
	ClientInfo& client = sessionClientMap[sessionID];


	struct sockaddr_in clientAddr;
	clientAddr.sin_family = AF_INET;
	clientAddr.sin_port = htons(client.port);
	inet_pton(AF_INET, client.ip.c_str(), &clientAddr.sin_addr);

	std::filesystem::path currentPath = std::filesystem::current_path();
	// all files stored in ServerProject/Files
	std::filesystem::path combineFilePath = currentPath / filePath / fileName;

	FILE* file = nullptr;
	errno_t errorCode = fopen_s(&file, combineFilePath.string().c_str(), "rb");
	if (errorCode != 0)
	{
		std::cout << "Unable to open file " << combineFilePath.string() << std::endl;
	}

	if (file == nullptr)
	{
		std::cout << "Unable to open file " << std::endl;
		return;
	}

	fseek(file, 0, SEEK_END);
	client.fileSize = ftell(file);
	rewind(file);



	char buffer[MAX_STR_LEN];
	uint32_t seqNum = 0;
	//std::unordered_map<int, bool> ackReceived;
	int messageHeader = 8;

	client.numPackets = client.fileSize / (MAX_STR_LEN - messageHeader) + ((client.fileSize % (MAX_STR_LEN - messageHeader) != 0) ? 1 : 0);

	auto startFileSend = std::chrono::steady_clock::now();


	while (!feof(file))
	{
		auto totalElapsedTime = std::chrono::steady_clock::now() - startFileSend;
		int sentPackets = 0;
		// send 5 packets first in one go
		while (sentPackets < WINDOW_SIZE && !feof(file))
		{
			memset(buffer, 0, sizeof(buffer));
			size_t bytesRead = fread(buffer, 1, (MAX_STR_LEN - 8), file);
			char packet[MAX_STR_LEN];

			uint32_t sessionIDNetworkOrder = htonl(sessionID);
			uint32_t seqNumNetworkOrder = htonl(seqNum);
			memcpy(packet, &sessionIDNetworkOrder, 4);
			memcpy(packet + 4, &seqNumNetworkOrder, 4);
			memcpy(packet + 8, buffer, bytesRead);

			if (SIMULATE_PACKET_LOSS)
			{
				if (SimulatePacketLost())
				{
					if (debugPrint)
						std::cout << "Failed sending packet to " << sessionID << " with seq num " << seqNum << std::endl;
				}
				else
				{
					sendto(udpListenerSocket, packet, (int)bytesRead + messageHeader, 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
					if (debugPrint)
						std::cout << "Sending packet to " << sessionID << " with seq num " << seqNum << std::endl;
				}

			}
			else
			{
				sendto(udpListenerSocket, packet, (int)bytesRead + messageHeader, 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
			}
			// store it's acknowledgement in a map to check for receiving back from client
			client.ackReceived[seqNum] = false;
			seqNum++;
			sentPackets++;
		}

		//std::cout << "total Elasped " << std::chrono::duration_cast<std::chrono::milliseconds>(totalElapsedTime).count() << std::endl;

		if (std::chrono::duration_cast<std::chrono::milliseconds>(totalElapsedTime).count() > PRINTOUT_MS)
		{
			startFileSend = std::chrono::steady_clock::now();
			// calculate percentage of packets sent vs packets left
			// num of packets sent / total packets * 100
			//int percentageDone = (int)(((seqNum + 1) / (float)client.numPackets) * 100.0f);

			std::lock_guard<std::mutex> userLock{ _stdoutMutex };
			std::cout << "Progress of file transfer : " << seqNum << "/" << client.numPackets << std::endl;
		}

		// start a timer to check for time out
		auto startTime = std::chrono::steady_clock::now();
		while (1)
		{

			// not all packets have been received yet
			bool allAcknowledged = true;
			for (const auto& ack : client.ackReceived)
			{
				// if any isn't yet then break
				if (ack.second == false)
				{
					allAcknowledged = false;
					break;
				}
			}

			// go to the next chunk
			if (allAcknowledged) break;

			// get the current elapsed time
			auto elapsedTime = std::chrono::steady_clock::now() - startTime;


			// if it reaches time out then find out which sequence didn't get received and send that
			if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsedTime).count() > TIMEOUT_MS)
			{
				//std::cout << "Receiving timed out, resending any unacknowledged packets" << std::endl;

				for (const auto& ackPair : client.ackReceived)
				{
					if (ackPair.second == false)
					{
						// find out where the unacknowledged sequence start
						fseek(file, ackPair.first * (MAX_STR_LEN - messageHeader), SEEK_SET);
						memset(buffer, 0, sizeof(buffer));
						size_t bytesRead = fread(buffer, 1, (MAX_STR_LEN - messageHeader), file);

						uint32_t sessionIDNetworkOrder = htonl(sessionID);
						uint32_t seqNumNetworkOrder = htonl(ackPair.first);


						char packet[MAX_STR_LEN];
						memcpy(packet, &sessionIDNetworkOrder, 4);
						memcpy(packet + 4, &seqNumNetworkOrder, 4);
						memcpy(packet + 8, buffer, bytesRead);

						sendto(udpListenerSocket, packet, (int)bytesRead + messageHeader, 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
					
						//if (debugPrint)
						{
							std::lock_guard<std::mutex> userLock{ _stdoutMutex };
							std::cout << "Resending packet " << ackPair.first << " to " << sessionID << std::endl;
						}
					}
				}

				client.retryCount++;
				startTime = std::chrono::steady_clock::now();
			}

			// if max retries
			if (client.retryCount == MAX_RETRIES)
			{
				std::cout << "Reaching max retries for " << client.sessionID << ". Ending Thread." << std::endl;
				break;
			}
		}

		if (client.retryCount == MAX_RETRIES)
		{
			break;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(50)); // sleep delay
	}

	fclose(file);
	printf("File transfer complete. \n");
}

void UDPReceiveHandler(SOCKET udpListenerSocket)
{

	u_long enable = 1;
	ioctlsocket(udpListenerSocket, FIONBIO, &enable);

	while (1)
	{
		char ackBuffer[8];
		struct sockaddr_in recvAddr;
		int recvAddrLen = sizeof(recvAddr);

		// receive from client
		int recvLen = recvfrom(udpListenerSocket, ackBuffer, sizeof(ackBuffer), 0, (struct sockaddr*)&recvAddr, &recvAddrLen);

		if (recvLen == SOCKET_ERROR)
		{
			size_t errCode = WSAGetLastError();
			if (errCode == WSAEWOULDBLOCK)
			{
				Sleep(200);
				continue;
			}
		}

		//if (recvLen < 0)
		//{
		//	/*size_t errCode = WSAGetLastError();
		//	if (errCode == WSAEWOULDBLOCK)
		//	{
		//		Sleep(200);
		//		continue;
		//	}*/
		//	//Sleep(200);
		//	continue;
		//}

		if (recvLen == 0)
		{
			// mutex lock
			std::cout << "Shutdown" << std::endl;
			break;
		}

		if (recvLen > 0)
		{

			uint32_t ackSessionID{};
			uint32_t ackSeqNum{};
			memcpy(&ackSessionID, ackBuffer, 4);
			memcpy(&ackSeqNum, ackBuffer + 4, 4);
			ackSessionID = ntohl(ackSessionID);
			ackSeqNum = ntohl(ackSeqNum);

			if (debugPrint)
				std::cout << "Receiving packet from " << ackSessionID << " with seq num " << ackSeqNum << std::endl;

			// Check through active file transfer list
			// if client ID is the same as the sessionID
			if (sessionClientMap.count(ackSessionID) == 0)
			{
				continue;
			}

			sessionClientMap[ackSessionID].ackReceived[ackSeqNum] = true;
			// received a packet
			sessionClientMap[ackSessionID].currPackets++;
			// reset retry count if its successful
			sessionClientMap[ackSessionID].retryCount = 0;

			if (sessionClientMap[ackSessionID].currPackets == sessionClientMap[ackSessionID].numPackets)
			{
				auto chronoEnd = std::chrono::steady_clock::now() - sessionClientMap[ackSessionID].startTime;
				sessionClientMap[ackSessionID].totalTime = (double)std::chrono::duration_cast<std::chrono::milliseconds>(chronoEnd).count();
				// received all packets
				sessionClientMap[ackSessionID].completed = true;

				if (debugPrint)
					std::cout << ackSessionID << " has finished downloading the file" << std::endl;

				//break;

				// send TCP msg for completed download
			}

		}


	}

}