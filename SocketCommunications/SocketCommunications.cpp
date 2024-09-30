// SocketCommunications.cpp : Defines the functions for the static library.
//
#include "SocketCommunications.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#ifdef _WIN32
#include <WinBase.h>
void printError(const char* prefix)
{
    wchar_t* s = NULL;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, WSAGetLastError(), MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), (LPWSTR)&s, 0, NULL);
    fprintf(stderr, "%s:%S\n", prefix, s);
    LocalFree(s);
}
#endif

#ifndef _WIN32
	using SOCKET = int;
#endif

namespace invisCom {
	const float ToMicroSeconds = 1e6f;

	bool initializeInvisCom()
	{
#ifdef _WIN32
		WSADATA wsaData;
		if (WSAStartup(0x0101, &wsaData)) {
			perror("WSAStartup");
			return false;
		}
		return true;
#endif
	}
	void destroyInvisCom()
	{
#ifdef _WIN32
		WSACleanup();
#endif
	}

	timeval timevalFromSeconds(float seconds)
	{
		float integerPart = 0, fractionalPart = 0;
		integerPart = modff(seconds, &fractionalPart);

		timeval ret;
		ret.tv_sec = static_cast<long>(integerPart);
		ret.tv_usec = static_cast<long> (fractionalPart * ToMicroSeconds);

		return ret;
	}

	bool isMulticastIP(unsigned int ip) {
		char* ip_str = (char*)&ip;

		int i = ip_str[0] & 0xFF;
		if (i >= 224 && i <= 239) {
			return true;
		}

		return false;
	}

	bool createSocket(SOCKET& sd, bool TCP)
	{
		bool ret = false;
		sd = socket(AF_INET, TCP ? SOCK_STREAM : SOCK_DGRAM, 0);
		if (sd < 0) {
			printError("(create)socket");
			return ret;
		}
		return ret = true;
	}

	// warning: inverse logic here, sockets are blocking by default
	// value : true     means set socket as non blocking
	// value : false    means set socket blocking

	bool setSocketNonBlockingMode(SOCKET sd, bool value)
	{
		bool ret = false;
		u_long isNonBlocking = value ? 1 : 0;
		int result = ioctlsocket(sd, FIONBIO, &isNonBlocking);
		if (result == SOCKET_ERROR)
		{
			printError("(setSocketNonBlockingMode)ioctlsocket");
			return ret;
		}

		return ret = true;
	}

	//    Using SO_REUSEADDR 
	//
	//        - for unicast TCP, REUSEADDR avoids TIME_WAIT at connect after closing
	//        - for unicast UDP it is useless and does harm, avoid (see below)
	//        - multicast UDP allows many sockets to join the same group(on the same machine)
	//
	//        Zocket used to set it to true by default unicast and multicast
	//        igSync Slave sets it to true in multicast
	//        MulticastClient(Listener) sets it to true by default
	//
	//        Note: SO_REUSEADDR does not interfere with other machine's sockets
	//
	//        however :
	//        - in unicast if two sockets bind to the same 'ip:port' with REUSEADDR the result is INDETERMINATE
	//          consider that if both processes got all the packets just fine, it would amount to multicast
	//        the second process that binds hijacks the packets but as explained in MSDN the result is indeterminate*
	//         * indeterminate means might work sometimes and not others, today but not tomorrow, in this Windows version but not in another, might work in my house but not at the office, etc.
	//
	//        we have observed this behaviour with gcc - old version - and on this program too
	//        if we start this program that listens to host after gcc is run, packets are not received
	//        if we start unreal that listens to host after gcc is run, packets are not received by unreal, same if we start gcc after unreal
	//          curiously enough invis works well with gcc, but problems with this issue were known
	//          the explanation might lie in the fact that gcc and invis use the same process (same PID) or use the same user for launch
	//          modern Windows does not allow REUSEADDR by default but has a special case to allow hijacking for processes that are run with the same user. (see documentation)
	//         zocket sets it to true by default - this is wrong for unicast - and bind should fail if other process in the machine (such as gcc) uses it
	//      Using SO_EXCLUSIVEADDRUSE
	//           actually since any second process can set SO_REUSEADDR to true and hijack packets, it is recommended to use SO_EXCUSIVEADDRUSE before a call to bind to make sure subsequent binds fail. It should also fail if the endpoint ip:port is in use by other socket. Also for this reason we consider old gcc and any program that uses REUSEADDR in unicast as a bad neighbor as its use will hijack packets
	//     Sources:
	//  https://learn.microsoft.com/en-us/windows/win32/winsock/using-so-reuseaddr-and-so-exclusiveaddruse?redirectedfrom=MSDN
	//  https://stackoverflow.com/questions/12540449/so-reuseaddr-with-udp-sockets-on-linux-is-it-necessary

