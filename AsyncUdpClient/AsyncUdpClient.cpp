// AsyncUdpClient.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <future>
#include <istream>
#include <SocketCommunications.h>


using namespace invisUtils;
using namespace invisCom;

int main(int argc, char* argv[])
{
    // listener 224.0.0.1 1034 192.168.1.143
    if (argc < 5) {
        printf("AsyncUdpClient.exe <localClientAddress> <localClientPort> <remoteServerAddress> <remoteServerPort> <\n");
        printf("e.g.: `AsyncUdpClient.exe 192.168.1.144 1034 192.168.1.58 1035`\n");
        printf(" use wild card to listen in all interfaces:%s\n", DEFAULT_INTERFACE_NAME);
        printf("e.g.: `AsyncUdpClient.exe 0.0.0.0 1034 192.168.1.58 1035`\n");
        return 1;
    }

    BroadcastType broadcastType = BroadcastType::eUnicast;

    const char* localClientAddress = DEFAULT_SERVER_NAME;
    localClientAddress = argv[1];

    int localClientPort = DEFAULT_SERVER_PORT;
    localClientPort= atoi(argv[2]); // 0 if error, which is an invalid destinationPort

    const char* remoteServerAddress = DEFAULT_SERVER_NAME;
    remoteServerAddress = argv[3];

    int remoteServerPort = DEFAULT_SERVER_PORT;
    remoteServerPort= atoi(argv[4]); // 0 if error, which is an invalid destinationPort


    if (!initializeInvisCom()) {
        return 1;
    }


	printf("client sending commands to \ndestination\t'%s:%d'\n", remoteServerAddress, localClientPort);
	SenderSocketInfo senderSocketInfo;
	if (!unicastSenderUDPsocket(senderSocketInfo, remoteServerAddress, remoteServerPort)) {
        return 1;
    }
    SenderSocketInfo receiverSocketInfo;
	printf("client receiving responses at \ndestination\t'%s:%d'\n", localClientAddress, localClientPort);
    if (!unicastReceiverUDPsocket(receiverSocketInfo, localClientAddress, localClientPort)) {
		return 1;
    }


    std::atomic<bool> done = false;
    std::atomic<float> value = 33;
    std::atomic<float> response = 0;

    auto handleInput = [](const std::vector<char> buffer)
    {
        std::string command(buffer.begin(), buffer.end());
        float ret = std::stof(command);

        return ret;
    };


    // coms thread
    auto clientFuture = std::async(std::launch::async, [&done, &value, &response, handleInput, senderSocketInfo, receiverSocketInfo]() {

        // create receive socket
        // create send socket
        while (!done) {

            std::string message = std::to_string(value);

			std::vector<char> buffer;
            buffer.assign(message.begin(), message.end());

            if (sendToWithSelect(senderSocketInfo, buffer)) {

                // receive through receive socket
                std::vector<char>  buffer;
                buffer.resize(DEFAULT_PACKAGE_SIZE);

                sockaddr_in outSenderAddress;
                int receivedSize = 0;
                if (receiveFromSelect(receiverSocketInfo, buffer, outSenderAddress)) {

                    response = handleInput(buffer);
                }
            }
			sleepThisThreadFor(1);
        }

        closesocket(senderSocketInfo.socket);
        closesocket(receiverSocketInfo.socket);

	});

    // main thread
    while (!done)
    {
        bool done = isKeyPressed(VK_ESCAPE);
        printf("\rsending val:%0.5f receiving:%0.5f", value.load(), response.load());
    }
    
    clientFuture.get();

    destroyInvisCom();
}

