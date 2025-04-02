#ifndef PACKET_H
#define PACKET_H
#include <string>
#define MAX_STR_LEN         2048
#define MAX_BODY_LEN		2000 // change after we decide how big header should be

// Command ID stuff
enum CMDID : unsigned char {
	PLAYER_DC = 1,
	PLAYER_JOIN,
	REPLY_PLAYER_JOIN,
	NEW_PLAYER_JOIN,
	BULLET_COLLIDE,
	BULLET_CREATED,
	ASTEROID_CREATED,
	ASTEROID_DESTROYED,
	SHIP_MOVE,
	SHIP_COLLIDE,
	REQ_HIGHSCORE,
	NEW_HIGHSCORE,
	GAME_START,
	PACKET_ERROR
};

struct Packet
{
	char body[MAX_BODY_LEN];
	CMDID id;
	unsigned int headerOffset{};
	int sessionID{};
	size_t writePos = 0;
	size_t readPos = 0;

	Packet() : id{ PACKET_ERROR }
	{
		std::memset(body, 0, MAX_BODY_LEN);
	}

	Packet(CMDID packetID) : id(packetID)
	{
		std::memset(body, 0, MAX_BODY_LEN);
	}

	Packet(const std::string& str)
	{
		std::memset(body, 0, MAX_BODY_LEN);
		if (str.empty())
		{
			id = PACKET_ERROR;
			return;
		}

		id = static_cast<CMDID>(static_cast<unsigned char>(str[0]));

		size_t bodyLen = str.size() - 1; // take out the ID
		if (bodyLen > MAX_BODY_LEN - 1)
		{
			id = PACKET_ERROR;
			return;
		}

		std::memcpy(body, str.data() + 1, bodyLen);
		writePos = bodyLen;
		readPos = 0;
	}

	void Reset()
	{
		writePos = 0;
		readPos = 0;
		std::memset(body, 0, MAX_BODY_LEN);
	}

	std::string ToString()
	{
		std::string  str;
		str.reserve(1 + writePos); // + 1 cause space for ID

		// pack the ID at the front
		str.push_back(static_cast<char>(id));

		// then the rest of the variables
		str.append(body, writePos);

		return str;
	}
};


//struct MessageData
//{
//	// header stuff
//	// idk?
//	int sessionID{};
//	Packet data{};
//};





#pragma region DEFAULT_TEMPLATE
template <typename T>
Packet& operator<< (Packet& packet, const T& data)
{
	static_assert(sizeof(T) == 0, "Unsupported");

	return packet;
}

template <typename T>
Packet& operator>>(Packet& packet, T& data)
{
	static_assert(sizeof(T) == 0, "Unsupported");

	return packet;
}
#pragma endregion

// might need make uint16_t and uint8_t but i test if this works first

#pragma region UINT32_T SPECIALIZATION
template <>
inline Packet& operator<< (Packet& packet, const int32_t& data)
{
	uint32_t netVal = htonl(static_cast<uint32_t>(data));
	std::memcpy(packet.body + packet.writePos, &netVal, sizeof(netVal));
	packet.writePos += sizeof(netVal);
	return packet;
}

template <>
inline Packet& operator>>(Packet& packet, int32_t& data)
{
	// reading out of bounds
	if (packet.readPos + sizeof(uint32_t) > packet.writePos)
	{
		return packet;
	}

	uint32_t netVal;
	std::memcpy(&netVal, packet.body + packet.readPos, sizeof(netVal));
	packet.readPos += sizeof(netVal);
	data = static_cast<uint32_t>(ntohl(netVal));
	return packet;
}
#pragma endregion

#pragma region FLOAT SPECIALIZATION
inline uint32_t FloatToNet(float val)
{
	uint32_t net;
	std::memcpy(&net, &val, sizeof(float));
	return htonl(net);
}

inline float NetToFloat(uint32_t val)
{
	val = ntohl(val); // convert the bits
	float fVal;
	std::memcpy(&fVal, &val, sizeof(float));
	return fVal;
}

template<>
inline Packet& operator<<(Packet& packet, const float& data)
{
	uint32_t netVal = FloatToNet(data);
	if (packet.writePos + sizeof(netVal) > MAX_BODY_LEN)
	{
		// too chonky
		return packet;
	}

	std::memcpy(packet.body + packet.writePos, &netVal, sizeof(netVal));
	packet.writePos += sizeof(netVal);
	return packet;
}

template<>
inline Packet& operator>>(Packet& packet, float& data)
{
	if (packet.readPos + sizeof(uint32_t) > packet.writePos)
	{
		// reading out of bounds
		return packet;
	}

	uint32_t netVal;
	std::memcpy(&netVal, packet.body + packet.readPos, sizeof(netVal));
	packet.readPos += sizeof(netVal);
	data = NetToFloat(netVal);
	return packet;
}

#pragma endregion

#endif