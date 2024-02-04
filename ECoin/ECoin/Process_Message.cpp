#include <iostream>
#include <vector>
#include "H_Constants.h"
#include "H_Node_Supporter.h"
#include "H_ECDSA.h"
#include "H_Message_Structure.h"
#include "H_Network_Interface.h"
#include "H_Network_Operations.h"
#include "H_Variables.h"
#include <iomanip>

using namespace std;

//message with id 0
void Handle_Connect_Process(char* message, int len)
{
	//answers the request to get the ip and port
	//checks if the message is of the correct size
	if (!Is_Bootnode or len != sizeof(Connect_0))
		return;

	//checks that the signature on the message is correct
	Connect_0* m = (Connect_0*)message;
	bool isGood = verifySignature((const unsigned char*)m, offsetof(Connect_0, signature), (const unsigned char*)&(m->nodeId), (const unsigned char*)&(m->signature));
	if (!isGood)
	{
		cout << "Process_Message.cpp Handle_Connect_Process error signature is not correct" << '\n';
		return;
	}

	//creates an answer to the request
	Answer_Connect_1* returnMessage = new Answer_Connect_1{};
	Handle_Answer_Connect_Create(m->nodeId, returnMessage);
	pair <string, int> address = getAddress();

	Update_Ping_Timer(&returnMessage->answerIdentity);

	//sends the answer message
	sendMessage((char*)returnMessage, sizeof(Answer_Connect_1), (char*)address.first.c_str(), address.second, true);
}

//message with id 1
void Handle_Answer_Connect_Process(char* message, int len)
{
	//check if 
	if (len != sizeof(Answer_Connect_1))
		return;

	//check if the signature is ok
	Answer_Connect_1* m = (Answer_Connect_1*)&(message[0]);
	bool isGood = verifySignature((const unsigned char*)m, offsetof(Answer_Connect_1, signature), (const unsigned char*)&(m->senderDetails.nodeID), (const unsigned char*)&(m->signature));
	if (!isGood)
	{
		cout << "Process_Message.cpp Handle_Answer_Connect_Process error signature is not correct" << '\n';
		return;
	}

	//check if the message is from one of the bootnodes
	bool isReal = false;
	//for (int a = 0; a < Number_Of_Bootnodes; a++)
	//	for (int b = 0; b < 32; b++)
	//		if (Bootnode_Details[a].nodeId[b] == m->nodeId[b])
				isReal = true;

	//check that the destination is the user
	for (int a = 0; a < 32; a++)
		if (My_Details.nodeID[a] != m->answerIdentity.nodeID[a])
			isReal = false;
	if (!isReal)
		return;

	Update_Ping_Timer(&m->senderDetails);

	//updates the ip and port
	copy(begin(m->answerIdentity.ip), end(m->answerIdentity.ip), My_Details.ip);
	My_Details.port = m->answerIdentity.port;
}

//message with id 2
void Handle_Ask_Close_Process(char* message, int len)
{
	//answer the request to get the closest to some id
	if (len != sizeof(Ask_Close_2))
		return;

	//check if the signature is ok
	Ask_Close_2* m = (Ask_Close_2*)&(message[0]);
	bool isGood = verifySignature((const unsigned char*)m, offsetof(Ask_Close_2, signature), (const unsigned char*)&(m->senderDetails.nodeID), (const unsigned char*)&(m->signature));
	if (!isGood)
	{
		cout << "Process_Message.cpp Handle_Ask_Close_Process error signature is not correct" << '\n';
		return;
	}

	Update_Ping_Timer(&m->senderDetails);

	//sends an answer to the message
	Answer_Close_3* returnMessage = new Answer_Close_3{};
	Handle_Answer_Close_Create(m->target, returnMessage);
	sendMessage((char*)returnMessage, sizeof(Answer_Close_3), m->senderDetails.ip, m->senderDetails.port);
}

