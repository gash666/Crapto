#include <iostream>
#include <vector>
#include "H_Variables.h"
#include "H_Constants.h"
#include "H_Node_Supporter.h"
#include "H_ECDSA.h"
#include "H_Message_Structure.h"
#include "H_Network_Interface.h"
#include "H_Network_Operations.h"
#include "H_General_Functions.h"
#include "H_Maintain_Blockchain.h"
#include "H_Treap.h"
#include "H_Block_Tree.h"
#include <iomanip>

using namespace std;

void Handle_Connect_Process(char* message, int len)
{
	//answers the request to get the ip and port
	//checks if the message is of the correct size
	if (!Is_Bootnode or len != sizeof(Connect))
		return;

	//checks that the signature on the message is correct
	Connect* m = (Connect*) message;
	bool isGood = verifySignature((const unsigned char*)m, offsetof(Connect, signature), (const unsigned char*)&(m->nodeId), (const unsigned char*)&(m->signature));
	if (!isGood)
	{
		cout << "Process_Message.cpp Handle_Connect_Process error signature is not correct" << '\n';
		return;
	}

	//creates an answer to the request
	Answer_Connect* returnMessage = new Answer_Connect{};
	Handle_Answer_Connect_Create(m->nodeId, returnMessage);
	pair <string, int> address = getAddress();

	Update_Ping_Timer(&returnMessage->answerIdentity);

	//sends the answer message
	sendMessage((char*)returnMessage, sizeof(Answer_Connect), (char*)address.first.c_str(), address.second, true);
}

void Handle_Answer_Connect_Process(char* message, int len)
{
	//handles the answer to a connect message that returns the port and ip
	if (len != sizeof(Answer_Connect))
		return;

	//check if the signature is ok
	Answer_Connect* m = (Answer_Connect*) message;
	bool isGood = verifySignature((const unsigned char*)m, offsetof(Answer_Connect, signature), (const unsigned char*)&(m->senderDetails.nodeID), (const unsigned char*)&(m->signature));
	if (!isGood)
	{
		cout << "Process_Message.cpp Handle_Answer_Connect_Process error signature is not correct" << '\n';
		return;
	}

	//check if the message is from one of the bootnodes
	bool isReal = false;
	for (int a = 0; a < Number_Of_Bootnodes; a++)
		if (memcmp(&Bootnode_Details[a], &m->senderDetails, sizeof(NodeDetails)) == 0)
			isReal = true;
	if (!isReal)
		return;

	//check that the destination is the user
	if (memcmp(&My_Details, &m->answerIdentity, sizeof(NodeDetails)) != 0)
		return;

	Update_Ping_Timer(&m->senderDetails);

	//updates the ip and port
	copy(begin(m->answerIdentity.ip), end(m->answerIdentity.ip), My_Details.ip);
	My_Details.port = m->answerIdentity.port;
}

void Handle_Ask_Close_Process(char* message, int len)
{
	//answer the request to get the closest to some id
	if (len != sizeof(Ask_Close))
		return;

	//check if the signature is ok
	Ask_Close* m = (Ask_Close*) message;
	bool isGood = verifySignature((const unsigned char*)m, offsetof(Ask_Close, signature), (const unsigned char*)&(m->senderDetails.nodeID), (const unsigned char*)&(m->signature));
	if (!isGood)
	{
		cout << "Process_Message.cpp Handle_Ask_Close_Process error signature is not correct" << '\n';
		return;
	}

	Update_Ping_Timer(&m->senderDetails);

	//sends an answer to the message
	Answer_Close* returnMessage = new Answer_Close{};
	Handle_Answer_Close_Create(m->target, returnMessage);
	sendMessage((char*)returnMessage, sizeof(Answer_Close), m->senderDetails.ip, m->senderDetails.port);
}

