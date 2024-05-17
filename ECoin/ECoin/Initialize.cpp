#include <boost/asio.hpp>
#include "H_Variables.h"
#include "H_Constants.h"
#include "H_Network_Operations.h"
#include "H_Network_Interface.h"
#include "H_Node_Supporter.h"
#include "H_Message_Structure.h"
#include "H_ECDSA.h"
#include "H_Treap.h"
#include "H_General_Functions.h"
#include "H_Block_Tree.h"
#include "H_Maintain_Blockchain.h"

#ifndef SODIUM
#define SODIUM
#define SODIUM_STATIC
#include "sodium.h"
#endif

#include <iostream>
#include <Windows.h>
#include <iomanip>

//initialize global variables
NodeDetails My_Details;
char My_Private_Key[64];
long long Number_Now;
bool Is_Bootnode = false;
unsigned long long Number_Coins;
char Buffer_For_Receiving_Messages[Maximum_Message_Size];
wstring Database_Path;
NodeDetails Bootnode_Details[Number_Of_Bootnodes];
NodeDetails zeroNode;
vector <Communication_With_Threads> commWithThreadsDetails;
mutex CanChangeCommWithThreads;
boost::asio::thread_pool ThreadPool(Number_Of_Threads);
bool Is_Staking_Pool_Operator = false;
bool hasContract = false;
bool isCreatingRandom;
vector <array <char, 32>> revealNumbers;
int indexLastReveal;
unsigned long long Block_Number;
NodeDetails Next_Block_Creator;
char hashBlockCreating[32];
bool isThisUserCreating;
vector <pair <NodeDetails, array<char, 64>>> Signatures_For_Confirm_Message;
deque <pair <NodeDetails, unsigned long long>> May_Need_Information;
char ShaOfHeadBlockInitializing[32];
unsigned long long lastTimeReceivedRandom = 0;
bool isFirstRandomReceived = false;
mutex canUseBlockTreeActions;
mutex canReceiveRandom;
bool isFirstAll = false;
bool hasInfo = false;
unsigned long long lastTimeApproved = 0;
unsigned long long blockNumberApproved;

char getFromInt(int val)
{
	//returns the char of a ASCII value
	if (0 <= val and val <= 9)
		return val + '0';
	else if (10 <= val and val <= 15)
		return val - 10 + 'a';
	return 0;
}

void turnToASCII(char* place, char* value, int size)
{
	//copies size bytes in hex from value into place
	for (int a = 0; a < size; a++)
	{
		place[a * 2] = getFromInt((value[a] >> 4) & 0xF);
		place[a * 2 + 1] = getFromInt(value[a] & 0xF);
	}
}

bool loadIntoFile()
{
	//writes the keys and amount of money the user has into the database file
	char dataToWrite[Data_Base_File_Size];
	turnToASCII(dataToWrite, My_Private_Key, 64);
	dataToWrite[128] = '\n';
	turnToASCII(&(dataToWrite[129]), My_Details.nodeID, 32);
	dataToWrite[193] = '\n';

	//convert the amount of money into char array
	char tempNumberCoins[sizeof(unsigned long long)];
	memcpy(tempNumberCoins, &Number_Coins, sizeof(unsigned long long));
	turnToASCII(&(dataToWrite[194]), tempNumberCoins, sizeof(unsigned long long));

	//creates a handle for writing into the database file
	HANDLE fileHandle = CreateFile(
		Database_Path.c_str(),  //path of the file
		GENERIC_WRITE,          //access
		0,                      //share mode
		NULL,                   //security attributes
		OPEN_ALWAYS,			//what to do if file exists
		FILE_ATTRIBUTE_NORMAL,  //attributes and flags
		NULL                    //template file
	);

	//checks if there was an error reading
	if (fileHandle == INVALID_HANDLE_VALUE)
	{
		cout << "error cant open database file for writing" << '\n';
		CloseHandle(fileHandle);
		return false;
	}

	//writes the data to the file
	DWORD bytesWritten;
	if (!WriteFile(fileHandle, dataToWrite, static_cast<DWORD>(Data_Base_File_Size), &bytesWritten, NULL))
	{
		//handles an error while writing to the file
		cout << "error writing to database" << '\n';
		CloseHandle(fileHandle);
		return false;
	}

	//close the handle and return that the write was done successfuly
	CloseHandle(fileHandle);
	return true;
}

int getFromHex(char val)
{
	//returns the hex value of a char
	if ('0' <= val and val <= '9')
		return val - '0';
	else if ('a' <= val and val <= 'f')
		return val - 'a' + 10;
	return 0;
}

