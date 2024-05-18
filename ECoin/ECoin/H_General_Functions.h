#pragma once

#include "H_Message_Structure.h"
#include <string>
using namespace std;

bool initValues(wstring username);
bool loadIntoFile();
void Convert_Ip_To_Char_Array(string ip, char* save);
unsigned long long Get_Time();
void fillRandom(unsigned char* randomValue, int numberBytes);
void turnToChar(char* place, char* value, int size);
void turnToASCII(char* place, char* value, int size);
void sendMessageStakingPoolOperator(bool isToNow = false);
void askInformation(bool isAll);
void processCommands();