	bool setSocketExclusiveUse(SOCKET sd, bool value)
	{
		bool ret = false;
		u_int yes = value ? 1 : 0;
		if (setsockopt(sd, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (char*)&yes, sizeof(yes)) == SOCKET_ERROR) {
			printError("(setsockopt)SO_EXCLUSIVEADDRUSE");
			return ret;
		}
		return ret = true;
	}

	// Please do not use in unicast or else your packets can be hijacked (see above)
	// in multicast context sets whether several sockets are allowed to share the same listening ip:port
	bool setReceiverSocketReuse(SOCKET sd, bool value)
	{
		bool ret = false;
		u_int yes = value ? 1 : 0;
		if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char*)&yes, sizeof(yes)) == SOCKET_ERROR) {
			printError("(setsockopt)SO_REUSEADDR");
			return ret;
		}
		return ret = true;
	}

	// sets whether multicast outbound packets will be looped back to sender machine 
	// set to false if you do not want other local process to get your outbound multicast packets
	// default = true
	bool setMulticastSenderLoopBack(SOCKET sd, bool value)
	{
		bool ret = false;
		u_int yes = value ? 1 : 0;
		if (setsockopt(sd, IPPROTO_IP, IP_MULTICAST_LOOP, (char*)&yes, sizeof(yes)) == SOCKET_ERROR) {
			printError("(setsockopt)IP_MULTICAST_LOOP");
			return ret;
		}
		return ret = true;
	}

	// sets time to live for outbound packets i.e. how many subnetworks outbound packets are allowed to jump
	// default = 1 
	bool setMulticastSenderTimeToLive(SOCKET sd, int value)
	{
		bool ret = false;
		if (setsockopt(sd, IPPROTO_IP, IP_MULTICAST_TTL, (char*)&value, sizeof(value)) == SOCKET_ERROR) {
			printError("(setsockopt)IP_MULTICAST_LOOP");
			return ret;
		}
		return ret = true;
	}

	// sets the interface (the local ip, if there are several) that outbound multicast packets will use
	// imagine a sender machine with two or more network cards or virtual interfaces, this specifies which one is used to send packets
	bool setMulticastSenderInterface(SOCKET sd, const char* interfaceAddressStr) {

		bool ret = false;
		u_long interfaceAddress = htonl(INADDR_ANY);
		if (interfaceAddressStr != nullptr) {
			interfaceAddress = inet_addr(interfaceAddressStr);
		}
		IN_ADDR imr_interface;
		imr_interface.s_addr = interfaceAddress;
		if (setsockopt(sd, IPPROTO_IP, IP_MULTICAST_IF, (const char*)&(imr_interface), sizeof(imr_interface)) == SOCKET_ERROR) {
			printError("(setsockopt)IP_MULTICAST_IF");
			return ret;
		}
		return ret = true;
	}

	bool addMulticastListenerMembershipAndInterface(SOCKET sd, const char* multicastGroupAddrStr, const char* interfaceAddressStr)
	{
		bool ret = false;
		u_long interfaceAddress = htonl(INADDR_ANY);
		if (interfaceAddressStr != nullptr) {
			interfaceAddress = inet_addr(interfaceAddressStr);
		}

		struct ip_mreq mreq;
		mreq.imr_multiaddr.s_addr = inet_addr(multicastGroupAddrStr);
		mreq.imr_interface.s_addr = interfaceAddress;
		if (setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq)) < 0) {
			printError("(setsockopt)IP_ADD_MEMBERSHIP");
			return ret;
		}
		return ret = true;
	}

	bool resolveAddress(IN_ADDR& sin_addr, const char* addressStr)
	{
		bool ret = false;

		sin_addr.s_addr = htonl(INADDR_ANY);

		// try addressStr as an IP
		if (addressStr) {
			sin_addr.s_addr = inet_addr(addressStr);
		}
		// if addressStr is not an IP, try resolve host by name
		if (sin_addr.s_addr == (unsigned long)INADDR_NONE) {
			struct hostent* hostp = nullptr;
			if (addressStr) {
				hostp = gethostbyname(addressStr);
			}
			if (hostp == nullptr) {
				// did not work, we give up
				printError("(resolveAddress)gethostbyname");
				return ret;
			}
			memcpy(&sin_addr, hostp->h_addr, sizeof(sin_addr));
		}
		return ret = true;
	}

	bool resolveAddressAndPort(sockaddr_in& toaddr, const char* addressStr, int port)
	{
		bool ret = false;

		memset(&toaddr, 0, sizeof(toaddr));
		toaddr.sin_family = AF_INET;

		if (!resolveAddress(toaddr.sin_addr, addressStr)) {
			return ret;
		}
		toaddr.sin_port = htons(port);

		//printf("resolved host: %s:%d\n", inet_ntoa(toaddr.sin_addr), ntohs(toaddr.sin_port)); // debug
		return ret = true;
	}

	bool getSocketMaxMessageSize(SOCKET sd, unsigned& outSize)
	{
		bool ret = false;
		int numBytes = sizeof(outSize);
		int result = -1;
		result = getsockopt(sd, SOL_SOCKET, SO_MAX_MSG_SIZE, (char*)&outSize, &numBytes);
		if (result == SOCKET_ERROR) {
			printError("(getsockopt)SO_MAX_MSG_SIZE");
			return ret;
		}
		return ret = true;
	}

	bool setReceiveSocketBufferSize(SOCKET sd, unsigned int bufferSize)
	{
		bool ret = false;
		int result;
		result = setsockopt(sd, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<char*>(&bufferSize), sizeof(bufferSize));
		if (result == SOCKET_ERROR) {
			printError("(setsockopt)SO_RCVBUF");
			return ret;
		}
		return ret = true;
	}

	bool setSendSocketBufferSize(SOCKET sd, unsigned int bufferSize)
	{
		bool ret = false;
		int result;
		result = setsockopt(sd, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<char*>(&bufferSize), sizeof(bufferSize));
		if (result == SOCKET_ERROR) {
			printError("(setsockopt)SO_SNDBUF");
			return ret;
		}
		return ret = true;
	}

	bool getReceiveSocketBufferSize(SOCKET sd, unsigned& size)
	{
		bool ret = false;
		int numBytes = sizeof(size);
		int result = getsockopt(sd, SOL_SOCKET, SO_RCVBUF, (char*)&size, &numBytes);
		if (result == SOCKET_ERROR) {
			printError("(getsockopt)SO_RCVBUF");
			return ret;
		}
		return ret = true;
	}

	bool getSendSocketBufferSize(SOCKET sd, unsigned& size)
	{
		bool ret = false;
		int numBytes = sizeof(size);
		int result = getsockopt(sd, SOL_SOCKET, SO_SNDBUF, (char*)&size, &numBytes);
		if (result == SOCKET_ERROR) {
			printError("(getsockopt)SO_SNDBUF");
			return ret;
		}
		return ret = true;
	}


	bool unicastSenderUDPsocket(SenderSocketInfo& socketInfo, const char* destinationAddressStr, int port)
	{
		bool ret = false;
		if (!resolveAddressAndPort(socketInfo.toaddr, destinationAddressStr, port)) {
			fprintf(stderr, "(unicastSenderUDPsocket) destinationAddressStr '%s' could not be solved\n", destinationAddressStr);
			return ret;
		}
		// only allow multicast addresses
		if (isMulticastIP(socketInfo.toaddr.sin_addr.s_addr)) {
			fprintf(stderr, "(multicastSenderSocketUDP) destinationAddressStr '%s' is not unicas\n", destinationAddressStr);
			return ret;
		}

		// do while(false) allows break to exit cleanly and close socket
		do {

			if (!createSocket(socketInfo.socket, false)) {
				break;
			}

			// needed in sendToWithSelect
			if (!getSocketMaxMessageSize(socketInfo.socket, socketInfo.maxMessageSize)) {
				break;
			}
#ifdef ResizeSocketBuffers
			if (!setSendSocketBufferSize(socketInfo.socket, socketInfo.maxMessageSize * DEFAULT_BUFFER_RESIZE_FACTOR)) {
				break;
			}
#endif

			ret = true;
		} while (false);

		if (ret == false)
			closesocket(socketInfo.socket);

		return ret;
	}

	bool multicastSenderUDPsocket(SenderSocketInfo& socketInfo, const char* destinationAddressStr, int port, const char* localInterfaceAddressStr)
	{
		bool ret = false;
		if (!resolveAddressAndPort(socketInfo.toaddr, destinationAddressStr, port)) {
			fprintf(stderr, "(multicastSenderSocketUDP) destinationAddressStr '%s' could not be solved\n", destinationAddressStr);
			return ret;
		}
		// only allow multicast addresses
		if (!isMulticastIP(socketInfo.toaddr.sin_addr.s_addr)) {
			fprintf(stderr, "(multicastSenderSocketUDP) destinationAddressStr '%s' is not multicast\n", destinationAddressStr);
			return ret;
		}
		// do while(false) allows break to exit cleanly and close socket
		do
		{
			if (!createSocket(socketInfo.socket, false)) {
				break;
			}

			// needed in sendToWithSelect
			if (!getSocketMaxMessageSize(socketInfo.socket, socketInfo.maxMessageSize)) {
				break;
			}

#ifdef ResizeSocketBuffers
			if (!setSendSocketBufferSize(socketInfo.socket, socketInfo.maxMessageSize * DEFAULT_BUFFER_RESIZE_FACTOR)) {
				break;
			}
#endif

			if (!resolveAddressAndPort(socketInfo.toaddr, destinationAddressStr, port)) {
				break;
			}

			if (!setMulticastSenderInterface(socketInfo.socket, localInterfaceAddressStr)) {
				break;
			}
			ret = true;
		} while (false);

		if (ret == false)
			closesocket(socketInfo.socket);

		return ret;
	}


	bool sendTo(const SenderSocketInfo& socketInfo, const std::vector<char>& buffer)
	{
		bool ret = false;
		int rc = sendto(socketInfo.socket, &buffer.front(), static_cast<int>(buffer.size()), 0, (struct sockaddr*)&socketInfo.toaddr, sizeof(socketInfo.toaddr));
		if (rc < 0) {
			printError("(sendto)sendto");
			return ret;
		}
		return ret = true;
	}


	int sendToWithSelect(const SenderSocketInfo& socketInfo, const std::vector<char>& data, float timeout)
	{
		int res = 0;
		int retry = 10;

		// set socket in socket set
		fd_set setWriteSck;
		FD_ZERO(&setWriteSck); // reset 
		FD_SET(socketInfo.socket, &setWriteSck);// add socket to set 

		timeval writeTimeout = timevalFromSeconds(timeout);

		const char* dataBegin = &data.front();
		const char* dataEnd = &data.front() + data.size();

		while (retry > 0) {
			// check socket for writeability
			int ready = select(0, 0, &setWriteSck, 0, &writeTimeout); // can we write ?
			if (ready > 0) // yes 
			{
				assert(socketInfo.maxMessageSize != 0);
				size_t dataLength = dataEnd - dataBegin;
				dataLength = dataLength < socketInfo.maxMessageSize ? dataLength : socketInfo.maxMessageSize;
				res = sendto(socketInfo.socket, dataBegin, static_cast<int>(dataLength), 0, (struct sockaddr*)&socketInfo.toaddr, sizeof(socketInfo.toaddr));

				if (res == SOCKET_ERROR) {
					printError("(sendToWithSelect)sendto");
					WSASetLastError(0); //To reset the error code
					break;
				}
				break;
			}
			else {
				int error = 0;
				int len = sizeof(error);

				if (getsockopt(socketInfo.socket, SOL_SOCKET, SO_ERROR, (char*)&error, &len) == SOCKET_ERROR) {
					printError("(sendToWithSelect)select");
				}
				retry--;
			}
		}
		return res;
	}

	bool bindSocketTo(SOCKET sd, const sockaddr_in& toaddr)
	{
		bool ret = false;
		if (bind(sd, (struct sockaddr*)&toaddr, sizeof(toaddr)) < 0) {
			printError("(bindSocketTo)bind");
			return ret;
		}
		return ret = true;
	}


	// for UDP
	// there is only one receiving socket and OS queues messages by order of arrival and handles every message as an indivisible block. 
	// if you request recvfrom will block if there are no datagrams. 
	// If there are datagrams will return the first datagram in the queue and discard the bytes that do not fit in the buffer. 
	// one strategy is always use a large enough buffer (you should know the maximum datagram size in your app) and specify the maximum read size with each recvfrom call.
	// ref: https://rg3.name/201504241907.html

	bool receiveFrom(const SenderSocketInfo& socketInfo, std::vector<char>& buffer, sockaddr_in& senderaddr)
	{
		bool ret = false;
		int serveraddrSize = sizeof(senderaddr);
		int size = recvfrom(socketInfo.socket, &buffer.front(), static_cast<int>(buffer.size()), 0, (struct sockaddr*)&senderaddr, &serveraddrSize);
		if (size == SOCKET_ERROR) {
			printError("(receiveFrom)recvfrom");
			return ret;
		}
		buffer.resize(size);
		return ret = true;
	}


	bool receiveFromSelect(const SenderSocketInfo& socketInfo, std::vector<char>& buffer, sockaddr_in& senderaddr, float timeout)
	{
		bool ret = false;

		// set socket in socket set set
		fd_set setSck;
		FD_ZERO(&setSck); // reset
		FD_SET(socketInfo.socket, &setSck); // set socket in set

		timeval readTimeout = timevalFromSeconds(timeout);

		// check socket set for readability
		int selectResult = select(0, &setSck, 0, 0, &readTimeout);
		if (selectResult < 0) {
			printError("(receiveFromSelect)select");
			return ret;
		}
		else if (selectResult == 0) {
			//fprintf(stderr, "(receiveFromSelect)select timeout\n");
			return ret;
		}
		if (!receiveFrom(socketInfo, buffer, senderaddr)) {
			return ret;
		}
		return ret = true;
	}

	bool unicastReceiverUDPsocket(SenderSocketInfo& socketInfo, const char* destinationAddressStr, int port)
	{
		bool ret = false;

		if (!resolveAddressAndPort(socketInfo.toaddr, destinationAddressStr, port)) {
			fprintf(stderr, "(unicastReceiverUDPsocket) destinationAddressStr '%s' could not be solved\n", destinationAddressStr);
			return ret;
		}
		// only allow unicast addresses
		if (isMulticastIP(socketInfo.toaddr.sin_addr.s_addr)) {
			fprintf(stderr, "(unicastReceiverUDPsocket) destinationAddress '%s' is not unicast (but multicast)\n", destinationAddressStr);
			return ret;
		}

		do {
			if (!createSocket(socketInfo.socket, false)) {
				break;
			}
			// other processes in this machine that try to bind to this same ip:port should fail (e.g. gcc)
			// if other process has taken the ip:port it should fail
			if (!setSocketExclusiveUse(socketInfo.socket, true)) {
				break;
			}
			// needed in sendToWithSelect
			if (!getSocketMaxMessageSize(socketInfo.socket, socketInfo.maxMessageSize)) {
				break;
			}
#ifdef ResizeSocketBuffers
			if (!setReceiveSocketBufferSize(socketInfo.socket, socketInfo.maxMessageSize * DEFAULT_BUFFER_RESIZE_FACTOR)) {
				break;
			}
#endif

			if (!bindSocketTo(socketInfo.socket, socketInfo.toaddr)) {
				break;
			}

			ret = true;
		} while (false);

		if (ret == false)
			closesocket(socketInfo.socket);

		return ret;
	}

	bool multicastReceiverUDPsocket(SenderSocketInfo& socketInfo, const char* groupAddressStr, int port, const char* localInterfaceAddressStr)
	{
		bool ret = false;

		IN_ADDR groupAddress;
		if (!resolveAddress(groupAddress, groupAddressStr)) {
			return ret;
		}
		// only allow multicast addresses
		if (!isMulticastIP(groupAddress.s_addr)) {
			fprintf(stderr, "(multicastReceiverUDPsocket) groupAddress '%s' is not multicast\n", groupAddressStr);
			return ret;
		}

		do
		{
			if (!createSocket(socketInfo.socket, false)) {
				break;
			}
			// needed in sendToWithSelect
			if (!getSocketMaxMessageSize(socketInfo.socket, socketInfo.maxMessageSize)) {
				break;
			}
#ifdef ResizeSocketBuffers
			if (!setReceiveSocketBufferSize(socketInfo.socket, socketInfo.maxMessageSize * DEFAULT_BUFFER_RESIZE_FACTOR)) {
				break;
			}
#endif

			// allows other processes in this machine to listen to this same multicast 
			if (!setReceiverSocketReuse(socketInfo.socket, true)) {
				break;
			}

			// bind to: 
			// if toaddr is groupAddrsStr: we receive multicast only (only linux though)
			// if toaddr is interfaceAddressStr: we will receive both multicast and unicast sent to that interface
			// source: https://rg3.name/201504241907.html
			// 
			// WIN32: since recvmsg is not available, to filter udp packets by destination address in source consider this: 
			// https://stackoverflow.com/questions/60729676/getting-the-destination-ip-of-incoming-udp-packet-in-c 

			if (!resolveAddressAndPort(socketInfo.toaddr, localInterfaceAddressStr, port)) {
				return ret;
			}

			if (!bindSocketTo(socketInfo.socket, socketInfo.toaddr)) {
				break;
			}

			if (!addMulticastListenerMembershipAndInterface(socketInfo.socket, groupAddressStr, localInterfaceAddressStr)) {
				break;
			}
			ret = true;
		} while (false);

		if (ret == false)
			closesocket(socketInfo.socket);

		return ret;
	}
} // invisCom