void turnToChar(char* place, char* value, int size)
{
	//places the values into the variables
	for (int a = 0; a < size; a++)
		place[a] = getFromHex(value[a * 2]) << 4 | getFromHex(value[a * 2 + 1]);
}

bool loadFromFile(wstring username)
{
	//handles the loading of private and public keys and how much money the user has
	//get the path of the exe file
	WCHAR buffer[MAX_PATH];
	GetModuleFileName(NULL, buffer, MAX_PATH);

	//remove the last word from the path
	wstring path(buffer);
	int ind = (int)path.length() - 1;
	while (ind >= 0 and path[ind] != '\\')
		ind--;
	wstring newPath = path.substr(0, ind);

	//adds the directory name to the path
	newPath.append(L"\\database");

	//checks if the directory exists
	DWORD fileAttributes = GetFileAttributes(newPath.c_str());
	if (!((fileAttributes != INVALID_FILE_ATTRIBUTES) && (fileAttributes & FILE_ATTRIBUTE_DIRECTORY)))
	{
		//if the directory doesnt exist, create it
		if (!(CreateDirectory(newPath.c_str(), NULL) || GetLastError() == ERROR_ALREADY_EXISTS))
		{
			cout << "error creating directory for the database" << '\n';
			return false;
		}
	}

	//creates the path for the wanted txt file for the user
	newPath.append(L"\\database_");
	newPath.append(username);
	newPath.append(L".txt");

	//initializes the global variable holding the path
	Database_Path = newPath;

	//creates a handle for the file
	HANDLE createFile = CreateFile(
		newPath.c_str(),		//path of the file
		GENERIC_READ,			//access
		0,						//share mode
		NULL,					//security attributes
		CREATE_NEW,				//what to do if file exists
		FILE_ATTRIBUTE_NORMAL,	//attributes and flags
		NULL					//template file
	);

	//checks if there was an error in trying to open/create the file
	if (createFile == INVALID_HANDLE_VALUE)
	{
		//checks if the error is not that the file exists
		if (GetLastError() != 80)
		{
			cout << "error opening or creating database file " << GetLastError() << '\n';
			CloseHandle(createFile);
			return false;
		}

		//closes the handle of the file that was meant to create it
		CloseHandle(createFile);

		HANDLE readFile = CreateFile(
			newPath.c_str(),		//path of the file
			GENERIC_READ,			//access
			0,						//share mode
			NULL,					//security attributes
			OPEN_EXISTING,			//what to do if file exists
			FILE_ATTRIBUTE_NORMAL,	//attributes and flags
			NULL					//template file
		);

		//get the file's size
		DWORD fileSize = GetFileSize(readFile, NULL);

		//checks that the size is right
		if (fileSize != Data_Base_File_Size)
		{
			cout << "error database file's size is wrong" << '\n';
			CloseHandle(readFile);
			return false;
		}

		//reading from the file
		char* filedata = new char[Data_Base_File_Size];
		DWORD bytesRead;
		if (!ReadFile(readFile, filedata, fileSize, &bytesRead, NULL))
		{
			cout << "error reading from the database file" << '\n';
			CloseHandle(readFile);
			return false;
		}

		//reads the data from the buffer into the corresponding variables
		turnToChar(My_Private_Key, filedata, 64);
		turnToChar(My_Details.nodeID, filedata + 128 + 1, 32);

		char tempNumberCoins[sizeof(unsigned long long)];
		turnToChar(tempNumberCoins, filedata + 128 + 64 + 2, sizeof(unsigned long long));

		//convert the char array into double
		memcpy(&Number_Coins, tempNumberCoins, sizeof(unsigned long long));

		//sets the public and private keys as the ones read from the file
		setKey((unsigned char*)My_Details.nodeID, (unsigned char*)My_Private_Key);

		//close the handle
		CloseHandle(readFile);
	}
	else
	{
		//create the file and initialize it
		//initialize the required variables for the database file
		CloseHandle(createFile);
		createKeys((unsigned char*)My_Details.nodeID, (unsigned char*)My_Private_Key);
		Number_Coins = 0;

		//call for the function that updates the database file with the values of the
		if (!loadIntoFile())
			return false;
	}
	return true;
}

