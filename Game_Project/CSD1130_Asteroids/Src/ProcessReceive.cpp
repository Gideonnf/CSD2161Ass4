#include "ProcessReceive.h"


void ProcessPacketMessages(std::string msg, GameData& data)
{
	if (msg.empty()) return;

	Packet newPacket(msg);

	//	char msgID = msg[0];


	switch (newPacket.id)
	{
		// switch cases
	case ASTEROID_CREATED: // temporary
		ProcessNewAsteroid(newPacket, data);
		break;
	}
}

void ProcessNewAsteroid(Packet& packet, GameData& data)
{
	float xPos;
	packet >> xPos;


	


}
