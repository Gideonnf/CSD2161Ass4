#ifndef GAME_H
#define GAME_H
enum TARGETTYPE : unsigned int
{
	TARGET_TYPE_SHIP,
	TARGET_TYPE_ASTEROID,
	TARGET_TYPE_BULLET
};
struct Ship
{
	float xPos;
	float yPos;
	float vel_x;
	float vel_y;
	float vel_server_x;
	float vel_server_y;
	float dirCur;
	int score;
	int ID;
	TARGETTYPE targetType = TARGET_TYPE_SHIP;
};
struct Asteroid
{
	std::chrono::steady_clock::time_point creationTime;
	int ID;
	float xPos;
	float yPos;
	float xScale;
	float yScale;
	float vel_x;
	float vel_y;
	float vel_server_x;
	float vel_server_y;
	float dirCur;
	bool active;
	TARGETTYPE targetType = TARGET_TYPE_ASTEROID;
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
	TARGETTYPE targetType = TARGET_TYPE_BULLET;
};

#endif
