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
        printf("AsyncUdpServer.exe <localServerAddress> <localServerPort> <remoteClientAddress> <remoteClientPort> <\n");
        printf("e.g.: `AsyncUdpServer.exe 192.168.1.144 1034 192.168.1.58 1035`\n");
        printf(" use wild card to listen in all interfaces:%s\n", DEFAULT_INTERFACE_NAME);
        printf("e.g.: `AsyncUdpServer.exe 0.0.0.0 1034 192.168.1.58 1035`\n");
        return 1;
    }

    BroadcastType broadcastType = BroadcastType::eUnicast;

    const char* localServerAddress = DEFAULT_SERVER_NAME;
    localServerAddress = argv[1];

    int localServerPort = DEFAULT_SERVER_PORT;
    localServerPort= atoi(argv[2]); // 0 if error, which is an invalid destinationPort

    const char* remoteClientAddress = DEFAULT_SERVER_NAME;
    remoteClientAddress = argv[3];

    int remoteClientPort = DEFAULT_SERVER_PORT;
    remoteClientPort= atoi(argv[4]); // 0 if error, which is an invalid destinationPort


    if (!initializeInvisCom()) {
        return 1;
    }

	printf("server receiving commands at \ndestination\t'%s:%d'\n", localServerAddress, localServerPort);
	SenderSocketInfo receiverSocketInfo;
    if (!unicastReceiverUDPsocket(receiverSocketInfo, localServerAddress, localServerPort)) {
        return 1;
    }

	printf("server sending responses at \ndestination\t'%s:%d'\n", localServerAddress, localServerPort);
    SenderSocketInfo senderSocketInfo;
	if (!unicastSenderUDPsocket(senderSocketInfo, remoteClientAddress, remoteClientPort)) {
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