void Handle_Answer_Close_Process(char* message, int len)
{
	//answer the request to get the closest to some id
	if (len != sizeof(Answer_Close))
		return;

	//check if the signature is ok
	Answer_Close* m = (Answer_Close*) message;
	bool isGood = verifySignature((const unsigned char*)m, offsetof(Answer_Close, signature), (const unsigned char*)&(m->senderDetails.nodeID), (const unsigned char*)&(m->signature));
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
		if (commWithThreadsDetails[a].threadMessageId == ANSWER_CLOSE and memcmp(commWithThreadsDetails[a].whereToCompare, m->target, 32) == 0)
			copy(&commWithThreadsDetails[a], (Communication_With_Threads*)((char*)&commWithThreadsDetails[a] + sizeof(Communication_With_Threads)), &threadDetails);

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

void Handle_Ask_Ping_Process(char* message, int len)
{
	//answer the ping
	if (len != sizeof(Ask_Ping))
		return;

	//check if the signature is ok
	Ask_Ping* m = (Ask_Ping*) message;
	bool isGood = verifySignature((const unsigned char*)m, offsetof(Ask_Ping, signature), (const unsigned char*)&(m->senderDetails.nodeID), (const unsigned char*)&(m->signature));
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
	Answer_Ping* returnMessage = new Answer_Ping{};
	Handle_Answer_Ping_Create(&m->senderDetails, returnMessage);
	sendMessage((char*) returnMessage, sizeof(Answer_Ping), returnMessage->receiverDetails.ip, returnMessage->receiverDetails.port);
}

void Handle_Answer_Ping_Process(char* message, int len)
{
	//update that the node is active
	if (len != sizeof(Answer_Ping))
		return;

	//check if the signature is ok
	Answer_Ping* m = (Answer_Ping*) message;
	bool isGood = verifySignature((const unsigned char*)m, offsetof(Answer_Ping, signature), (const unsigned char*)&(m->senderDetails.nodeID), (const unsigned char*)&(m->signature));
	if (!isGood)
	{
		cout << "Process_Message.cpp Handle_Answer_Ping_Process error signature is not correct" << '\n';
		return;
	}
	Update_Ping_Timer(&m->senderDetails);
}

void Handle_Pay_Process(char* message, int len)
{
	//process a payment
	if (len != sizeof(Pay))
		return;

	//check if the payment is ok
	Pay* m = (Pay*)message;

	//check if the signature is ok
	bool isGood = verifySignature((const unsigned char*)m, offsetof(Pay, signature), (const unsigned char*)&(m->senderDetails.nodeID), (const unsigned char*)&(m->signature));
	if (!isGood)
	{
		cout << "Process_Message.cpp Handle_Pay_Process error signature is not correct" << '\n';
		return;
	}

	//check if the payment is not familiar to the user already
	if (isAlreadyIn(message, 0))
		return;

	//add the message to the messages known to the user
	addMessageInd(message, Get_Time(), 0);

	//spread the message to other users
	spreadMessage(message, len);
}

void Handle_Block_Process(char* message, int len)
{
	//receives a block and processes it
	Block* m = (Block*) message;

	//check that the number of payments is ok
	if (m->HowmanyFromEachType[2] > Max_Number_Payments_Block)
		return;

	//check that the number of contracts that bind random staking pool operators is ok
	if (m->HowmanyFromEachType[3] > Max_Number_Bind_Random_Staking_Pool_Operator_Block)
		return;

	//check that the number of contracts that bind staking pool operators is ok
	if (m->HowmanyFromEachType[4] > Max_Number_Bind_Staking_Pool_Operator_Block)
		return;

	//check if the length of the proposed block is ok
	if (len < sizeof(Block) or len != sizeof(Block) + 
		m->HowmanyFromEachType[0] * sizeof(Random_Reveal) +
		m->HowmanyFromEachType[1] * sizeof(NodeDetails) +
		m->HowmanyFromEachType[2] * sizeof(Transaction) +
		m->HowmanyFromEachType[3] * sizeof(Contract_Random) +
		m->HowmanyFromEachType[4] * sizeof(Contract) + 64)
		return;

	//check if the time of the block is in the future
	if (Get_Time() < m->TimeAtCreation)
		return;

	//check if the block is familiar to the user
	char SHAOfBlock[32];
	SHA256(message, len, SHAOfBlock);
	if (getBlock(SHAOfBlock).first != NULL)
		return;

	if (!hasInfo)
	{
		//check if the message is familiar to this user
		if (isAlreadyIn(SHAOfBlock, 6))
			return;

		//spread the message if not familiar
		addMessageInd(SHAOfBlock, Get_Time(), 6);
		spreadMessage(message, len);
		return;
	}

	bool isHeadInitializing = false;
	if (memcmp(ShaOfHeadBlockInitializing, SHAOfBlock, 32) == 0)
		isHeadInitializing = true;

	//check if the parent of the proposed block is familiar
	pair <char*, int> blockParent = getBlock(m->SHA256OfParent);
	if (!isHeadInitializing and blockParent.first == NULL)
		return;

	//check the block number
	if (!isHeadInitializing and m->BlockNumber != ((Block*)blockParent.first)->BlockNumber + 1)
		return;

	//check if the time of the proposed block is ok
	if (!isHeadInitializing and (m->TimeAtCreation % Time_Block != 0 or m->TimeAtCreation <= ((Block*)blockParent.first)->TimeAtCreation))
		return;

	//check if the signature is ok
	bool isGood = verifySignature((const unsigned char*)m, len - 64, (const unsigned char*)&(m->BlockCreator.nodeID), (const unsigned char*)(message + len - 64));
	if (!isGood)
	{
		cout << "Process_Message.cpp Handle_Block_Process error signature is not correct" << '\n';
		return;
	}

	//if nodes that asked for help might not know about this block, send it to them
	if (Is_Bootnode)
	{
		//removes users that don't need help anymore
		unsigned long long timeNow = Get_Time();
		while (!May_Need_Information.empty() and May_Need_Information.front().second < timeNow)
			May_Need_Information.pop_front();

		//sends this block to all users that might have missed it
		for (auto a : May_Need_Information)
			sendMessage(message, len, a.first.ip, a.first.port);
	}

	//if the user is not a staking pool operator, act accordingly
	if (!Is_Staking_Pool_Operator)
	{
		addBlock(m->SHA256OfParent, SHAOfBlock, message, len, true);
		spreadMessage(message, len);
		return;
	}

	//simulate the state before this block
	lock_guard <mutex> lock1(canUseBlockTreeActions);

	//save the block
	addBlock(m->SHA256OfParent, SHAOfBlock, message, len, true);

	//apply the blocks before it
	applyBlockFake(m->SHA256OfParent);

	//removes staking pool operators that their contract has ended
	unsigned long long temp = Get_Time();
	temp -= temp % Time_Block + Time_Block;
	deleteAllNodeIndPlace(1, 0, temp);
	deleteAllNodeIndPlace(3, 0, temp);

	//initialize pointer
	char* tempPointer = message + sizeof(Block);

	//initialize variable
	char xorOfAll[32]{ 0 };

	//calculate the xor of all random values in the block
	for (int a = 0; a < m->HowmanyFromEachType[0]; a++, tempPointer += sizeof(Random_Reveal))
		for (int b = 0; b < 32; b++)
			xorOfAll[b] ^= tempPointer[sizeof(NodeDetails) + b];

	//get the real block creator according to this random numbers
	NodeDetails realCreator;
	getNextBlockCreator(xorOfAll, &realCreator);

	//check if the current block is proposed by the real creator
	if (memcmp(&realCreator, &m->BlockCreator, sizeof(NodeDetails)) != 0)
	{
		reversePath(m->SHA256OfParent);
		return;
	}

	//reverse the pointer back to his original state
	tempPointer -= m->HowmanyFromEachType[0] * sizeof(Random_Reveal);

	if (getSizeTreapInd(3) != m->HowmanyFromEachType[0] + m->HowmanyFromEachType[1])
		return;

	//check if the random values that appear in the block are valid
	for (int a = 0; a < m->HowmanyFromEachType[0]; a++, tempPointer += sizeof(Random_Reveal))
	{
		checkRandomReveal((NodeDetails*)tempPointer, tempPointer + sizeof(NodeDetails), m->TimeAtCreation - Time_Block);
		char randomValIn[32];
		unsigned long long tempVal = getVariableIndPlace((NodeDetails*)tempPointer, 3, 2, randomValIn, 32);
		if (tempVal == 0 or memcmp(randomValIn, tempPointer + sizeof(NodeDetails), 32) != 0)
		{
			reversePath(m->SHA256OfParent);
			return;
		}

		//check if this was already revealed in this block
		if (isAlreadyIn(((NodeDetails*)tempPointer)->nodeID, 1))
		{
			reversePath(m->SHA256OfParent);
			return;
		}

		//adds the pay message to the queue and map
		addMessageInd(((NodeDetails*)tempPointer)->nodeID, Get_Time(), 1);
	}

	//remove the staking pool operators that revealed from the queue and map
	reset(1);

	//check if the new amount of money of the staking pool operator is correct
	if (getAmountOfMoneyInd(&m->BlockCreator, 1) + Number_Coins_Per_Block != m->newAmountCreator)
	{
		reversePath(m->SHA256OfParent);
		return;
	}

	//set the new amount of money of the block creator
	setAmountMoneyInd(&m->BlockCreator, m->newAmountCreator, 1);

	//check if the random staking pool operators that didn't reveal didn't sent their values on time
	for (int a = 0; a < m->HowmanyFromEachType[1]; a++, tempPointer += sizeof(NodeDetails))
	{
		unsigned long long TimeUpdate = getVariableIndPlace((NodeDetails*)tempPointer, 3, 2);
		unsigned long long TimeReceived = getVariableIndPlace((NodeDetails*)tempPointer, 3, 2);
		if (TimeUpdate == m->TimeAtCreation - Time_Block and TimeReceived <= m->TimeAtCreation - Time_Block / 3)
		{
			ReverseBlockUntil(message, len, { 1, a });
			reversePath(m->SHA256OfParent);
			return;
		}

		//check if this random staking pool operator was already punished in this block
		if (isAlreadyIn(((NodeDetails*)tempPointer)->nodeID, 1))
		{
			ReverseBlockUntil(message, len, { 1, a });
			reversePath(m->SHA256OfParent);
			return;
		}

		//adds the pay message to the queue and map
		addMessageInd(((NodeDetails*)tempPointer)->nodeID, Get_Time(), 1);

		//update to the new amount of money
		unsigned long long amountOfMoney = getAmountOfMoneyInd((NodeDetails*)tempPointer, 1) - Punishment_Not_Reveal;

		//removes the punished random staking pool operator from their treap if needed
		if (amountOfMoney < Min_Coins_Random_Staking_Pool_Operator)
			removeNode((NodeDetails*)tempPointer, 3);

		//set the new amount of money for the random staking pool operator
		setAmountMoneyInd((NodeDetails*)tempPointer, amountOfMoney, 1);
	}

	//remove the punished staking pool operators from the queue and map
	reset(1);

	//check the payments in the block
	for (int a = 0; a < m->HowmanyFromEachType[2]; a++, tempPointer += sizeof(Transaction))
		if (checkPayment((Transaction*) tempPointer, m->TimeAtCreation) != 2)
		{
			//the block is wrong - reverse the actions taken so far
			ReverseBlockUntil(message, len, { 2, a } );
			reversePath(m->SHA256OfParent);
			return;
		}

	//remove the payments from the queue and map
	reset(1);
	
	//check the Bind_Random_Staking_Pool_Operator in the block
	for (int a = 0; a < m->HowmanyFromEachType[3]; a++, tempPointer += sizeof(Contract_Random))
	{
		if (checkBindRandomStakingPoolOperator((Contract_Random*)tempPointer, m->TimeAtCreation) != 2)
		{
			//the block is wrong - reverse the actions taken so far
			ReverseBlockUntil(message, len, { -1, -1 });
			reversePath(m->SHA256OfParent);
			return;
		}
	}

	//check the Bind_Staking_Pool_Operator in the block
	for (int a = 0; a < m->HowmanyFromEachType[4]; a++, tempPointer += sizeof(Contract))
		if (checkBindStakingPoolOperator((Contract*) tempPointer, m->TimeAtCreation) != 2)
		{
			//the block is wrong - reverse the actions taken so far
			ReverseBlockUntil(message, len, { -1, -1 });
			reversePath(m->SHA256OfParent);
			return;
		}

	//sends a confirmation message or starts the confirmation process if needed
	shouldSignBlock(SHAOfBlock);

	//revese the actions done while creating this block
	ReverseBlockUntil(message, len, { -1, -1 });
	reversePath(m->SHA256OfParent);

	//spread the message
	spreadMessage(message, len);
}

void Handle_Bind_Random_Staking_Pool_Operator_Process(char* message, int len)
{
	//processes a message that binds a user to be a random staking pool operator
	if (len != sizeof(Bind_Random_Staking_Pool_Operator))
		return;

	//check if the Bind_Random_Staking_Pool_Operator is ok
	Bind_Random_Staking_Pool_Operator* m = (Bind_Random_Staking_Pool_Operator*) message;

	//check if the signature is ok
	if (!verifySignature((const unsigned char*)m, offsetof(Bind_Random_Staking_Pool_Operator, signature), (const unsigned char*)&(m->newStakingPoolOperator.nodeID), (const unsigned char*)&(m->signature)))
	{
		cout << "Process_Message.cpp Handle_Bind_Random_Staking_Pool_Operator_Process error signature is not correct" << '\n';
		return;
	}

	//check if the times are ok
	if ((m->startTime < Get_Time() and !isFirstAll) or m->startTime > m->untilTime or m->startTime % Time_Block != 0 or m->untilTime % Time_Block != 0)
		return;

	//checks if the message is familiar to this user
	if (isAlreadyIn(message, 2))
		return;

	//add the message to the messages known to the user
	addMessageInd(message, Get_Time(), 2);

	//spread the message
	spreadMessage(message, len);
}

void Handle_Bind_Staking_Pool_Operator_Process(char* message, int len)
{
	//processes a message that binds a user to be a random staking pool operator
	if (len != sizeof(Bind_Staking_Pool_Operator))
		return;

	//check if the Bind_Random_Staking_Pool_Operator is ok
	Bind_Staking_Pool_Operator* m = (Bind_Staking_Pool_Operator*)message;

	//check if the signature is ok
	if (!verifySignature((const unsigned char*)m, offsetof(Bind_Staking_Pool_Operator, signature), (const unsigned char*)&(m->newStakingPoolOperator.nodeID), (const unsigned char*)&(m->signature)))
	{
		cout << "Process_Message.cpp Handle_Bind_Staking_Pool_Operator_Process error signature is not correct" << '\n';
		return;
	}

	//check if the times are ok
	if (m->startTime < Get_Time() or m->startTime > m->untilTime or m->startTime % Time_Block != 0 or m->untilTime % Time_Block != 0)
		return;

	//checks if the message is familiar to this user
	if (isAlreadyIn(message, 3))
		return;

	//add the message to the messages known to the user
	addMessageInd(message, Get_Time(), 3);

	//spread the message
	spreadMessage(message, len);
}

void Handle_Reveal_Process(char* message, int len)
{
	//processes a message that contains a revealed random number
	if (len != sizeof(Reveal))
		return;

	//check if the signature is ok
	Reveal* m = (Reveal*) message;
	bool isGood = verifySignature((const unsigned char*)m, offsetof(Reveal, signature), (const unsigned char*)&(m->senderDetails.nodeID), (const unsigned char*)&(m->signature));
	if (!isGood)
	{
		cout << "Process_Message.cpp Handle_Reveal_Process error signature is not correct" << '\n';
		return;
	}

	//check if the time is divisible by the block times
	if (m->timeWhenSend % Time_Block != 0)
		return;

	//check if the time has come to reveal this random number
	if (m->timeWhenSend + Time_Block / 3 > Get_Time())
		return;

	//check if this message is familiar to this user
	if (isAlreadyIn(message, 4))
		return;

	//check if the revealed value is correct and apply it
	if (Is_Staking_Pool_Operator and !checkRandomReveal(&m->senderDetails, m->randomValue, m->timeWhenSend, m->HashOfContract))
	{
		cout << "problem with receiving random number" << '\n';
		return;
	}

	//add the message to the messages known to the user
	addMessageInd(message, Get_Time(), 4);

	//spread the message to other users
	spreadMessage(message, len);
}

void Handle_Confirm_Block_Process(char* message, int len)
{
	//processes a message that confirms the current proposed block by one user
	if (len != sizeof(Confirm_Block))
		return;

	//check if the user is creating a block
	if (!isThisUserCreating)
		return;

	//check if the signature is ok
	Confirm_Block* m = (Confirm_Block*)message;
	bool isGood = verifySignature((const unsigned char*)m, offsetof(Confirm_Block, signature), (const unsigned char*)&(m->senderDetails.nodeID), (const unsigned char*)&(m->signature));
	if (!isGood)
	{
		cout << "Process_Message.cpp Handle_Confirm_Block_Process error signature is not correct" << '\n';
		return;
	}

	//check if the user has enough money to sign on the block
	if (!isFirstAll and getAmountOfMoneyInd(&m->senderDetails, 1) < Min_Coins_Staking_Pool_Operator)
		return;

	//check if the signature on the created block is ok
	if (!verifySignature((const unsigned char*)hashBlockCreating, 32, (const unsigned char*)&(m->senderDetails.nodeID), (const unsigned char*)&(m->BlockSignature)))
		return;

	//check if the user signed on the block already
	if (isInTreapInd(&m->senderDetails, 5))
		return;

	//save that this user has signed already
	addToTree(&m->senderDetails, 0, 5, 0, NULL);

	//save the signature later to be added to a message confirming the block
	array <char, 64> tempSignature;
	copy(m->BlockSignature, m->BlockSignature + 64, tempSignature.begin());
	Signatures_For_Confirm_Message.push_back({ m->senderDetails, tempSignature });
}

bool Handle_Confirm_Block_All_Process(char* message, int len)
{
	//processes a message that confirms that the current proposed block is valid
	Confirm_Block_All* m = (Confirm_Block_All*)message;
	
	//check if the message length is ok
	if (len < sizeof(Confirm_Block_All) or len != sizeof(Confirm_Block_All) + m->numberOfSignatures * (64 + sizeof(NodeDetails)) + 64)
		return false;

	//check if the signature is ok
	bool isGood = verifySignature((const unsigned char*)m, sizeof(Confirm_Block_All) + m->numberOfSignatures * (64 + sizeof(NodeDetails)), (const unsigned char*)&(m->senderDetails.nodeID), (const unsigned char*)(message + sizeof(Confirm_Block_All) + m->numberOfSignatures * (64 + sizeof(NodeDetails))));
	if (!isGood)
	{
		cout << "Process_Message.cpp Handle_Confirm_Block_All_Process error signature is not correct" << '\n';
		return false;
	}

	//check if the time is ok
	if (m->messageNumber / 100000 > Get_Time())
		return false;

	//check if this message is familiar to this user
	char shaOfMessage[32];
	SHA256(message, len, shaOfMessage);
	if (isAlreadyIn(shaOfMessage, 5))
		return false;

	if (!hasInfo)
	{
		//spread the message if not familiar
		addMessageInd(shaOfMessage, Get_Time(), 5);
		spreadMessage(message, len);
		return false;
	}

	//check if the block exists
	pair <char*, int> blockDetails = getBlock(m->BlockHash);
	if (blockDetails.first == NULL)
		return false;

	//check if the sender is the block creator that sent the approved block
	if (memcmp((char*)&((Block*)blockDetails.first)->BlockCreator, (char*)&m->senderDetails, sizeof(NodeDetails)) != 0)
		return false;

	//check if the block that is tried to be confirmed is the head
	char shaOfHead[32];
	setShaOfHead(shaOfHead);
	if (memcmp(m->BlockHash, shaOfHead, 32) == 0)
		return false;

	//make sure checking this will not cause errors
	lock_guard <mutex> lock1(canUseBlockTreeActions);

	//initialize treaps
	initializeTreapsAll();

	//get the blocks on the path from this block to the root
	pair <char*, int> path = getPathToNode(m->BlockHash);

	for (int b = path.second - 1; b >= 1; b--)
	{
		//initialize variables
		char* tempPointer = message + sizeof(Confirm_Block_All);
		unsigned long long NumberCoinsSignedBlock = 0;
		BlockTreeNode* blockNow = *(BlockTreeNode**)(path.first + b * sizeof(BlockTreeNode*));
		Block* blockPointer = (Block*)(blockNow->startOfBlock);

		//apply the current block
		applyOneBlock(blockNow->startOfBlock, blockNow->sizeOfBlock);

		for (unsigned int a = 0; a < m->numberOfSignatures; a++)
		{
			unsigned long long tempAnswer = getVariableIndPlace((NodeDetails*)tempPointer, 1, 1);

			//check if the user didn't sign already
			if (tempAnswer == blockPointer->BlockNumber)
			{
				reversePath(blockNow->sha256OfBlock);
				return false;
			}
			setVariableIndPlace((NodeDetails*)tempPointer, 1, 1, blockPointer->BlockNumber);

			//check if his signature is correct
			if (!verifySignature((const unsigned char*)m->BlockHash, 32, (const unsigned char*)((NodeDetails*)tempPointer)->nodeID, (const unsigned char*)(tempPointer + sizeof(NodeDetails))))
			{
				reversePath(blockNow->sha256OfBlock);
				return false;
			}
			//set that the user signed this block
			NumberCoinsSignedBlock += askNumberOfCoins((NodeDetails*)tempPointer, 1);

			//move the pointer
			tempPointer += sizeof(NodeDetails) + 64;
		}

		//check if the sum of staked coins that signed this block is enough
		if (((Block*)blockNow->startOfBlock)->BlockNumber != 10000 and NumberCoinsSignedBlock <= getSumCoins(1) / 2)
		{
			reversePath(blockNow->sha256OfBlock);
			return false;
		}
	}

	//reverse the simulation of the blockchain
	reversePath(m->BlockHash, false);
	
	//add the message to the messages known to the user
	addMessageInd(shaOfMessage, m->messageNumber / 100000, 5);

	//apply the blocks on the path to the confirmed block
	applyBlockReal(m->BlockHash);

	//spread the message
	spreadMessage(message, len);

	return true;
}

void Handle_Ask_All_Info(char* message, int len)
{
	//check if the message length is ok
	if (sizeof(Ask_All_Info) != len)
		return;

	Ask_All_Info* m = (Ask_All_Info*)message;

	//check if the signature is ok
	bool isGood = verifySignature((const unsigned char*)m, offsetof(Ask_All_Info, signature), (const unsigned char*)&(m->senderDetails.nodeID), (const unsigned char*)&(m->signature));
	if (!isGood)
	{
		cout << "Process_Message.cpp Handle_Ask_All_Info error signature is not correct" << '\n';
		return;
	}

	//check if the user is a bootnode
	if (!Is_Bootnode)
		return;

	//make sure checking this will not cause errors
	lock_guard <mutex> lock1(canUseBlockTreeActions);
	lock_guard <mutex> lock2(canReceiveRandom);

	//send all the data about the blockchain
	initializeSendAllData(&m->senderDetails);
	if (m->allOrNot)
	{
		copyAllData(1);
		copyInfoAnswer(0);
		copyInfoRandom(4);
	}
	else
	{
		//give the user only data about the amount of money he has
		char tempData[sizeof(NodeDetails) + sizeof(unsigned long long)];
		copy(&m->senderDetails, (NodeDetails*)((char*)&m->senderDetails + sizeof(NodeDetails)), (NodeDetails*)tempData);
		unsigned long long tempAmount = getAmountOfMoneyInd(&m->senderDetails, 1);
		*(unsigned long long*)& tempData[sizeof(NodeDetails)] = tempAmount;
		copyDataToMessage(tempData, sizeof(NodeDetails) + sizeof(unsigned long long), 0);
		copyInfoAnswer(0);
	}
	endSendAllData();

	//send the answer and all the blocks in the block tree
	sendBlockTree(&m->senderDetails);
	May_Need_Information.push_back({ m->senderDetails, Get_Time() + Max_Time_Spread });
}

void Handle_Answer_All_Info_Process(char* message, int len)
{
	//processes a message that sends information about the blockchain from the bootnode to this user
	Answer_All_Info* m = (Answer_All_Info*)message;

	//check if the length is ok
	int sizeToSkip = sizeof(NodeDetails) + sizeof(unsigned long long);
	if (len < sizeof(Answer_All_Info) or len != sizeof(Answer_All_Info) + (int)m->howmanyEachType[0] * sizeToSkip + (int)m->howmanyEachType[1] * sizeof(Info_Blockchain) + (int)m->howmanyEachType[2] * sizeof(Info_Random) + 64)
		return;

	int messageLength = sizeof(Answer_All_Info) + (int)m->howmanyEachType[0] * sizeToSkip + (int)m->howmanyEachType[1] * sizeof(Info_Blockchain) + (int)m->howmanyEachType[2] * sizeof(Info_Random);

	//check if the signature is ok
	bool isGood = verifySignature((const unsigned char*)m, messageLength, (const unsigned char*)&(m->senderDetails.nodeID), (const unsigned char*)(message + messageLength));
	if (!isGood)
	{
		cout << "Process_Message.cpp Handle_Answer_All_Info_Process error signature is not correct" << '\n';
		return;
	}

	//check if the message was meant for this user
	if (memcmp(&My_Details, &m->ReceiverDetails, sizeof(NodeDetails)) != 0)
		return;

	//check if the sender of the message is a bootnode
	bool bootnodeSent = false;
	for (int a = 0; a < Number_Of_Bootnodes; a++)
		if (memcmp(&Bootnode_Details[a], &m->senderDetails, sizeof(NodeDetails)) == 0)
			bootnodeSent = true;
	
	//if the sender is not a bootnode, return
	if (!bootnodeSent)
		return;

	//initialize a pointer to the start of the data
	char* tempPointer = message + sizeof(Answer_All_Info);

	//save the SHA256 of the head block to approve automatically
	copy(m->shaOfHeadBlock, m->shaOfHeadBlock + 32, ShaOfHeadBlockInitializing);

	//set amounts of money for each user
	for (int a = 0; a < m->howmanyEachType[0]; a++, tempPointer += sizeof(Info_Blockchain) - sizeof(unsigned long long))
	{
		Info_Blockchain* infoNow = (Info_Blockchain*)tempPointer;
		addNodeToTreeInd(&infoNow->identity, 1, true);
		setAmountMoneyInd(&infoNow->identity, infoNow->coinsAmount, 1);
	}

	//set staking pool operators
	for (int a = 0; a < m->howmanyEachType[1]; a++, tempPointer += sizeof(Info_Blockchain))
	{
		Info_Blockchain* infoNow = (Info_Blockchain*)tempPointer;
		addToTree(&infoNow->identity, infoNow->coinsAmount, 0, 0, NULL);
		setVariableIndPlace(&infoNow->identity, 0, 0, infoNow->timeEnd);
	}

	//set random staking pool operators
	for (int a = 0; a < m->howmanyEachType[2]; a++, tempPointer += sizeof(Info_Random))
	{
		Info_Random* infoNow = (Info_Random*)tempPointer;
		addToTree(&infoNow->identity, 0, 2, 0, NULL);
		setVariableIndPlace(&infoNow->identity, 2, 0, getVariableIndPlace(&infoNow->identity, 0, 0));
		setVariableIndPlace(&infoNow->identity, 2, 1, infoNow->timeLastReveal, infoNow->lastRandomRevealed, 32);

		addToTree(&infoNow->identity, 0, 4, 32, infoNow->shaOfContract);
		setVariableIndPlace(DataCompare(&infoNow->identity, 32, infoNow->shaOfContract), 4, 0, getVariableIndPlace(&infoNow->identity, 0, 0));
		setVariableIndPlace(DataCompare(&infoNow->identity, 32, infoNow->shaOfContract), 4, 1, infoNow->timeLastReveal, infoNow->lastRandomRevealed, 32);
	}

	//if all the information has arrived, set the amount of money of this user
	if (m->isLast)
	{
		Number_Coins = getAmountOfMoneyInd(&My_Details, 1);
		loadIntoFile();
		hasInfo = true;

		//send a cotract to become a staking pool operator if needed
		if (hasContract)
		{
			sendMessageStakingPoolOperator(false);
			Is_Staking_Pool_Operator = true;
		}
	}
}

void handleMessage(char* message, int len)
{
	//receives a message and reacts to it accordingly
	//check if the message is too short or too long
	if (len > Maximum_Message_Size or len < Minimum_Message_Size)
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
	else if (message[0] == PAY)
		Handle_Pay_Process(message, len);
	else if (message[0] == BLOCK)
		Handle_Block_Process(message, len);
	else if (message[0] == BIND_RANDOM_STAKING_POOL_OPERATOR)
		Handle_Bind_Random_Staking_Pool_Operator_Process(message, len);
	else if (message[0] == BIND_STAKING_POOL_OPERATOR)
		Handle_Bind_Staking_Pool_Operator_Process(message, len);
	else if (message[0] == REVEAL)
		Handle_Reveal_Process(message, len);
	else if (message[0] == CONFIRM_BLOCK)
		Handle_Confirm_Block_Process(message, len);
	else if (message[0] == CONFIRM_BLOCK_ALL)
		Handle_Confirm_Block_All_Process(message, len);
	else if (message[0] == ASK_ALL_INFO)
		Handle_Ask_All_Info(message, len);
	else if (message[0] == ANSWER_ALL_INFO)
		Handle_Answer_All_Info_Process(message, len);
}
