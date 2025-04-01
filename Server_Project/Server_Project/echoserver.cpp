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

// Add these new handler functions:

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