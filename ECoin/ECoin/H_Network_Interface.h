#pragma once
#ifndef NETWORK_INTERFACEH
#define NETWORK_INTER
#include <iostream>
#include <vector>

using namespace std;

//from Network_Interface.cpp
bool initSocket(bool = false, int = -1);
void closeSocket();
int getPort();
string getIp();
pair <string, int> getAddress();
bool sendMessage(char* message, int len, char* receivingIp, int receivingPort);
bool receiveMessage(vector <char>* message);

#endif