#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include "Constants.h"

using namespace std;

SOCKET mySocket;

void closeSocket()
{
    //closes the socket and the library
    closesocket(mySocket);
    WSACleanup();
}

int getPort()
{
    //returns the port the socket is listening to
    sockaddr_in boundAddress;
    int boundAddressLength = sizeof(boundAddress);
    if (getsockname(mySocket, (struct sockaddr*)&boundAddress, &boundAddressLength) == SOCKET_ERROR)
    {
        cout << "error getting local address: " << WSAGetLastError() << '\n';
        return -1;
    }
    return ntohs(boundAddress.sin_port);
}

string getIp()
{
    //returns the local ip
    sockaddr_in boundAddress;
    int boundAddressLength = sizeof(boundAddress);
    if (getsockname(mySocket, (struct sockaddr*)&boundAddress, &boundAddressLength) == SOCKET_ERROR)
    {
        cout << "error getting local address: " << WSAGetLastError() << '\n';
        return "";
    }
    char ipAddressStr[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &boundAddress.sin_addr, ipAddressStr, sizeof(ipAddressStr)) != nullptr)
        return ipAddressStr;
    return "";
}

int getRandomPort()
{
    //get a port from a range that is generally free to use 49152 - 65535
    return 49152 + (rand() % (65535 - 49152 + 1));
}

bool initSocket(bool firstTime = false, int listenPort = -1)
{
    //initializes a socket for communication, returns true if succeeded and false if not
    if (firstTime)
    {
        //initializes library data
        WSADATA wsaData;

        //makes sure Winsock version is 2.2
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        {
            cout << "failed to initialize Winsock" << '\n';
            return false;
        }
    }

    //creates the socket and makes sure it is good
    mySocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (mySocket == INVALID_SOCKET)
    {
        cout << "error when creating the socket" << '\n';
        return false;
    }

    if (listenPort != -1)
    {
        //sets struct for bind
        struct sockaddr_in localAddress;
        localAddress.sin_family = AF_INET;
        localAddress.sin_port = htons(listenPort);
        localAddress.sin_addr.s_addr = INADDR_ANY;

        //tries to bind the socket to the port
        auto result = bind(mySocket, (SOCKADDR*)&localAddress, sizeof(localAddress));
        if (result == SOCKET_ERROR)
        {
            cout << "error binding the socket to a specific port" << WSAGetLastError() << '\n';
            return false;
        }
        return true;
    }

    //tries to bind the socket to any port
    bool enough = false;
    int attempts = 0;
    while (!enough and attempts < Max_Attempts)
    {
        //sets struct for bind
        struct sockaddr_in localAddress;
        localAddress.sin_family = AF_INET;
        int tryPort = getRandomPort();
        localAddress.sin_port = htons(tryPort);
        localAddress.sin_addr.s_addr = INADDR_ANY;

        //tries to bind the socket to the port
        auto result = bind(mySocket, (SOCKADDR*)&localAddress, sizeof(localAddress));
        if (result == SOCKET_ERROR)
            cout << "attempt " << attempts << " error binding the socket to port " << tryPort << " " << WSAGetLastError() << '\n';
        else
            enough = true;
        attempts++;
    }
}

bool sendMessage(char* message, char* receivingIp, int receivingPort)
{
    //sets the address of the receiver
    sockaddr_in destinationAddress;
    destinationAddress.sin_family = AF_INET;
    destinationAddress.sin_port = htons(receivingPort);
    inet_pton(AF_INET, receivingIp, &destinationAddress.sin_addr);

    //sends the message and handles errors
    int bytesSent = sendto(mySocket, message, strlen(message), 0, (struct sockaddr*)&destinationAddress, sizeof(destinationAddress));
    if (bytesSent == SOCKET_ERROR)
    {
        std::cerr << "error sending message: " << WSAGetLastError() << '\n';
        return false;
    }
    else
        cout << "Sent " << bytesSent << " bytes: " << message << '\n';
    return true;
}

//saves the start of a different message if read accidentaly
char receiveBuffer[Read_Block_Size];
int bytesReceived;
bool readStart = false;

bool receiveMessage(vector <char>* message)
{
    //set the address and buffer
    sockaddr_in senderAddress;
    int senderAddressSize = sizeof(senderAddress);

    //check if you read the start before
    if (readStart)
        (*message).insert((*message).end(), receiveBuffer, receiveBuffer + bytesReceived);

    //set that the start of the next message is not read
    readStart = false;

    //variables to use while receiving the message
    int portReceivedFrom;
    bool first = true;
    string ipReceivedFrom = "";

    //receive the message
    do
    {
        //set the time of delay
        timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;

        //set up the file descriptor set for select
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(mySocket, &readSet);

        //checks to see if there is data waiting to be read
        int selectResult = select(0, &readSet, nullptr, nullptr, &timeout);
        if (selectResult == SOCKET_ERROR)
        {
            //handles an error
            std::cerr << "error in select: " << WSAGetLastError() << '\n';
            return false;
        }
        else if (selectResult == 0)
        {
            //returns when there is nothing to read
            cout << "time has ended" << '\n';
            return false;
        }

        bytesReceived = recvfrom(mySocket, receiveBuffer, sizeof(receiveBuffer), 0, (struct sockaddr*)&senderAddress, &senderAddressSize);
        if (bytesReceived == SOCKET_ERROR)
        {
            //check if there is an error
            std::cerr << "error receiving message: " << WSAGetLastError() << '\n';
            return false;
        }

        char senderIpStr[INET_ADDRSTRLEN];
        if (inet_ntop(AF_INET, &senderAddress.sin_addr, senderIpStr, sizeof(senderIpStr)) == nullptr)
        {
            cout << "error getting the sender's ip address" << '\n';
            return false;
        }

        if (first)
        {
            //sets the details about the sender
            first = false;
            portReceivedFrom = ntohs(senderAddress.sin_port);
            ipReceivedFrom = senderIpStr;
        }
        else if (strcmp(senderIpStr, ipReceivedFrom.c_str()) != 0 or portReceivedFrom != ntohs(senderAddress.sin_port))
        {
            //checks if the sender has changed - new message
            readStart = true;
            break;
        }

        //adds the block received to the rest of the message
        (*message).insert((*message).end(), receiveBuffer, receiveBuffer + bytesReceived);

    } while (bytesReceived == Read_Block_Size);

    //returns that the message was read successfully
    return true;
}
