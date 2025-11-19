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
    if (argc < 6) {
        printf("AsyncUdpClient.exe <localReceivingAddress> <localReceivingPort> unicast/multicast <remoteSenderAddress> <remoteSenderPort> <localSendingInterfaceAddress>\n");
        printf("imagine you are a client at your local ip 192.169.1.58 and your server was in 192.169.1.144\n");
        printf("e.g.: `AsyncUdpClient.exe 192.168.1.58 1035 unicast 192.168.1.58 1035`\n");
        printf(" send multicast commands like this:\n");
        printf("e.g.: `AsyncUdpClient.exe 192.168.1.58 1034 multicast 224.0.0.1 1035 192.168.1.58`\n");
        return 1;
    }


    const char* localReceivingAddress = DEFAULT_SERVER_NAME;
    localReceivingAddress = argv[1];

    int localReceivingPort = DEFAULT_SERVER_PORT;
    localReceivingPort= atoi(argv[2]); // 0 if error, which is an invalid destinationPort

    BroadcastType receiverBroadcastType = BroadcastType::eUnicast;
    receiverBroadcastType = case_insensitive_equals(std::string(argv[3]), "unicast") ? BroadcastType::eUnicast : BroadcastType::eMulticast;

    const char* remoteSenderAddress = DEFAULT_SERVER_NAME;
    remoteSenderAddress = argv[4];

    int remoteSenderPort = DEFAULT_SERVER_PORT;
    remoteSenderPort= atoi(argv[5]); // 0 if error, which is an invalid destinationPort

    const char* localSendingInterfaceAddress = DEFAULT_INTERFACE_NAME;
    if (argc > 6)
        localSendingInterfaceAddress = argv[6];


    if (!initializeInvisCom()) {
        return 1;
    }

	invisCom::SocketInfo senderSocketInfo;
    if (receiverBroadcastType == BroadcastType::eUnicast) {
        printf("client sending unicast commands to \ndestination\t'%s:%d'\n", remoteSenderAddress, remoteSenderPort);
        if (!unicastSenderUDPsocket(senderSocketInfo, remoteSenderAddress, remoteSenderPort))
            return 1;
    }
    else {
        printf("client sending multicast commands to\ndestination\t'%s:%d'\noutput interface\t'%s'\n", remoteSenderAddress, remoteSenderPort, localSendingInterfaceAddress);
        if (!multicastSenderUDPsocket(senderSocketInfo, remoteSenderAddress, remoteSenderPort, localSendingInterfaceAddress))
            return 1;
    }



    invisCom::SocketInfo receiverSocketInfo;
	printf("client receiving responses at \ndestination\t'%s:%d'\n", localReceivingAddress, localReceivingPort);
    if (!unicastReceiverUDPsocket(receiverSocketInfo, localReceivingAddress, localReceivingPort)) {
		return 1;
    }


    std::atomic<bool> done = false;
    std::atomic<float> value = 2;
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
        sleepThisThreadFor(0.1f);
    }

    // block until coms thread done
    clientFuture.get();

    destroyInvisCom();

    return 0;
}

