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
	PAY = 6, //pays another user
	BLOCK = 7, //block on the blockchain
	CONTRACT = 8, ////////////////////////////////////////////////////////////////////////////////////////
	BIND_RANDOM_STAKING_POOL_OPERATOR = 9, //bind a staking pool operator that creates random numbers
	BIND_STAKING_POOL_OPERATOR = 10, //bind a staking pool operator that doesn't creates random numbers
	REVEAL = 11, //reveals a random number
	CONFIRM_BLOCK = 12, //confirms a block
	CONFIRM_BLOCK_ALL = 13, //confirms a block and makes his content valid
	ASK_ALL_INFO = 14, //asks the bootnode for all the info about the blockchain
	ANSWER_ALL_INFO = 15, //returns all the information about the blockchain
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

//message that starts a payment, id is 6
struct Pay_6
{
	id_t messageId;
	unsigned long long messageNumber;
	unsigned long long timeApply;
	NodeDetails senderDetails;
	NodeDetails receiverDetails;
	unsigned long long amountToPay;
	char signature[64];
};
void Handle_Pay_Create(NodeDetails* payTo, unsigned long long amountPay, Pay_6* ans);

//the structure of a transaction on the blockchain
struct Transaction
{
	Pay_6 payMessage;
	unsigned long long newAmountSender;
	unsigned long long newAmountReceiver;
};

//the structure of the start of a block
struct Block_7
{
	id_t messageId;
	char SHA256OfParent[32];
	unsigned long long BlockNumber;
	unsigned long long TimeAtCreation;
	NodeDetails BlockCreator;
	unsigned long long newAmountCreator;
	unsigned char HowmanyFromEachType[5];
	//HowmanyFromEachType[0] X Random_Reveal
	//HowmanyFromEachType[1] X NodeDetails - didn't reveal
	//HowmanyFromEachType[2] X Transaction
	//HowmanyFromEachType[3] X Bind_Random_Staking_Pool_Operator
	//HowmanyFromEachType[4] X Bind_Staking_Pool_Operator
	//signature
};
pair <char*, int> Handle_Block_Create(char* shaOfParent, unsigned long long blockNumber);

//the structure of the constract that binds a user as a staking pool operator that generates random numbers
struct Bind_Random_Staking_Pool_Operator_9
{
	id_t messageId;
	unsigned long long startTime;
	unsigned long long untilTime;
	char randomValueCommit[32];
	NodeDetails newStakingPoolOperator;
	char signature[64];
};
void Handle_Bind_Random_Staking_Pool_Operator_Create(char* randomVal, unsigned long long timeStart, unsigned long long timeEnd, Bind_Random_Staking_Pool_Operator_9* ans);

struct Contract_Random
{
	Bind_Random_Staking_Pool_Operator_9 contract;
	unsigned long long amountOfMoney;
};

//the structure of the constract that binds a user as a staking pool operator
struct Bind_Staking_Pool_Operator_10
{
	id_t messageId;
	unsigned long long startTime;
	unsigned long long untilTime;
	NodeDetails newStakingPoolOperator;
	char signature[64];
};
void Handle_Bind_Staking_Pool_Operator_Create(unsigned long long timeStart, unsigned long long timeEnd, Bind_Staking_Pool_Operator_10* ans);

struct Contract
{
	Bind_Staking_Pool_Operator_10 contract;
	unsigned long long amountOfMoney;
};

//the struct of a message that reveals a random number to determine the next block creator
struct Reveal_11
{
	id_t messageId;
	unsigned long long timeWhenSend;
	NodeDetails senderDetails;
	char HashOfContract[32];
	char randomValue[32];
	char signature[64];
};
void Handle_Reveal_Create(char* randomVal, char* hashOfContract, Reveal_11* ans);

struct Random_Reveal
{
	NodeDetails senderDetails;
	char randomValue[32];
};

//the struct of a message that confirms the current proposed block
struct Confirm_Block_12
{
	id_t messageId;
	unsigned long long messageNumber;
	NodeDetails senderDetails;
	char BlockSignature[64];
	char signature[64];
};
void Handle_Confirm_Block_Create(char* blockHash, Confirm_Block_12* ans);

struct Confirm_Block_All_13
{
	id_t messageId;
	unsigned long long messageNumber;
	char BlockHash[32];
	NodeDetails senderDetails;
	unsigned int numberOfSignatures;
	//{ NodeDetails, signature[64] } X numberOfSignatures
	//char signature[64];
};
pair <char*, int> Handle_Confirm_Block_All_Create();
bool Handle_Confirm_Block_All_Process(char* message, int len);

struct Ask_All_Info_14
{
	id_t messageId;
	unsigned long long messageNumber;
	bool allOrNot;
	NodeDetails senderDetails;
	char signature[64];
};
void Handle_Ask_All_Info_Create(Ask_All_Info_14* ans, bool isAll = true);

struct Answer_All_Info_15
{
	id_t messageId;
	unsigned long long messageNumber;
	char shaOfHeadBlock[32];
	NodeDetails senderDetails;
	NodeDetails ReceiverDetails;
	bool isLast;
	int howmanyEachType[2];
	//{ NodeDetails, unsigned long long } X howmanyEachType[0] - amounts of money
	//{ NodeDetails, unsigned long long } X howmanyEachType[1] - staking pool operators
	//char signature[64]
};
pair <char*, int> Handle_Answer_All_Info_Create(NodeDetails* whoAsked);

struct Info_Blockchain
{
	NodeDetails identity;
	unsigned long long coinsAmount;
	unsigned long long timeEnd;
};

//a struct for communication between the main thread and other threads
struct Communication_With_Threads
{
	id_t threadMessageId;
	int threadId;
	char* whereToCompare;
	char* whereToAnswer;
	mutex* canAccessThis;
};

//functions to know which messages were already sent and received
void addNewMapQueue(pair <unsigned long long, unsigned long long> time, int sizeOfMessage);
void addMessageInd(char* message, unsigned long long time, int ind);
bool getFirstAdded(int ind, char* placeTheAnswer);
void moveFirstLast(int ind);
void popFrontInd(int ind, bool moveSecond = true);
bool isAlreadyIn(char* message, int ind, bool isSHA256 = false);
void removeFromList(char* message, int ind, bool isSHA256 = false);
void setWas(char* message, int ind, bool isSHA256 = false);
bool getWas(char* message, int ind, bool isSHA256 = false);
int getSizeInd(int ind);
void reset(int ind);


#pragma pack(pop)
