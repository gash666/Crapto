#include "H_Variables.h"
#include "H_Constants.h"
#include "H_Message_Structure.h"
#include "H_General_Functions.h"
#include "H_ECDSA.h"
#include "H_Network_Operations.h"
#include "H_Network_Interface.h"
#include "H_Treap.h"

using namespace std;

void paySomeone()
{
	//function that pays someone
	string inp;
	cout << "Enter the user's id: " << '\n';
	cin >> inp;

	unsigned long long amountToPayUser;
	cout << "Enter the amount to pay: " << '\n';
	cin >> amountToPayUser;

	if (Number_Coins < amountToPayUser)
	{
		cout << "You don't have enough money!" << '\n';
		return;
	}

	//checks if the input's length is ok
	if (inp.length() != 64)
	{
		cout << "Wrong id length" << '\n';
		return;
	}

	//turns the input into an id
	char target[32];
	turnToChar(target, (char*) inp.c_str(), 32);

	//checks if the user knows this node already
	NodeDetails receiver[1];
	fillListInd(1, target, receiver, 0);
	bool knowsHim = false;
	for (int a = 0; a < 32 and !knowsHim; a++)
		if (receiver->nodeID[a] != 0)
			knowsHim = true;

	//gets the details of the node
	getClosest(target, 3, &receiver[0]);

	//checks if the node was found
	if (memcmp(receiver[0].nodeID, target, 32) != 0)
	{
		cout << "The user you are trying to pay is not online" << '\n';
		return;
	}
	
	//create the message that pays the user
	Pay* message = new Pay{};
	Handle_Pay_Create(&(receiver[0]), amountToPayUser, message);

	//send the transaction to the payment receiver
	sendMessage((char*) message, sizeof(Pay), receiver[0].ip, receiver[0].port);

	//spread the message to other nodes
	handleMessage((char*)message, sizeof(Pay));
}

void printId()
{
	//function that prints the user id
	//turns the id to hex
	char answer[64];
	turnToASCII(answer, My_Details.nodeID, 32);

	//prints the id
	cout << "Your id is: ";
	for (int a = 0; a < 64; a++)
		cout << answer[a];
	cout << '\n';
}

void printMoney()
{
	//prints the amount of money this user has
	cout << "You have " << Number_Coins / Number_Coins_Per_Block << ".";

	//calculate the amount of digits
	int temp = Number_Coins % Number_Coins_Per_Block;
	int numberDigits = 0;
	if (temp == 0)
		numberDigits = 1;
	while (temp != 0)
	{
		numberDigits++;
		temp /= 10;
	}

	//print zeros as needed
	for (int a = numberDigits; a < 9; a++)
		cout << "0";

	cout << Number_Coins % Number_Coins_Per_Block << " coins" << '\n';
}

void printBlockNumberApproved()
{
	//print the current block number
	cout << "The current block number is " << blockNumberApproved << '\n';
}

unsigned long long NumberMinutesStakingPoolOperator = Number_Minutes_Bootnode_Active;