bool initValues(wstring username)
{
	//initialize libsodium
	if (sodium_init() < 0)
	{
		cout << "error initializing libsodium" << '\n';
		return false;
	}

	//initialize the variable that saves the head of the head of the block tree
	for (int a = 0; a < 32; a++)
		ShaOfHeadBlockInitializing[a] = 0;

	//initializes the keys and how much money the user has
	if (!loadFromFile(username))
		return false;

	//initializes the values for the bootnode list
	char tempIp[4] = { (char)127, (char)0, (char)0, (char)1 };//{ (char)77, (char)139, (char)1, (char)166 };//
	char tempID[32] = {(char)-13, (char)-109, (char)112, (char)-107,
		(char)-63, (char)39, (char)20, (char)-59, 
		(char)-16, (char)-77, (char)120, (char)56, 
		(char)40, (char)-53, (char)25, (char)-86, 
		(char)116, (char)-29, (char)-86, (char)-19, 
		(char)108, (char)-56, (char)-127, (char)68, 
		(char)-7, (char)102, (char)-100, (char)-54, 
		(char)121, (char)-57, (char)61, (char)5 };
	Bootnode_Details[0].port = (unsigned short)51647;
	memcpy(Bootnode_Details[0].ip, tempIp, 4);
	memcpy(Bootnode_Details[0].nodeID, tempID, 32);

	//creates trees for the communication and for staking pool operator 
	occupyNewTree();
	occupyNewTree();

	//structures that save which messages were already received and helps filling blocks with actions
	addNewMapQueue({ Time_Message_Valid, Max_Time_Spread }, sizeof(Pay));
	addNewMapQueue({ Time_Message_Valid, Max_Time_Spread }, 32);
	addNewMapQueue({ Time_Message_Valid, Max_Time_Spread }, sizeof(Bind_Random_Staking_Pool_Operator));
	addNewMapQueue({ Time_Message_Valid, Max_Time_Spread }, sizeof(Bind_Staking_Pool_Operator));
	addNewMapQueue({ Time_Message_Valid, Max_Time_Spread }, sizeof(Reveal));
	addNewMapQueue({ Time_Message_Valid, Max_Time_Spread }, 32);
	addNewMapQueue({ Time_Message_Valid, Max_Time_Spread }, 32);

	//adds a treap of the staking pool operators
	initTreap();
	initTreap();
	initTreap();
	initTreap();
	initTreap();
	initTreap();

	//monitor ping messages
	post(ThreadPool, []() { MonitorAfterPing(); });
	post(ThreadPool, []() { MonitorBeforePing(); });

	//sends a message to the bootnode to get the node's ip and port outside the NAT
	if (!Is_Bootnode)
	{
		//if the user is not a bootnode, send a message asking for the port and ip outside of the NAT
		initSocket(true);
		if (!connectToBootnode(false))
		{
			cout << "error connecting to the network" << '\n';
			return false;
		}
		else
		{
			//adds this user as a node known to the user
			addNodeToTreeInd(&My_Details, 0);
			post(ThreadPool, []() { discoverNodesInTheNetwork(); });
		}
	}
	else
	{
		//set the id port and ip to those of the bootnode
		copy(&Bootnode_Details[0], (NodeDetails*)((char*)&Bootnode_Details[0] + sizeof(NodeDetails)), &My_Details);
		initSocket(true, My_Details.port);

		//adds this user as a node known to the user
		addNodeToTreeInd(&My_Details, 0);
		addNodeToTreeInd(&My_Details, 1);

		//if the network is already online, discover other nodes
		if (connectToBootnode(true))
			post(ThreadPool, []() { discoverNodesInTheNetwork(); });

		if (getTreeSizeInd(0) == 1)
		{
			//create the first block
			//wait until the correct time
			this_thread::sleep_for(chrono::milliseconds(Time_Block - Get_Time() % Time_Block));

			//set the correct amount of money for the bootnode
			if (Number_Coins == 0)
				setAmountMoneyInd(&My_Details, (unsigned long long)Bootnode_Start_Money - Number_Coins_Per_Block, 1);
			else
				setAmountMoneyInd(&My_Details, Number_Coins, 1);

			//make the bootnode into a random staking pool operator
			isCreatingRandom = true;
			hasInfo = true;
			hasContract = true;
			Is_Staking_Pool_Operator = true;
			isFirstAll = true;
			sendMessageStakingPoolOperator(true);
		}
	}

	cout << "my ip is: ";
	for (int a = 0; a < 4; a++)
	{
		cout << int((unsigned char)My_Details.ip[a]);
		if (a != 3)
			cout << ".";
	}
	cout << '\n' << "my port is: " << getPort() << '\n';

	return true;
}
