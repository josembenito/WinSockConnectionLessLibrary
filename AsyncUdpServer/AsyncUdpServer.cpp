// AsyncUdpServer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <future>
#include <SocketCommunications.h>
#include <istream>

using namespace invisUtils;
using namespace invisCom;

int main(int argc, char* argv[])
{
    // listener 224.0.0.1 1034 192.168.1.143
    if (argc < 5) {
        printf("AsyncUdpServer.exe unicast/multicast <localReceiverAddress> <localReceiverPort> <localReceiverInterfaceIp> <remoteSenderAddress> <remoteSenderPort> \n");
        printf("imagine you are a server at your local ip was 192.169.1.144 and your client was in 192.169.1.58\n");
        printf("e.g.: `AsyncUdpServer.exe unicast 192.168.1.144 1034 0.0.0.0 192.168.1.58 1035`\n");
        printf(" use wild card to listen in all interfaces:%s\n", DEFAULT_INTERFACE_NAME);
        printf("e.g.: `AsyncUdpServer.exe unicast 0.0.0.0 1034 0.0.0.0 192.168.1.58 1035`\n");
        printf(" receive multicast commands like this\n");
        printf("e.g.: `AsyncUdpServer.exe multicast 224.0.0.1 1034 192.168.1.144 192.168.1.58 1035`\n");
        return 1;
    }


    BroadcastType receiverBroadcastType = BroadcastType::eUnicast;
    receiverBroadcastType = case_insensitive_equals(std::string(argv[1]), "unicast") ? BroadcastType::eUnicast : BroadcastType::eMulticast;

    const char* localReceiverAddress = DEFAULT_SERVER_NAME;
    localReceiverAddress = argv[2];

    int localReceiverPort = DEFAULT_SERVER_PORT;
    localReceiverPort= atoi(argv[3]); // 0 if error, which is an invalid destinationPort

    const char* localReceiverInterfaceAddress = DEFAULT_INTERFACE_NAME;
	localReceiverInterfaceAddress = argv[4];

    const char* remoteSenderAddress = DEFAULT_SERVER_NAME;
    remoteSenderAddress = argv[5];

    int remoteSenderPort = DEFAULT_SERVER_PORT;
    remoteSenderPort= atoi(argv[6]); // 0 if error, which is an invalid destinationPort


    if (!initializeInvisCom()) {
        return 1;
    }

	SocketInfo receiverSocketInfo;
    if (receiverBroadcastType == BroadcastType::eUnicast) {
        printf("server receiving unicast commands at\ndestination\t'%s:%d'\n", localReceiverAddress, localReceiverPort);
		if (!unicastReceiverUDPsocket(receiverSocketInfo, localReceiverAddress, localReceiverPort)) {
			return 1;
		}
    }
    else {
        printf("server receiving multicast commands at\ndestination\t'%s:%d'\ninput interface\t'%s'\n", localReceiverAddress, localReceiverPort, localReceiverInterfaceAddress);
        if (!multicastReceiverUDPsocket(receiverSocketInfo, localReceiverAddress, localReceiverPort, localReceiverInterfaceAddress)) {
            return 1;
        }
    }

	printf("server sending responses to \ndestination\t'%s:%d'\n", remoteSenderAddress, remoteSenderPort);
    SocketInfo senderSocketInfo;
	if (!unicastSenderUDPsocket(senderSocketInfo, remoteSenderAddress, remoteSenderPort)) {
		return 1;
    }


    std::atomic<bool> done = false;
    std::atomic<float> val = 0;

    auto handleInput = [&val](const std::vector<char> buffer) 
    {
        std::string command(buffer.begin(), buffer.end());
        float receivedVal = std::stof(command);

        // pretend long compute
        //sleepThisThreadFor(1);
        val = sqrt(receivedVal);

        std::string ret = std::to_string(val);
        return ret;
    };


	// coms thread
    auto serverFuture = std::async(std::launch::async, [&done, receiverSocketInfo, senderSocketInfo, &val, &handleInput]() {

        while (!done) {

            // receive through receive socket
			std::vector<char>  buffer;
            buffer.resize(DEFAULT_PACKAGE_SIZE);

            sockaddr_in outSenderAddress;
            int receivedSize = 0;
			//fprintf(stdout, "receivefrom\n");
            if (receiveFromSelect(receiverSocketInfo, buffer, outSenderAddress)) {

                const std::string& response = handleInput(buffer);
				
                std::vector<char>  retBuffer;
                retBuffer.assign(response.begin(), response.end());

                sendToWithSelect(senderSocketInfo, retBuffer);
            }
        }

        closesocket(receiverSocketInfo.socket);
        closesocket(senderSocketInfo.socket);
	});

    // render thread
    while (!done)
    {
        done = isKeyPressed(VK_ESCAPE);
        printf("\rval:%0.5f done:%s", val.load(), done ? "true":"false");
		sleepThisThreadFor(0.1f);
    }
   
    // block until coms thread done
    serverFuture.get();

    destroyInvisCom();

    return 0;
}

