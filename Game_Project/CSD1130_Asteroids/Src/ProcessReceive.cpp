#include "ProcessReceive.h"


void ProcessMessages(std::string msg, GameData& data)
{
	if (msg.empty()) return;

	//Packet newPacket(msg); data

	char msgID = msg[0];

	switch (msgID)
	{
		// switch cases
	case 0: // temporary
		//ProcessNewAsteroid(msg, data);
		break;
	}
}
