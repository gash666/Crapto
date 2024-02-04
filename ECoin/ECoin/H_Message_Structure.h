#pragma once
#include <mutex>
#include "H_Node_Supporter.h"
#include "H_Constants.h"

#pragma pack(push, 1)

//id of different messages
typedef enum : uint8_t
{
	CONNECT = 0, //ask the bootnode for your id and port
	ANSWER_CONNECT = 1, //bootnode answers to your ip and port question
	ASK_CLOSE = 2, //asks a user who are the closest nodes to a target
	ANSWER_CLOSE = 3, //ansewrs a user who are the closesr nodes to a target
	ASK_PING = 4, //pings a user who wasn't active for a while
	ANSWER_PING = 5, //answers a ping with a proof the user is active
	DONT_EXIST = 255
} id_t;

void handleMessage(char* message, int len);

//message that asks for its ip and port, id is 0
struct Connect_0
{
	id_t messageId;
	unsigned long long messageNumber;
	char nodeId[32];
	char signature[64];
};
void Handle_Connect_Create(Connect_0* ans);

//message that answers to Connect_0, id is 1
struct Answer_Connect_1
{
	id_t messageId;
	unsigned long long messageNumber;
	NodeDetails senderDetails;
	NodeDetails answerIdentity;
	char signature[64];
};
void Handle_Answer_Connect_Create(char* idAsk, Answer_Connect_1* ans);

//message that asks about closest nodes, id is 2
struct Ask_Close_2
{
	id_t messageId;
	unsigned long long messageNumber;
	NodeDetails senderDetails;
	char target[32];
	char signature[64];
};
void Handle_Ask_Close_Create(char* closeTo, Ask_Close_2* ans);

//message that returns an answer to who are the close neighbors, id is 3
struct Answer_Close_3
{
	id_t messageId;
	unsigned long long messageNumber;
	NodeDetails senderDetails;
	char howmanyInAnswer;
	NodeDetails answerClose[Bucket_Size];
	char target[32];
	char signature[64];
};
void Handle_Answer_Close_Create(char* closeTo, Answer_Close_3* ans);

//message that pings another user to check if he is online, id is 4
struct Ask_Ping_4
{
	id_t messageId;
	unsigned long long messageNumber;
	NodeDetails senderDetails;
	NodeDetails receiverDetails;
	char signature[64];
};
void Handle_Ask_Ping_Create(NodeDetails* sendTo, Ask_Ping_4* ans);

//message that answers to a ping and returns that the user is online, id is 5
struct Answer_Ping_5
{
	id_t messageId;
	unsigned long long messageNumber;
	NodeDetails senderDetails;
	NodeDetails receiverDetails;
	char signature[64];
};
void Handle_Answer_Ping_Create(NodeDetails* sendTo, Answer_Ping_5* ans);

//a struct for communication between the main thread and other threads
struct Communication_With_Threads
{
	id_t threadMessageId;
	int threadId;
	char* whereToCompare;
	char* whereToAnswer;
	mutex* canAccessThis;
};

#pragma pack(pop)
