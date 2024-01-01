#include "H_Variables.h"
#include "H_Message_Structure.h"
#include "H_Network_Interface.h"
#include "H_ECDSA.h"
#include <iostream>

char My_Id[32];
char My_Private_Key[64];
char My_Ip[4];
unsigned short My_Port;
int Number_Now = 0;
NodeData Bootnode_Details[Number_Of_Bootnodes] = { {} };
bool Is_Bootnode = false;

void initValues()
{
	createKeys((unsigned char*)My_Id, (unsigned char*)My_Private_Key);

	Connect_0* message = new Connect_0{};
	Handle_Connect_Create(message);
	if (!sendMessage((char*)message, sizeof(Connect_0), (char*)Bootnode_Details[0].nodeIp.c_str(), Bootnode_Details[0].nodePort))
		cout << "error" << '\n';
	else
		cout << "sent the message!" << '\n';
	bool isGood = verifySignature((const unsigned char*)message, sizeof(Connect_0) - 64, (const unsigned char*)&message->nodeId, (const unsigned char*)&message->signature);
	if (!isGood)
		cout << "error signature is not correct from the start!!!!" << '\n';
	else
		cout << "signature is good" << '\n';
	int t;
	vector <char> mess;
	while (true)
	{
		cin >> t;
		if (t == -1)
			break;
		message = {};
		receiveMessage(&mess);
		handleMessage(mess.data(), mess.size());
		cout << '\n';
	}
	/*cout << "1: " << hex << message->messageId << '\n';
	cout << "2: " << message->messageNumber << '\n';
	cout << "3: ";
	for (int a = 0; a < 32; a++)
		cout << hex << setw(2) << setfill('0') << static_cast<unsigned>(static_cast<unsigned char>(message->nodeId[a])) << ' ';
	cout << '\n' << "4: ";
	for (int a = 0; a < 64; a++)
		cout << hex << setw(2) << setfill('0') << static_cast<unsigned>(static_cast<unsigned char>(message->signature[a])) << ' ';
	cout << '\n' << dec << "after" << '\n';
	cout << "public key is: ";
	for (int a = 0; a < 32; a++)
		cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned>(static_cast<unsigned char>(My_Id[a])) << ' ';*/
}