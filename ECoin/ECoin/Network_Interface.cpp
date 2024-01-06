#include <iostream>
#include <vector>
#include "H_Constants.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iomanip>

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

    //returns that the init was done successfully
    return true;
}

bool sendMessage(char* message, int len, char* receivingIp, int receivingPort)
{
    //sends a message
    //sets the address of the receiver
    sockaddr_in destinationAddress;
    destinationAddress.sin_family = AF_INET;
    destinationAddress.sin_port = htons(receivingPort);
    inet_pton(AF_INET, receivingIp, &destinationAddress.sin_addr);

    //sends the message and handles errors
    int bytesSent = sendto(mySocket, message, (size_t)len, 0, (struct sockaddr*)&destinationAddress, sizeof(destinationAddress));
    if (bytesSent == SOCKET_ERROR)
    {
        cout << "error sending message: " << WSAGetLastError() << '\n';
        return false;
    }
    else
    {
        /////////////////////////////////////////////////////////////
        cout << "Sent " << bytesSent << " to " << receivingIp << " " << unsigned (receivingPort) << " bytes: " << '\n';
        cout << hex;
        for (int a = 0; a < len; a++)
            cout << hex << setw(2) << std::setfill('0') << static_cast<unsigned>(static_cast<unsigned char>(message[a])) << ' ';
        cout << dec << '\n' << '\n';
    }

    //returns that the message was sent successfully
    return true;
}

//saves the start of a different message if read accidentaly
char receiveBuffer[Maximum_Message_Size];
int bytesReceived;
int portReceivedFrom;
string ipReceivedFrom = "";

pair <string, int> getAddress()
{
    //returns the address of the last sender
    return { ipReceivedFrom, portReceivedFrom };
}

bool receiveMessage(vector <char>* message)
{
    //receives a message
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
        cout << "error in select: " << WSAGetLastError() << '\n';
        return false;
    }
    else if (selectResult == 0)
    {
        //returns when there is nothing to read
        cout << "no message to read" << '\n';
        return false;
    }

    //set the address and buffer
    sockaddr_in senderAddress;
    int senderAddressSize = sizeof(senderAddress);

    bytesReceived = recvfrom(mySocket, receiveBuffer, sizeof(receiveBuffer), 0, (struct sockaddr*)&senderAddress, &senderAddressSize);
    if (bytesReceived == SOCKET_ERROR)
    {
        //check if there is an error
        cout << "error receiving message: " << WSAGetLastError() << '\n';
        return false;
    }

    /////////////////////////////////////////////////////////
    cout << "Received " << bytesReceived << " bytes: " << '\n';
    cout << hex;
    for (int a = 0; a < bytesReceived; a++)
        cout << hex << setw(2) << std::setfill('0') << static_cast<unsigned>(static_cast<unsigned char>(receiveBuffer[a])) << ' ';
    cout << dec << '\n' << '\n';

    if (receiveBuffer[0] == 0)
    {
        //needs to save sender's ip and port
        char senderIpStr[INET_ADDRSTRLEN];
        if (inet_ntop(AF_INET, &senderAddress.sin_addr, senderIpStr, sizeof(senderIpStr)) == nullptr)
        {
            cout << "error getting the sender's ip address" << '\n';
            return false;
        }
        //saves the details about the sender
        portReceivedFrom = ntohs(senderAddress.sin_port);
        ipReceivedFrom = senderIpStr;
    }
    //adds the block received to the rest of the message
    (*message).insert((*message).end(), receiveBuffer, receiveBuffer + bytesReceived);

    //returns that the message was read successfully
    return true;
}
