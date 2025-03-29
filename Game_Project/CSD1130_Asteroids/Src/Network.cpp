#include "Network.h"


int NetworkClient::Init()
{
	// read from text file
	std::string host{};
	std::string udpPortString;
	std::string clientUDPPortString;

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

	recvThread = std::thread(&NetworkClient::ReceiveMessages, udpSocket);
	senderThread = std::thread(&NetworkClient::SendMessages, udpSocket);
}

void NetworkClient::Shutdown()
{
	connected = false;

	// join back the two threads
	if (senderThread.joinable())
		senderThread.join();
	if (recvThread.joinable())
		recvThread.join();

	int errorCode = shutdown(udpSocket, SD_SEND);
	if (SOCKET_ERROR == errorCode)
	{
		std::cerr << "shutdown() failed" << std::endl;
	}

	closesocket(udpSocket);
	WSACleanup();
}

void NetworkClient::SendMessages(SOCKET clientSocket)
{
	while (connected)
	{

	}
}

void NetworkClient::ReceiveMessages(SOCKET udpSocket)
{
	while (connected)
	{

	}
}