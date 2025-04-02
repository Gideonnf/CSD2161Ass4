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

	Packet newPlayer(PLAYER_JOIN);
	PushMessage(newPlayer);
	//{
	//	std::lock_guard<std::mutex> lock(outMutex);
	//	outgoingMessages.push(newPlayer.ToString());
	//}
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
			// send it?? process it?? idk
			//Read command ID and then construct the message 
			char buffer[MAX_STR_LEN];
			
			CreateMessage(outMsg, buffer);


			sockaddr_in udpServerAddress = {};
			udpServerAddress.sin_family = AF_INET;
			udpServerAddress.sin_port = htons(9999);
			inet_pton(AF_INET, serverIP.c_str(), &udpServerAddress.sin_addr);

			int sentBytes = sendto(clientSocket, buffer, outMsg.headerOffset, 0,
				reinterpret_cast<sockaddr*>(&udpServerAddress), sizeof(udpServerAddress));

			if (sentBytes == SOCKET_ERROR)
			{
				std::cerr << "Failed to send message!" << std::endl;
			}

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
	
	// -- TODO -- Basically take this my client's gameStart time -= what frame time the server says we start at
	// Which means that we can use the currTime to get my curr frame time to help sync
	time(&gameStartTime);

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
			uint8_t msgID = buffer[0];
			offset++;
			
			// if theres any other header stuff u need to take out cna do it here
			uint32_t seshID;
			memcpy(&seshID, buffer + offset, sizeof(seshID));
			seshID = ntohl(seshID);
			offset += sizeof(seshID);

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

		Sleep(SLEEP_TIME);
	}
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

void NetworkClient::PushMessage(Packet msg)
{
	{
		std::lock_guard<std::mutex> lock(outMutex);
		outgoingMessages.push(msg);
	}
}

void NetworkClient::CreateMessage(Packet msg, char* buffer)
{
	

	uint8_t commandID = static_cast<uint8_t>(msg.id);
	memcpy(&buffer[0], &commandID, sizeof(commandID));
	msg.headerOffset += 1;

	uint32_t sessID = static_cast<uint32_t>(msg.sessionID);
	memcpy(buffer + msg.headerOffset, &sessID, sizeof(sessID));
	msg.headerOffset += sizeof(sessID);

	// any other header stuff do here

	// get the file length/message length based on writePos
	uint32_t fileLength = static_cast<uint32_t>(msg.writePos);
	fileLength = htonl(fileLength);
	memcpy(buffer + msg.headerOffset, &fileLength, sizeof(fileLength));
	msg.headerOffset += sizeof(fileLength);


	// copy the body into the msg
	memcpy(buffer + msg.headerOffset, msg.body, msg.writePos);
	msg.headerOffset += msg.writePos;
}