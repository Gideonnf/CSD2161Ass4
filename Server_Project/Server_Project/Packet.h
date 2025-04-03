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
	STATE_UPDATE,
	BULLET_COLLIDE, // when bullet collides it gets destroyed, set Bullet active to false
	BULLET_CREATED, // bullet fired
	ASTEROID_CREATED,
	ASTEROID_UPDATE,
	ASTEROID_DESTROYED,
	SHIP_RESPAWN,
	SHIP_MOVE,
	SHIP_COLLIDE,
	REQ_HIGHSCORE,
	CLIENT_REQ_HIGHSCORE,
	NEW_HIGHSCORE,
	GAME_START,
	GAME_OVER,
	PACKET_ERROR
};

struct Packet
{
	char body[MAX_BODY_LEN];
	CMDID id;
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


struct MessageData
{
	// header stuff
	// idk?
	CMDID commandID;
	int sessionID{};
	int seqNum{};
	int fileLength{};
	int headerOffset{};
	int dataLength{};
	Packet data{};
};





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

#pragma region INT8_T SPECIALIZATION
template <>
inline Packet& operator<< (Packet& packet, const uint8_t& data)
{
	uint8_t netVal = data;
	std::memcpy(packet.body + packet.writePos, &netVal, sizeof(netVal));
	packet.writePos += sizeof(netVal);
	return packet;
}

template <>
inline Packet& operator<< (Packet& packet, const int8_t& data)
{
	int8_t netVal = data;
	std::memcpy(packet.body + packet.writePos, &netVal, sizeof(netVal));
	packet.writePos += sizeof(netVal);
	return packet;
}

template <>
inline Packet& operator>>(Packet& packet, uint8_t& data)
{
	// reading out of bounds
	if (packet.readPos + sizeof(uint8_t) > packet.writePos)
	{
		return packet;
	}

	uint8_t netVal;
	std::memcpy(&netVal, packet.body + packet.readPos, sizeof(netVal));
	packet.readPos += sizeof(netVal);
	data = netVal;
	return packet;
}

template <>
inline Packet& operator>>(Packet& packet, int8_t& data)
{
	// reading out of bounds
	if (packet.readPos + sizeof(int8_t) > packet.writePos)
	{
		return packet;
	}

	int8_t netVal;
	std::memcpy(&netVal, packet.body + packet.readPos, sizeof(netVal));
	packet.readPos += sizeof(netVal);
	data = netVal;
	return packet;
}

#pragma endregion

#pragma region INT16_T SPECIALIZATION
template <>
inline Packet& operator<< (Packet& packet, const int16_t& data)
{
	uint16_t netVal = htons(static_cast<uint16_t>(data));
	std::memcpy(packet.body + packet.writePos, &netVal, sizeof(netVal));
	packet.writePos += sizeof(netVal);
	return packet;
}

inline Packet& operator<< (Packet& packet, const uint16_t& data)
{
	uint16_t netVal = htons(static_cast<uint16_t>(data));
	std::memcpy(packet.body + packet.writePos, &netVal, sizeof(netVal));
	packet.writePos += sizeof(netVal);
	return packet;
}

template <>
inline Packet& operator>>(Packet& packet, int16_t& data)
{
	// reading out of bounds
	if (packet.readPos + sizeof(int16_t) > packet.writePos)
	{
		return packet;
	}

	uint16_t netVal;
	std::memcpy(&netVal, packet.body + packet.readPos, sizeof(netVal));
	packet.readPos += sizeof(netVal);
	data = static_cast<int16_t>(ntohs(netVal));
	return packet;
}

template <>
inline Packet& operator>>(Packet& packet, uint16_t& data)
{
	// reading out of bounds
	if (packet.readPos + sizeof(uint16_t) > packet.writePos)
	{
		return packet;
	}

	uint16_t netVal;
	std::memcpy(&netVal, packet.body + packet.readPos, sizeof(netVal));
	packet.readPos += sizeof(netVal);
	data = static_cast<uint16_t>(ntohs(netVal));
	return packet;
}

#pragma endregion

#pragma region INT32_T SPECIALIZATION
template <>
inline Packet& operator<< (Packet& packet, const int32_t& data)
{
	uint32_t netVal = htonl(static_cast<uint32_t>(data));
	std::memcpy(packet.body + packet.writePos, &netVal, sizeof(netVal));
	packet.writePos += sizeof(netVal);
	return packet;
}

template <>
inline Packet& operator<< (Packet& packet, const uint32_t& data)
{
	uint32_t netVal = htonl(data);
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
	data =static_cast<int32_t>(ntohl(netVal));
	return packet;
}

template <>
inline Packet& operator>>(Packet& packet, uint32_t& data)
{
	// reading out of bounds
	if (packet.readPos + sizeof(uint32_t) > packet.writePos)
	{
		return packet;
	}

	uint32_t netVal;
	std::memcpy(&netVal, packet.body + packet.readPos, sizeof(netVal));
	packet.readPos += sizeof(netVal);
	data = /*static_cast<uint32_t>*/(ntohl(netVal));
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

#pragma region INT64_T SPECIALIZATION
template <>
inline Packet& operator<< (Packet& packet, const int64_t& data)
{
	int64_t netVal = htonll(static_cast<uint64_t>(data));
	std::memcpy(packet.body + packet.writePos, &netVal, sizeof(netVal));
	packet.writePos += sizeof(netVal);
	return packet;
}

template <>
inline Packet& operator<< (Packet& packet, const uint64_t& data)
{
	uint64_t netVal = htonll(data);
	std::memcpy(packet.body + packet.writePos, &netVal, sizeof(netVal));
	packet.writePos += sizeof(netVal);
	return packet;
}


template <>
inline Packet& operator>>(Packet& packet, int64_t& data)
{
	// reading out of bounds
	if (packet.readPos + sizeof(int64_t) > packet.writePos)
	{
		return packet;
	}

	uint64_t netVal;
	std::memcpy(&netVal, packet.body + packet.readPos, sizeof(netVal));
	packet.readPos += sizeof(netVal);
	data = static_cast<int64_t>(ntohll(netVal));
	return packet;
}

template <>
inline Packet& operator>>(Packet& packet, uint64_t& data)
{
	// reading out of bounds
	if (packet.readPos + sizeof(uint64_t) > packet.writePos)
	{
		return packet;
	}

	uint64_t netVal;
	std::memcpy(&netVal, packet.body + packet.readPos, sizeof(netVal));
	packet.readPos += sizeof(netVal);
	data = /*static_cast<int64_t>*/(ntohll(netVal));
	return packet;
}



#pragma endregion

#pragma region STRING SPECIALIZATION
template <>
inline Packet &operator<< (Packet &packet, const std::string &data)
{
	// Truncate to 20 characters if necessary
	std::string truncatedData = data.substr(0, 20);  // Ensure max 20 chars

	// Copy the data into the packet
	std::memcpy(packet.body + packet.writePos, truncatedData.c_str(), truncatedData.length());

	// Update the write position
	packet.writePos += 20;  // Always write exactly 20 bytes, even if padded with nulls

	return packet;
}


template <>
inline Packet& operator>>(Packet& packet, std::string& data)
{
	//  i dont want to deal with string for now
	// reading out of bounds
	if (packet.readPos + 19 > packet.writePos)
	{
		return packet;
	}

	// Create a buffer to store up to 20 characters
	char buffer[20];  // 19 chars + 1 for null-terminator

	// Copy the data into the buffer, ensuring we don't exceed 20 chars
	std::memcpy(buffer, packet.body + packet.readPos, 19);

	// Null-terminate the string (in case it's shorter than 20 characters)
	buffer[20] = '\0';

	// Convert buffer into a string (this truncates if more than 20 chars)
	data = std::string(buffer);

	// Update read position in the packet
	packet.readPos += 20;

	return packet;
}
#pragma endregion


#endif