#pragma once
#include <iostream>
#include <vector>

using namespace std;

//from Network_Interface.cpp
bool initSocket(bool = false, int = -1);
void closeSocket();
int getPort();
string getIp();
bool sendMessage(char*, char*, int);
bool receiveMessage(vector <char>*);