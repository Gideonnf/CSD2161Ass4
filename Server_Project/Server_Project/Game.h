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
};

#endif
