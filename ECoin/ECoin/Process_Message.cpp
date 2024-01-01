#include <iostream>
#include <vector>
#include <boost/multiprecision/cpp_int.hpp>
#include "H_Constants.h"
#include "H_Node_Supporter.h"
#include "H_ECDSA.h"
#include "H_Message_Structure.h"
#include "H_Network_Interface.h"
#include "H_Variables.h"

using namespace std;
using namespace boost::multiprecision;

//message with id 0
void Handle_Connect_Process(char* message, int len)
{
	//answers the request to get the ip and port
	//checks if the message is valid
	if (!Is_Bootnode or len != sizeof(Connect_0))
		return;
	Connect_0* m = (Connect_0*)message;
	//	bool isGood = verifySignature((const unsigned char*)&message, sizeof(Connect_0) - 64, (const unsigned char*)&message->nodeId, (const unsigned char*)&message->signature);
	bool isGood = verifySignature((const unsigned char*)m, sizeof(Connect_0) - 64, (const unsigned char*)&(m->nodeId), (const unsigned char*)&(m->signature));
	if (!isGood)
	{
		cout << "Process_Message.cpp Handle_Connect_Process error signature is not correct" << '\n';
		return;
	}
	Answer_Connect_1* returnMessage = new Answer_Connect_1{};
	
	//creates an answer to the request
	Handle_Answer_Connect_Create(m->nodeId, returnMessage);
	pair <string, int> address = getAddress();

	//sends the answer message
	sendMessage((char*)returnMessage, sizeof(Answer_Connect_1), (char*)address.first.c_str(), address.second);
}

//message with id 1
void Handle_Answer_Connect_Process(char* message, int len)
{
	//check if 
	if (len != sizeof(Answer_Connect_1))
		return;
	Answer_Connect_1* m = (Answer_Connect_1*)&(message[0]);

	//check if the signature is ok
	bool isGood = verifySignature((const unsigned char*)m, sizeof(Answer_Connect_1) - 64, (const unsigned char*)&(m->nodeId), (const unsigned char*)&(m->signature));
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
		if (My_Id[a] != m->askId[a])
			isReal = false;
	if (!isReal)
		return;

	//updates the ip and port
	copy(begin(m->askIp), end(m->askIp), My_Ip);
	My_Port = m->askPort;

	cout << "my ip is: " << dec;
	for (int a = 0; a < 4; a++)
		cout << setw(3) << setfill('0') << static_cast<unsigned>(static_cast<unsigned char>(My_Ip[a])) << ' ';
	cout << '\n' << "my port is: " << My_Port << '\n';
}

//message with id 2
void Handle_Ask_Close_Process(char* message, int len)
{
	//answer the request to get the closest to some id
	if (len != sizeof(Ask_Close_2))
		return;
	Ask_Close_2* m = (Ask_Close_2*)&(message[0]);
	vector <NodeDetails> now;
	//fillList(Bucket_Size, &m->close, &now);
	char* returnMessage = nullptr;

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

}