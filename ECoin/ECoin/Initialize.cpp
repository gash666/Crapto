#include "H_Variables.h"
#include "H_Constants.h"
#include "H_Message_Structure.h"
#include "H_Network_Interface.h"
#include "H_ECDSA.h"
#include <iostream>
#include <Windows.h>
#include <iomanip>

uint256_t My_Id_As_Number;
char My_Id[32];
char My_Private_Key[64];
char My_Ip[4];
unsigned short My_Port;
long long Number_Now;
NodeData Bootnode_Details[Number_Of_Bootnodes] = { { (unsigned short) 51648, "77.139.1.166", 1 } };
bool Is_Bootnode = false;
double Number_Coins;
wstring Database_Path;

void setIdAsNumber()
{
	//sets the node's id as a uint256_t variable that can be used easily
	uint256_t temp;
	for (int a = 0; a < 32; a++)
	{
		temp = static_cast<uint8_t>(My_Id[a]);
		My_Id_As_Number = (My_Id_As_Number << 8) | temp;
	}
}

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
	turnToASCII(&dataToWrite[129], My_Id, 32);
	dataToWrite[193] = '\n';

	//convert the amount of money into char array
	char tempNumberCoins[sizeof(double)];
	memcpy(tempNumberCoins, &Number_Coins, sizeof(double));
	turnToASCII(&dataToWrite[194], tempNumberCoins, sizeof(double));

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
		turnToChar(My_Id, filedata + 128 + 1, 32);

		char tempNumberCoins[sizeof(double)];
		turnToChar(tempNumberCoins, filedata + 128 + 64 + 2, sizeof(double));

		//convert the char array into double
		memcpy(&Number_Coins, tempNumberCoins, sizeof(double));

		//sets the public and private keys as the ones read from the file
		setKey((unsigned char*) My_Id, (unsigned char*) My_Private_Key);

		//close the handle
		CloseHandle(readFile);
	}
	else
	{
		//create the file and initialize it
		//initialize the required variables for the database file
		CloseHandle(createFile);
		createKeys((unsigned char*)My_Id, (unsigned char*)My_Private_Key);
		Number_Coins = 0;

		//call for the function that updates the database file with the values of the
		if (!loadIntoFile())
			return false;
	}
	return true;
}

bool initValues(wstring username)
{
	//initializes the keys and how much money the user has
	if (!loadFromFile(username))
		return false;

	//sends a message to the bootnode to get the node's ip and port outside the NAT
	Connect_0* message = new Connect_0{};
	Handle_Connect_Create(message);
	if (!sendMessage((char*)message, sizeof(Connect_0), (char*)Bootnode_Details[0].nodeIp.c_str(), (int)Bootnode_Details[0].nodePort))
	{
		cout << "error sending message to bootnode while initializing " << '\n';
		return false;
	}

	int t;
	vector <char> mess;
	while (true)
	{
		cin >> t;
		if (t == -1)
			break;
		message = {};
		receiveMessage(&mess);
		handleMessage(mess.data(), (int) mess.size());
	}
	return true;
}
