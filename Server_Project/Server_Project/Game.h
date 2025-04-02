#ifndef GAME_H
#define GAME_H

struct Ship
{
	float xPos;
	float yPos;
	float vel_x;
	float vel_y;
	float vel_server_x;
	float vel_server_y;
	int score;
	int ID;
};
struct Asteroid
{
	int ID;
	float xPos;
	float yPos;
	float vel_x;
	float vel_y;
	float vel_server_x;
	float vel_server_y;
};
struct Bullet
{
	int ownerID; // should be the same ID as the ship it belongs to
	bool active; // becomes inactive/destroyed after hitting an asteroid
	float xPos;
	float yPos;
	float vel_x;
	float vel_y;
	float vel_server_x;
	float vel_server_y;
};

#endif
