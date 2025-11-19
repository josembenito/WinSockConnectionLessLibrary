#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#ifdef _WIN32
#include <Winsock2.h> // before Windows.h, else Winsock 1 conflict
#include <Ws2tcpip.h> // needed for ip_mreq definition for multicast
#include <Windows.h>
#include <vector>
#include <string>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> // for sleep()
#endif

namespace invisCom {

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_BROADCAST_NAME     "unicast"
#define DEFAULT_SERVER_NAME     "localhost"
#define DEFAULT_SERVER_PORT		0  // invalid port
#define DEFAULT_INTERFACE_NAME     "0.0.0.0"
	constexpr size_t DEFAULT_PACKAGE_SIZE = 64 * 1024;

#define ResizeSocketBuffers

#ifdef ResizeSocketBuffers
	constexpr size_t DEFAULT_BUFFER_RESIZE_FACTOR = 10;
#endif

	enum class BroadcastType { eUnicast, eMulticast };

	struct SocketInfo
	{
#ifdef _WIN32
		SOCKET socket = -1;
#else
		int socket = -1;
#endif
		sockaddr_in toaddr;
		unsigned maxMessageSize = 0;
	};
	bool initializeInvisCom();
	void destroyInvisCom();

	bool isMulticastIP(unsigned int ip);
	bool unicastSenderUDPsocket(SocketInfo& socketInfo, const char* destinationAddressStr, int port);
	bool multicastSenderUDPsocket(SocketInfo& socketInfo, const char* destinationAddressStr, int port, const char* localInterfaceAddressStr);

	bool unicastReceiverUDPsocket(SocketInfo& socketInfo, const char* destinationAddressStr, int port);
	bool multicastReceiverUDPsocket(SocketInfo& socketInfo, const char* groupAddressStr, int port, const char* localInterfaceAddressStr);

	bool sendTo(const SocketInfo& socketInfo, const std::vector<char>& data);
	int sendToWithSelect(const SocketInfo& socketInfo, const std::vector<char>& data, float timeoutSeconds = 1e-4f);

	// output buffer may be resized to only hold valid bytes
	bool receiveFrom(const SocketInfo& socketInfo, std::vector<char>& buffer, sockaddr_in& outSenderAddr);
	bool receiveFromSelect(const SocketInfo& socketInfo, std::vector<char>& buffer, sockaddr_in& senderaddr, float timeout =  1e-4f);

}

namespace invisUtils {

    // utils: TODO substitute or find a proper place
	bool case_insensitive_equals(std::string const& a, std::string const& b);
	void hexOutputBuffer(std::string& out, const std::vector<char>& buffer, size_t numBytes);

	void sleepThisThreadFor(float seconds);
	bool isKeyPressed(unsigned char Key);
}