//message with id 3
void Handle_Answer_Close_Process(char* message, int len)
{
	//answer the request to get the closest to some id
	if (len != sizeof(Answer_Close_3))
		return;

	//check if the signature is ok
	Answer_Close_3* m = (Answer_Close_3*)&(message[0]);
	bool isGood = verifySignature((const unsigned char*)m, offsetof(Answer_Close_3, signature), (const unsigned char*)&(m->senderDetails.nodeID), (const unsigned char*)&(m->signature));
	if (!isGood)
	{
		cout << "Process_Message.cpp Handle_Answer_Close_Process error signature is not correct" << '\n';
		return;
	}

	//search for the communications with the thread in the vector
	Communication_With_Threads threadDetails;
	threadDetails.threadMessageId = DONT_EXIST;
	CanChangeCommWithThreads.lock();
	for (int a = 0; a < commWithThreadsDetails.size(); a++)
		if (commWithThreadsDetails[a].threadMessageId == ANSWER_CLOSE)// and memcmp(commWithThreadsDetails[a].whereToCompare, message + offsetof(Answer_Close_3, target), 32) == 0)
		{
			cout << "here, problem with copy" << '\n';
			copy(&commWithThreadsDetails[a], (Communication_With_Threads*)((char*)&commWithThreadsDetails[a] + sizeof(Communication_With_Threads)), &threadDetails);
		}

	//check if a proper thread was found
	if (threadDetails.threadMessageId != DONT_EXIST)
	{
		threadDetails.canAccessThis->lock();
		CanChangeCommWithThreads.unlock();

		//get the index of the tree
		char newInd = threadDetails.whereToAnswer[0];

		//add the nodes to the tree
		for (int a = 0; a < Bucket_Size; a++)
			if (m->answerClose[a].port == 0)
				break;
			else
				addNodeToTreeInd(&m->answerClose[a], newInd);

		//set that the sender has sent an answer to the question
		setHasSentInd(&m->senderDetails, newInd);

		threadDetails.canAccessThis->unlock();
	}
	else
		CanChangeCommWithThreads.unlock();

	Update_Ping_Timer(&m->senderDetails);
}

//message with id 4
void Handle_Ask_Ping_Process(char* message, int len)
{
	//answer the ping
	if (len != sizeof(Ask_Ping_4))
		return;

	//check if the signature is ok
	Ask_Ping_4* m = (Ask_Ping_4*)&(message[0]);
	bool isGood = verifySignature((const unsigned char*)m, offsetof(Ask_Ping_4, signature), (const unsigned char*)&(m->senderDetails.nodeID), (const unsigned char*)&(m->signature));
	if (!isGood)
	{
		cout << "Process_Message.cpp Handle_Ask_Ping_Process error signature is not correct" << '\n';
		return;
	}

	//if the destination is not the user return
	if (memcmp(m->receiverDetails.nodeID, My_Details.nodeID, 32) != 0)
		return;

	Update_Ping_Timer(&m->senderDetails);

	//create and send an answer to the ping
	Answer_Ping_5* returnMessage = new Answer_Ping_5{};
	Handle_Answer_Ping_Create(&m->senderDetails, returnMessage);
	sendMessage((char*) returnMessage, sizeof(Answer_Ping_5), returnMessage->receiverDetails.ip, returnMessage->receiverDetails.port);
}

void Handle_Answer_Ping_Process(char* message, int len)
{
	//update that the node is active
	if (len != sizeof(Answer_Ping_5))
		return;

	//check if the signature is ok
	Answer_Ping_5* m = (Answer_Ping_5*)&(message[0]);
	bool isGood = verifySignature((const unsigned char*)m, offsetof(Answer_Ping_5, signature), (const unsigned char*)&(m->senderDetails.nodeID), (const unsigned char*)&(m->signature));
	if (!isGood)
	{
		cout << "Process_Message.cpp Handle_Answer_Ping_Process error signature is not correct" << '\n';
		return;
	}
	Update_Ping_Timer(&m->senderDetails);
}

void handleMessage(char* message, int len)
{
	//receives a message and reacts to it accordingly
	//check if the message is longer than the minimum size
	if (len < Minimum_Message_Size)
		return;

	//check for every type of message and handle it
	if (message[0] == CONNECT)
		Handle_Connect_Process(message, len);
	else if (message[0] == ANSWER_CONNECT)
		Handle_Answer_Connect_Process(message, len);
	else if (message[0] == ASK_CLOSE)
		Handle_Ask_Close_Process(message, len);
	else if (message[0] == ANSWER_CLOSE)
		Handle_Answer_Close_Process(message, len);
	else if (message[0] == ASK_PING)
		Handle_Ask_Ping_Process(message, len);
	else if (message[0] == ANSWER_PING)
		Handle_Answer_Ping_Process(message, len);
}
