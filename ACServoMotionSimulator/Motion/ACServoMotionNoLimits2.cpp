#include "ACServoMotionNoLimits2.h"

#include <stdio.h>
#include <utility>
#include <WS2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

ACServoMotionNoLimits2::ACServoMotionNoLimits2()
{
	sock = INVALID_SOCKET;
	last_rendered_frame = 0;

	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
}

char* ACServoMotionNoLimits2::getMotionName()
{
	return "No Limits 2";
}

bool ACServoMotionNoLimits2::start()
{
	const int sockaddrSize = sizeof(struct sockaddr_in);
	sockaddr_in serverAddr;

	// socket setting: ip address and port
	memset(&serverAddr, 0, sockaddrSize);

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(15151);

	if (inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr) <= 0)
	{
		printf("inet_pton() failed.\n");
		return false;
	}

	// socket create
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock <= 0)
	{
		printf("socket creating failed.\n");
		return false;
	}

	// recv timeout
	DWORD timeout = 2000;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

	// temporary setting non-blocking socket for timeout
	unsigned long modeNonBlocking = 1;
	ioctlsocket(sock, FIONBIO, &modeNonBlocking);

	int ret = ::connect(sock, (struct sockaddr*)& serverAddr, sockaddrSize);
	if (ret == 0)
	{
		// connection has succeeded immediately
		return true;
	}

	fd_set fdset;
	FD_ZERO(&fdset);
	FD_SET(sock, &fdset);

	timeval tv;
	tv.tv_sec = 2;
	tv.tv_usec = 0;
	ret = select((int)(sock + 1), NULL, &fdset, NULL, &tv);

	if (ret <= 0)
	{
		printf("socket connection failed: %d\n", WSAGetLastError());
		return false;
	}

	int length = sizeof(int);
	getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*)&ret, (socklen_t *)&length);

	if (ret != 0)
	{
		printf("socket error: %d\n", errno);

		closesocket(sock);
		return false;
	}

	modeNonBlocking = 0;
	ioctlsocket(sock, FIONBIO, &modeNonBlocking);

	return true;
}

void ACServoMotionNoLimits2::stop()
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

bool ACServoMotionNoLimits2::process(void* arg)
{
	if (sock <= 0)
	{
		return false;
	}

	char senddata[] = { 'N', 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 'L' };
	send(sock, senddata, sizeof(senddata), 0);


	char buffer[500];
	int offset = 1; // due to magic 'N'

	Message message;
	TelemetryData telemetry;

	int received = recv(sock, buffer, sizeof(buffer), 0);

	if (received <= 0 ||
		buffer[0] != 'N' ||
		buffer[received - 1] != 'L')
	{
		return false;
	}

	memcpy(&message, buffer + offset, sizeof(message));
	offset += sizeof(message) + 1; // due to magic 'L'

	ConvertEndianness(message.type_id);
	ConvertEndianness(message.request_id);

	if (message.type_id != 0x06)
	{
		return false;
	}

	memcpy(&telemetry, buffer + offset, sizeof(telemetry));
	offset += sizeof(telemetry);

	ConvertEndianness(telemetry.rendered_frame);

	if (last_rendered_frame == telemetry.rendered_frame)
	{
		return false;
	}

	last_rendered_frame = telemetry.rendered_frame;


	ConvertEndianness(telemetry.gforce_x);
	ConvertEndianness(telemetry.gforce_y);
	ConvertEndianness(telemetry.gforce_z);

	angle_.x = telemetry.gforce_x * 20;
	angle_.y = telemetry.gforce_z * 20;

	float heave = telemetry.gforce_y - 1;
	axis_.ll = axis_.lr = axis_.rl = axis_.rr = -heave;

	return true;
}
