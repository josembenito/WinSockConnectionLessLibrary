// Sender.cpp : uses SocketCommunications to create a socket sender
//

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include <SocketCommunications.h>

using namespace invisUtils;
using namespace invisCom;

int main(int argc, char* argv[])
{
	int tryOuts = 10;
    const char* message = "Hello world!";

    if (argc < 4) {
        printf("syntax:\nsender unicast/multicast <destinationAddress> <destinationPort> <localInterfaceIP>\n");
        printf("for multicast -destination should be IP in range (224-239)-:\n");
        printf("e.g.: `sender.exe multicast 224.0.0.1 1034 192.168.1.143`\n");
        printf("for multicast omit third parameter to use wildcard (uses default interface):%s\n", DEFAULT_INTERFACE_NAME);
        printf("e.g.: `sender.exe multicast 224.0.0.1 1034` \n");
        printf("for unicast destination should be unicast IP, interface will not be used:`\n");
        printf("e.g.: `sender.exe unicast 10.0.0.102 1034`)\n");
        printf("description: this process will send the message [%s] %d times through sockets to its destination\n", message, tryOuts);
        return 1;
    }
    if (!case_insensitive_equals(std::string(argv[1]), "multicast") && case_insensitive_equals(std::string(argv[1]), "unicast"))
    {
        fprintf(stderr, "error parameter 1: should be either 'multicast' or 'unicast'");
        return 1;
    }

    BroadcastType broadcastType = BroadcastType::eUnicast;
    broadcastType = case_insensitive_equals(std::string(argv[1]), "unicast") ? BroadcastType::eUnicast : BroadcastType::eMulticast;

    const char* destinationAddress = DEFAULT_SERVER_NAME;
    destinationAddress = argv[2];

    int destinationPort = DEFAULT_SERVER_PORT;
    destinationPort = atoi(argv[3]); // 0 if error, which is an invalid destinationPort

	const char* interfaceAddress = DEFAULT_INTERFACE_NAME;
	const char* messageStr = message;
    if (argc > 4) {
        if (broadcastType == BroadcastType::eMulticast) {
            interfaceAddress = argv[4];
        }
        else {
            messageStr = argv[4];
        }
    }
    
    if (!initializeInvisCom()) {
        return 1;
    }
 
    std::vector<char> buffer;
    buffer.assign(messageStr, messageStr + strlen(messageStr));

    do
    {
		SenderSocketInfo socketInfo;
        if (broadcastType == BroadcastType::eUnicast) {
            if (!unicastSenderUDPsocket(socketInfo, destinationAddress, destinationPort))
                break;
        }
        else {
            if (!multicastSenderUDPsocket(socketInfo, destinationAddress, destinationPort, interfaceAddress))
                break;
        }

        do {
            printf("sending: [%s] to %s:%d\n", messageStr, inet_ntoa(socketInfo.toaddr.sin_addr), ntohs(socketInfo.toaddr.sin_port) );
            sendTo(socketInfo, buffer);

#ifdef _WIN32
			const int seconds = 1;
			Sleep(seconds * 1000); // Windows Sleep is milliseconds
#else
			sleep(delay_secs); // Unix sleep is seconds
#endif
        } while (tryOuts-- > 0);

		closesocket(socketInfo.socket);

    } while (false);


    destroyInvisCom();
    return 0;
}


