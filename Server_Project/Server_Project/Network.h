#ifndef NETWORK_H
#define NETWORK_H
#include "Game.h"

#define MAX_CONNECTION 4
#define MAX_ASTEROIDS 50

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
	std::unordered_map<std::string, int> playerMap;

	// store a vector of all asteroids or smth
	//std::vector<Asteroid> asteroids;
	Asteroid totalAsteroids[MAX_ASTEROIDS];

	int activeAsteroids = 0;
	// power ups

	// idk what else u want
	bool gameRunning;
	std::unordered_map<int, Bullet> activeBullets;
	int nextBulletID = 0;
};

#endif