namespace invisUtils {

    // utils: TODO substitute or find a proper place

	bool case_insensitive_equals(std::string const& a, std::string const& b)
	{
        if (a.length() != b.length()) {
            return false;
        }
		return std::equal(b.begin(), b.end(), a.begin(), [](unsigned char a, unsigned char b) {
			return std::tolower(a) == std::tolower(b);
		});
	}

	void hexOutputBuffer(std::string& out, const std::vector<char>& buffer, size_t numBytes)
	{
		out.resize(numBytes * 3 + 1);
		const char* pHexTable = "0123456789ABCDEF";
		int iPos = 0;

		for (int i = 0; i < numBytes; i++) {
			//assume buffer contains some binary data at this point
			const char cHex = buffer[i];
			out[iPos++] = pHexTable[(cHex >> 4) & 0x0f];
			out[iPos++] = pHexTable[cHex & 0x0f];
			out[iPos++] = ' ';
		}
		//out[iPos] = '\0';
	}


	bool isKeyPressed(unsigned char Key)
	{
		bool ret = false;
	#ifdef _WIN32
			//BYTE arr[256];
			//memset(arr, 0, sizeof(256));
			//GetKeyState(0);
			
			if (GetAsyncKeyState(Key))
			{
				//if (arr[Key] > 0) {
					ret = true;
				//}
			}
	#else
		// TODO
	#endif
		return ret;
	}

	void sleepThisThreadFor(float seconds)
	{
	#ifdef _WIN32
		Sleep(static_cast<DWORD>(seconds * 1000)); // Windows Sleep is milliseconds
	#else
		sleep(seconds); // Unix sleep is seconds
	#endif
	}


}
