#pragma once
#pragma pack(1)
#ifndef MESSAGE_STRUCTUREH
#define MESSAGE_STRUCTUREH
#include <boost/multiprecision/cpp_int.hpp>

//id of different messages
typedef enum : uint8_t
{
	CONNECT = 0, //ask the bootnode for your id and port
	ANSWER_CONNECT = 1, //bootnode answers to your ip and port question
	ASK_CLOSE = 2, //asks a user who are the closest nodes to a target
	ANSWER_CLOSE = 3 //ansewrs a user who are the closesr nodes to a target
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
	short nodePort;
	char nodeIp[4];
	char nodeId[32];
	short askPort;
	char askIp[4];
	char askId[32];
	char signature[64];
};
void Handle_Answer_Connect_Create(char* idAsk, Answer_Connect_1* ans);

//structure of message that asks about closest nodes, id is 2

struct Ask_Close_2
{
	id_t messageId;
	unsigned long long messageNumber;
	char nodeId[32];
	short nodePort;
	char nodeIp[4];
	char close[32];
	char signature[64];
};

#endif