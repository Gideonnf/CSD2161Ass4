#ifndef NETWORK_H
#define NETWORK_H
#include "Game.h"

#define MAX_CONNECTION 4

struct ClientInfo
{
	int sessionID{}; // represent the player's ID
	std::string ip;
	uint16_t port;

	Ship playerShip;

	// idk if i'll need this for doing packet loss shit
	//uint32_t seqNum{};
	std::unordered_map<int, bool> ackReceived;
	uint32_t retryCount{};

	bool connected{ false };
};

struct ServerData
{
	// max is 4
	ClientInfo totalClients[MAX_CONNECTION];

	// store a vector of all asteroids or smth
	std::vector<Asteroid> asteroids;
	// power ups

	// idk what else u want
	std::unordered_map<int, Bullet> activeBullets;
	int nextBulletID = 0;
};

#endif
