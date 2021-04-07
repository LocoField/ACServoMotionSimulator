#include "ACServoMotionXPlane11.h"

#include <stdio.h>
#include <utility>
#include <WS2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

ACServoMotionXPlane11::ACServoMotionXPlane11()
{
	sock = INVALID_SOCKET;

	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
}

char* ACServoMotionXPlane11::getMotionName()
{
	return "X-Plane 11";
}

bool ACServoMotionXPlane11::start()
{
	const int sockaddrSize = sizeof(struct sockaddr_in);
	sockaddr_in serverAddr;

	// socket setting: ip address and port
	memset(&serverAddr, 0, sockaddrSize);

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(49000);

	// socket create
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == INVALID_SOCKET)
	{
		printf("socket creating failed.\n");
		return false;
	}

	// socket option
	DWORD timeout = 2000;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

	if (bind(sock, (sockaddr*)&serverAddr, sockaddrSize) == SOCKET_ERROR)
	{
		printf("socket binding failed.\n");
		return false;
	}

	return true;
}

void ACServoMotionXPlane11::stop()
{
	closesocket(sock);
	sock = INVALID_SOCKET;
}

template <class T>
inline void ConvertEndianness(T& in)
{
	char* const p = reinterpret_cast<char*>(&in);
	for (size_t i = 0; i < sizeof(T) / 2; i++)
		std::swap(p[i], p[(sizeof(T) - 1) - i]);
}

bool ACServoMotionXPlane11::process(void* arg)
{
	char buffer[1000];
	Data data;

	int sockaddrSize = sizeof(struct sockaddr_in);
	sockaddr_in fromAddr;

	int received = recvfrom(sock, (char*)&buffer, sizeof(buffer), 0, (sockaddr*)&fromAddr, &sockaddrSize);
	if (received <= 0)
	{
		printf("recvfrom error: %d\n", WSAGetLastError());
		return false;
	}

	memcpy(&data, buffer, sizeof(data));

	if (memcmp(data.header, "DATA", sizeof(data.header)) != 0)
	{
		printf("data header corrupted.\n");
		return false;
	}

	auto angleFilter = [](float& angle, float range)
	{
		angle = min(angle, range);
		angle = max(angle, -range);
	};

	float roll = data.values[1];
	float pitch = data.values[0];

	angleFilter(roll, 15);
	angleFilter(pitch, 15);

	angle_.x = roll;
	angle_.y = pitch;

	return true;
}
