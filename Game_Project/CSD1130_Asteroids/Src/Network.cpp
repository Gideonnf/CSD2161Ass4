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


#define SLEEP_TIME 50


uint32_t seqNum = 0;
sockaddr_in udpClientAddress{};
sockaddr_in udpServerAddress{};


int NetworkClient::Init()
{
	std::ifstream serverConfigFile("serverConfig.txt");

	if (!serverConfigFile.is_open())
	{
		std::cerr << "File not found!" << std::endl;
	}

	std::string fileText;

	// read from text file
	std::string host; // Server IP
	std::string udpPortString; // Server Port
	std::string clientUDPPortString;

	std::getline(serverConfigFile, host);
	std::getline(serverConfigFile, udpPortString);
	std::getline(serverConfigFile, clientUDPPortString);

	serverIP = host;
	serverPort = udpPortString;

	//std::cout << host << " " << udpPortString << " " << clientUDPPortString << std::endl;

	serverConfigFile.close();


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

	udpClientAddress.sin_family = AF_INET;
	udpClientAddress.sin_addr.s_addr = INADDR_ANY;
	udpClientAddress.sin_port = htons((u_short)std::stoi(clientUDPPortString));

	udpServerAddress.sin_family = AF_INET;
	udpServerAddress.sin_port = htons((u_short)std::stoi(udpPortString));
	inet_pton(AF_INET, serverIP.c_str(), &udpServerAddress.sin_addr);


	if (bind(udpSocket, reinterpret_cast<sockaddr*>(&udpClientAddress), sizeof(udpClientAddress)) == SOCKET_ERROR)
	{
		std::cerr << "UDP bind() failed." << std::endl;
		closesocket(udpSocket);
		WSACleanup();
		return RETURN_CODE_4;
	}

	connected = true;

	NetworkClient::Instance().gameStartTime = std::chrono::high_resolution_clock::now();


	recvThread = std::thread(&NetworkClient::ReceiveMessages, this, udpSocket);
	recvThread.detach();
	senderThread = std::thread(&NetworkClient::SendMessages, this, udpSocket);
	senderThread.detach();

	Packet newPlayer(PLAYER_JOIN);
	CreateMessage(newPlayer);
	//{
	//	std::lock_guard<std::mutex> lock(outMutex);
	//	outgoingMessages.push(newPlayer.ToString());
	//}

	return 1;
}

NetworkClient::~NetworkClient()
{
	// send disconnect message
	// ok i cant do this here
	//Packet dcPacket(PLAYER_DC);
	//CreateMessage(dcPacket);

	Shutdown();
}

void NetworkClient::Shutdown()
{

	if (connected)
	{
		connected = false;

		SendSingularMessage(udpSocket, shutdownPck);

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

void NetworkClient::SetShutdownPCK(int currID)
{
	shutdownPck = Packet(CMDID::PLAYER_DC);
	shutdownPck << currID;
}

//Reading and sending the message
void NetworkClient::SendMessages(SOCKET clientSocket)
{

	while (connected)
	{
		Packet outMsg;
		{
			std::lock_guard<std::mutex> lock(outMutex);
			if (!outgoingMessages.empty())
			{
				outMsg = outgoingMessages.front();
				outgoingMessages.pop();
			}
		}

		// if its not an empty msg
		if (outMsg.id != PACKET_ERROR)
		{
			SendSingularMessage(clientSocket, outMsg);
		}

		//Sleep(SLEEP_TIME);
	}
}

void  NetworkClient::SendSingularMessage(SOCKET clientSocket, Packet msg)
{
	// send it?? process it?? idk
	//Read command ID and then construct the message 
	char buffer[MAX_STR_LEN];

	unsigned int headerOffset = 0;

	uint8_t commandID = static_cast<uint8_t>(msg.id);
	memcpy(&buffer[0], &commandID, sizeof(commandID));
	headerOffset += 1;

	// any other header stuff do here

	// get the file length/message length based on writePos
	uint32_t fileLength = static_cast<uint32_t>(msg.writePos);
	fileLength = htonl(fileLength);
	memcpy(buffer + headerOffset, &fileLength, sizeof(fileLength));
	headerOffset += sizeof(fileLength);


	// copy the body into the msg
	memcpy(buffer + headerOffset, msg.body, msg.writePos);
	headerOffset += (int)msg.writePos;

	int sentBytes = sendto(clientSocket, buffer, headerOffset, 0,
		reinterpret_cast<sockaddr*>(&udpServerAddress), sizeof(udpServerAddress));

	if (sentBytes == SOCKET_ERROR)
	{
		std::cerr << "Failed to send message!" << std::endl;
	}

}
//Take out the header and parse the message before adding it into the queue
//Queue msg should be the commandID and the message
void NetworkClient::ReceiveMessages(SOCKET _udpSocket)
{
	UNREFERENCED_PARAMETER(_udpSocket);
	sockaddr_in senderAddr;
	int senderAddrSize = sizeof(senderAddr);
	
	// -- TODO -- Basically take this my client's gameStart time -= what frame time the server says we start at
	// Which means that we can use the currTime to get my curr frame time to help sync

	while (connected)
	{
		char buffer[MAX_STR_LEN];
		memset(buffer, 0, MAX_STR_LEN);
		int receivedBytes = recvfrom(udpSocket, buffer, MAX_STR_LEN, 0,
			reinterpret_cast<sockaddr*>(&senderAddr), &senderAddrSize);

		if (receivedBytes != SOCKET_ERROR)
		{
			int offset = 0;
			//buffer[receivedBytes] = '\0';
			char msgID = buffer[0];
			offset++;
			
			// if theres any other header stuff u need to take out cna do it here

			// get the file length
			uint32_t msgLength;
			memcpy(&msgLength, buffer + offset, sizeof(msgLength));
			msgLength = ntohl(msgLength);
			offset += sizeof(msgLength);

			// create the packet for game to process
			Packet newPacket(static_cast<CMDID>(msgID));
			newPacket.writePos = msgLength;
			memcpy(newPacket.body, buffer + offset, msgLength);


			{
			std::lock_guard<std::mutex> lock(inMutex);
			incomingMessages.push(newPacket);
			}
		}

		//Sleep(SLEEP_TIME);
	}
}

uint64_t NetworkClient::GetTimeDiff()
{
	auto timestamp = std::chrono::high_resolution_clock::now();
	uint64_t timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(timestamp - gameStartTime).count();
	return timeDiff;
}

Packet NetworkClient::GetIncomingMessage()
{
	Packet outMsg{};
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

void NetworkClient::CreateMessage(Packet msg)
{
	{
		std::lock_guard<std::mutex> lock(outMutex);
		outgoingMessages.push(msg);
	}
}