void sendMessageStakingPoolOperator(bool isToNow)
{
	//sends the messages of the contracts of staking pool operator
	//calculate the start and end time of the contract
	unsigned long long timeEnd = Get_Time();
	unsigned long long timeStart;
	timeEnd -= timeEnd % Time_Block;
	if (!isToNow)
		timeStart = timeEnd + Time_Block * 2;
	else
		timeStart = timeEnd;
	timeEnd = timeStart + NumberMinutesStakingPoolOperator * 60000;

	if (isCreatingRandom)
	{
		//the user wants to be a part of the creation of the random numbers
		//initializing variables
		revealNumbers = {};
		revealNumbers.emplace_back();
		fillRandom((unsigned char*)&revealNumbers[0][0], 32);

		//creates the layers of random numbers
		for (int a = 0; a < NumberMinutesStakingPoolOperator * 3; a++)
		{
			revealNumbers.emplace_back();
			SHA256(&revealNumbers[a][0], 32, &revealNumbers[a + 1][0]);
		}

		//initializing the variable that saves the next index to be revealed
		indexLastReveal = (int)revealNumbers.size() - 2;

		//create and spread the message
		Bind_Random_Staking_Pool_Operator* message = new Bind_Random_Staking_Pool_Operator{};
		Handle_Bind_Random_Staking_Pool_Operator_Create(&revealNumbers.back()[0], timeStart, timeEnd, message);
		handleMessage((char*)message, sizeof(Bind_Random_Staking_Pool_Operator));
		char hashContract[32];
		SHA256((char*) message, sizeof(Bind_Random_Staking_Pool_Operator), hashContract);
		post(ThreadPool, [timeStart, timeEnd, hashContract]() { spreadRandomNumbers(timeStart, timeEnd, (char*)hashContract); });
	}
	else
	{
		//the user doesn't want to be a part of the creation of the random numbers
		Bind_Staking_Pool_Operator* message = new Bind_Staking_Pool_Operator{};
		Handle_Bind_Staking_Pool_Operator_Create(timeStart, timeEnd, message);
		handleMessage((char*)message, sizeof(Bind_Staking_Pool_Operator));
		post(ThreadPool, [timeStart, timeEnd]() { handleStakingPoolOperator(timeStart, timeEnd); });
	}
}

void askInformation(bool isAll)
{
	//sends a message asking for all the information about the blockchain
	//delete the known data in the treap of staking pool operators
	deleteTreapAll(0);
	deleteTreapAll(2);
	deleteTreapAll(4);

	//create and send a message asking for information
	Ask_All_Info* message = new Ask_All_Info{};
	Handle_Ask_All_Info_Create(message, isAll);

	//make sure this user is not asking himself for the info
	int randomNumber = rand() % Number_Of_Bootnodes;
	while (memcmp(&Bootnode_Details[randomNumber], &My_Details, sizeof(NodeDetails)) == 0)
		randomNumber = rand() % Number_Of_Bootnodes;

	sendMessage((char*)message, sizeof(Ask_All_Info), Bootnode_Details[randomNumber].ip, Bootnode_Details[randomNumber].port);
}

void openStakingPool(bool isRandom)
{
	//makes this user become a staking pool operator
	//check if the user has an ongoing contract
	if (hasContract)
	{
		cout << "You already have an ongoing contract" << '\n';
		return;
	}

	//check if the user has enough money to become a staking pool operator
	if (Number_Coins < Min_Coins_Staking_Pool_Operator)
	{
		cout << "You don't have enough coins in order to become a staking pool operator" << '\n';
		return;
	}

	cout << "howmany minutes would you like to be a staking pool operator for? " << '\n';
	cin >> NumberMinutesStakingPoolOperator;

	//saves if the user is a staking pool operator that creates random numbers or not
	isCreatingRandom = isRandom;

	//saves that this user is already a staking pool operator
	hasContract = true;

	askInformation(true);
}

void printDataStakingPoolOperators()
{
	//makes sure the details will not change during the printing
	lock_guard <mutex> lock(canUseBlockTreeActions);

	//prints the answer
	cout << "There are currently " << getSizeTreapInd(0) << " staking pool operators" << '\n';
	printDataTreap();
}

void estimateAmount()
{
	//prints an estimation for the number of online users
	cout << "There are approximatly " << estimateUserAmount() << " users online right now" << '\n';
}

void processCommands()
{
	//gets commands from the user and executes them
	string command;
	while (true)
	{
		cin >> command;
		if (command == "PRINT_ID")
			printId();
		else if (command == "PAY")
			paySomeone();
		else if (command == "PRINT_MONEY")
			printMoney();
		else if (command == "OPEN_STAKING_POOL_RAND")
			openStakingPool(true);
		else if (command == "OPEN_STAKING_POOL")
			openStakingPool(false);
		else if (command == "ASK_INFO")
			askInformation(false);
		else if (command == "PRINT_BLOCK_NUMBER")
			printBlockNumberApproved();
		else if (command == "PRINT_STAKED")
			printDataStakingPoolOperators();
		else if (command == "USER_COUNT")
			estimateAmount();
	}
}
