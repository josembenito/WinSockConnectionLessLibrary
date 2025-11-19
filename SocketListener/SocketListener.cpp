// Listener.cpp : uses SocketCommunications to create a socket listener
//

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>

#include <SocketCommunications.h>

using namespace invisUtils;
using namespace invisCom;

int main(int argc, char* argv[])
{
    // listener 224.0.0.1 1034 192.168.1.143
    if (argc < 4) {
        printf("listener.exe unicast/multicast <destinationAddress> <destinationPort> <localInterfaceIP>\n");
        printf("for multicast destination should be  IP (224-239)\n");
        printf("e.g.: `listener.exe multicast 224.0.0.1 1034 192.168.1.144`\n");
        printf("for unicast destination should be unicast local IP\n:");
        printf("e.g.: `listener.exe unicast 192.168.1.144 1034 `\n");
        printf("for unicast use wild card to listen in all interfaces:%s\n", DEFAULT_INTERFACE_NAME);
        printf("e.g.: `listener.exe unicast 0.0.0.0 1034`\n");
        return 1;
    }

    if (!case_insensitive_equals(std::string(argv[1]), "multicast") && !case_insensitive_equals(std::string(argv[1]), "unicast"))
    {
        fprintf(stderr, "error parameter 1: should be either 'multicast' or 'unicast'");
        return 1;
    }

    BroadcastType broadcastType = BroadcastType::eUnicast;
	broadcastType = case_insensitive_equals(std::string(argv[1]) , "unicast") ? BroadcastType::eUnicast : BroadcastType::eMulticast;

    const char* destinationAddress = DEFAULT_SERVER_NAME;
	destinationAddress = argv[2];

    int destinationPort = DEFAULT_SERVER_PORT;
	destinationPort = atoi(argv[3]); // 0 if error, which is an invalid destinationPort

    const char* interfaceAddress = DEFAULT_INTERFACE_NAME;
    if (argc > 4)
        interfaceAddress = argv[4];

    if (!initializeInvisCom()) {
        return 1;
    }

    do
    {

		invisCom:SocketInfo socketInfo;

        if (broadcastType == BroadcastType::eUnicast) {
			printf("unicast listener for packets with\ndestination\t'%s:%d'\n", destinationAddress, destinationPort);
			if (!unicastReceiverUDPsocket(socketInfo, destinationAddress, destinationPort))
				break;
        }
        else {
			printf("multicast listener for packets with\ndestination\t'%s:%d'\ninput interface\t'%s'\n", destinationAddress, destinationPort, interfaceAddress);
            if (!multicastReceiverUDPsocket(socketInfo, destinationAddress, destinationPort, interfaceAddress))
				break;
        }
		printf("listening ... \n");
        do
        {
            std::vector<char>  buffer;
            buffer.resize(DEFAULT_PACKAGE_SIZE);

            sockaddr_in outSenderAddress;
            int receivedSize = 0;
            if (receiveFromSelect(socketInfo, buffer, outSenderAddress)) {
                std::string outHexa;
                hexOutputBuffer(outHexa, buffer, buffer.size());
                const size_t maxOutputStrSize = 80;
                if (outHexa.length() > maxOutputStrSize) {
                    outHexa = outHexa.substr(0, maxOutputStrSize);
                    outHexa = outHexa + " ... ";
                }
                printf("hexa:[%s] from %s:%d\n", outHexa.c_str(), inet_ntoa(outSenderAddress.sin_addr), ntohs(outSenderAddress.sin_port));
				printf("listening ... \n");
            }
        } while (true);

        closesocket(socketInfo.socket);

    } while (FALSE);


    destroyInvisCom();
    return 0;
}
