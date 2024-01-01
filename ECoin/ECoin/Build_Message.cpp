#include <iostream>
#include <vector>
#include <boost/multiprecision/cpp_int.hpp>
#include <chrono>
#include "H_Constants.h"
#include "H_Node_Supporter.h"
#include "H_ECDSA.h"
#include "H_Message_Structure.h"
#include "H_Network_Interface.h"
#include "H_Variables.h"

using namespace std;
using namespace boost::multiprecision;

void Convert_Ip_To_Char_Array(string ip, char* save)
{
    //converts an ip from string to char array
    int place = 0;
    for (int a = 0; a < 4; a++)
    {
        int now = 0;
        while (place < ip.length() and ip[place] != '.')
        {
            now *= 10;
            now += ip[place] - '0';
            place++;
        }
        place++;
        save[a] = now;
    }
}

unsigned long long Get_Message_Number()
{
    //gets the number of milliseconds since jan 1970
    Number_Now++;
    if (Number_Now == 100000)
        Number_Now = 0;
    auto currentTimePoint = chrono::system_clock::now();
    auto durationSinceEpoch = currentTimePoint.time_since_epoch();
    int64_t millisecondsSinceEpoch = chrono::duration_cast<chrono::milliseconds>(durationSinceEpoch).count();

    return millisecondsSinceEpoch * 100000 + Number_Now;
}

//message with id 0
void Handle_Connect_Create(Connect_0* ans)
{
    //creates the message to send in order to connect
    ans->messageId = CONNECT;
    ans->messageNumber = Get_Message_Number();
    copy(begin(My_Id), begin(My_Id) + 32, begin(ans->nodeId));
    signMessage((const unsigned char*)ans, sizeof(Connect_0) - 64, (unsigned char*)ans->signature);
}

//message with id 1
void Handle_Answer_Connect_Create(char* idAsk, Answer_Connect_1* ans)
{
    //creates the answer to send after receiving connect
    ans->messageId = ANSWER_CONNECT;
    ans->messageNumber = Get_Message_Number();
    copy(begin(My_Ip), end(My_Ip), begin(ans->nodeIp));
    ans->nodePort = My_Port;
    copy(begin(My_Id), end(My_Id), begin(ans->nodeId));
    pair <string, int> address = getAddress();
    Convert_Ip_To_Char_Array(address.first, ans->askIp);
    ans->askPort = (short)address.second;
    copy(idAsk, idAsk + 32, begin(ans->askId));
    signMessage((const unsigned char*)ans, sizeof(Answer_Connect_1) - 64, (unsigned char*)&(ans->signature));
}