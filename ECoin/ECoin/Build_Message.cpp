#include <iostream>
#include <vector>
#include <chrono>
#include <iomanip>
#include "H_Constants.h"
#include "H_Node_Supporter.h"
#include "H_ECDSA.h"
#include "H_Message_Structure.h"
#include "H_Network_Interface.h"
#include "H_Variables.h"

using namespace std;

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
    copy(My_Details.nodeID, My_Details.nodeID + sizeof(NodeDetails::nodeID), ans->nodeId);
    signMessage((const unsigned char*)ans, offsetof(Connect_0, signature), (unsigned char*)ans->signature);
}

//message with id 1
void Handle_Answer_Connect_Create(char* idAsk, Answer_Connect_1* ans)
{
    //creates the answer to send after receiving connect
    ans->messageId = ANSWER_CONNECT;
    ans->messageNumber = Get_Message_Number();
    copy((char*) & My_Details, ((char*)&My_Details + sizeof(NodeDetails)), (char*)&ans->senderDetails);
    pair <string, int> address = getAddress();
    Convert_Ip_To_Char_Array(address.first, ans->answerIdentity.ip);
    ans->answerIdentity.port = (short)address.second;
    copy(idAsk, idAsk + 32, ans->answerIdentity.nodeID);
    signMessage((const unsigned char*)ans, offsetof(Answer_Connect_1, signature), (unsigned char*)&(ans->signature));
}

//message with id 2
void Handle_Ask_Close_Create(char* closeTo, Ask_Close_2* ans)
{
    //creates the question to ask who is close to a certain node
    ans->messageId = ASK_CLOSE;
    ans->messageNumber = Get_Message_Number();
    copy(&My_Details, (NodeDetails*)((char*)&My_Details + sizeof(NodeDetails)), &ans->senderDetails);
    copy(closeTo, closeTo + 32, ans->target);
    signMessage((const unsigned char*)ans, sizeof(Ask_Close_2) - 64, (unsigned char*)&(ans->signature));
}

//message with id 3
void Handle_Answer_Close_Create(char* closeTo, Answer_Close_3* ans)
{
    //creates a message that answers the question in the message received
    ans->messageId = ANSWER_CLOSE;
    ans->messageNumber = Get_Message_Number();
    copy(&My_Details, (NodeDetails*)((char*)&My_Details + sizeof(NodeDetails)), &ans->senderDetails);
    for (int a = 0; a < Bucket_Size; a++)
        ans->answerClose[a].port = 0;
    fillListInd(Bucket_Size, closeTo, ans->answerClose, 0);
    copy(closeTo, closeTo + 32, ans->target);
    signMessage((const unsigned char*)ans, sizeof(Answer_Close_3) - 64, (unsigned char*)&(ans->signature));
}

//message with id 4
void Handle_Ask_Ping_Create(NodeDetails* sendTo, Ask_Ping_4* ans)
{
    //creates a message that pings another user
    ans->messageId = ASK_PING;
    ans->messageNumber = Get_Message_Number();
    copy(&My_Details, (NodeDetails*)((char*)&My_Details + sizeof(NodeDetails)), &ans->senderDetails);
    copy(sendTo, (NodeDetails*)((char*)sendTo + sizeof(NodeDetails)), &ans->receiverDetails);
    signMessage((const unsigned char*)ans, sizeof(Ask_Ping_4) - 64, (unsigned char*)&(ans->signature));
}

//message with id 5
void Handle_Answer_Ping_Create(NodeDetails* sendTo, Answer_Ping_5* ans)
{
    //creates a message that answers a ping from another user
    ans->messageId = ANSWER_PING;
    ans->messageNumber = Get_Message_Number();
    copy(&My_Details, (NodeDetails*)((char*)&My_Details + sizeof(NodeDetails)), &ans->senderDetails);
    copy(sendTo, (NodeDetails*)((char*)sendTo + sizeof(NodeDetails)), &ans->receiverDetails);
    signMessage((const unsigned char*)ans, sizeof(Answer_Ping_5) - 64, (unsigned char*)&(ans->signature));
}