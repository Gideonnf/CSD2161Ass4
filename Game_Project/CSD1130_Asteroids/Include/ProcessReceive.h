#ifndef PROCESS_RECEIVE_H
#define PROCESS_RECEIVE_H
#include <string>
#include "Entity.h"
#include "Packet.h"


void ProcessPacketMessages(std::string msg, GameData& data);
void ProcessNewAsteroid(Packet& packet, GameData& data);




#endif