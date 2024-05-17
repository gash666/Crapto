#include "H_Variables.h"
#include "H_General_Functions.h"
#include "H_Constants.h"
#include "H_Network_Operations.h"
#include "H_Block_Tree.h"

#ifndef SODIUM
#define SODIUM
#define SODIUM_STATIC
#include "sodium.h"
#endif

#include "H_ECDSA.h"
#include "H_Treap.h"
#include <chrono>
#include <mutex>

using namespace std;

void fillRandom(unsigned char* randomValue, int numberBytes)
{
	//generate random data
	randombytes_buf(randomValue, numberBytes);
}

int checkPayment(Transaction* paymentTransaction, unsigned long long time, bool isConfirmed)
{
	//checks if a payment is valid
	//calculate the new amounts after the payment
	addNodeToTreeInd(&paymentTransaction->payMessage.receiverDetails, 1);
	unsigned long long moneySender = getAmountOfMoneyInd(&paymentTransaction->payMessage.senderDetails, 1);
	unsigned long long moneyReceiver = getAmountOfMoneyInd(&paymentTransaction->payMessage.receiverDetails, 1);
	moneySender -= paymentTransaction->payMessage.amountToPay;
	moneyReceiver += paymentTransaction->payMessage.amountToPay;

	//check if the message needs confirmation
	if (!isConfirmed)
	{
		//check if the message id is valid
		if (paymentTransaction->payMessage.messageId != PAY)
			return 0;

		//check that the amount to pay is not 0
		if (paymentTransaction->payMessage.amountToPay == 0)
			return 0;

		//make sure the user is not paying himself
		if (memcmp(&paymentTransaction->payMessage.receiverDetails, &paymentTransaction->payMessage.senderDetails, sizeof(NodeDetails)) == 0)
			return 0;

		//check signature
		bool isGood = verifySignature((const unsigned char*)paymentTransaction, offsetof(Pay, signature), (const unsigned char*)&(paymentTransaction->payMessage.senderDetails.nodeID), (const unsigned char*)&(paymentTransaction->payMessage.signature));
		if (!isGood)
			return 0;

		//check if the time of applying is ok
		if (paymentTransaction->payMessage.timeApply % Time_Block != 0 or paymentTransaction->payMessage.timeApply < time)
			return 0;

		//check if the payment is applied at the right time
		if (paymentTransaction->payMessage.timeApply != time)
			return 1;

		//check if the payment is already in the block
		char shaOfPay[32];
		SHA256((char*)paymentTransaction, sizeof(Pay), shaOfPay);
		if (isAlreadyIn(shaOfPay, 1))
			return 0;

		//check if the one paying has enough money
		if (Is_Staking_Pool_Operator and getAmountOfMoneyInd(&paymentTransaction->payMessage.senderDetails, 1) < paymentTransaction->payMessage.amountToPay)
			return 0;

		//check if the user is a staking pool operator and will have enough money to stay one after this payment
		if (Is_Staking_Pool_Operator and isInTreapInd(&paymentTransaction->payMessage.senderDetails, 1) and getAmountOfMoneyInd(&paymentTransaction->payMessage.senderDetails, 1) < paymentTransaction->payMessage.amountToPay + Min_Coins_Staking_Pool_Operator)
			return 0;

		//check if the user is a random staking pool operator and will have enough money to stay one after this payment
		if (Is_Staking_Pool_Operator and isInTreapInd(&paymentTransaction->payMessage.senderDetails, 3) and getAmountOfMoneyInd(&paymentTransaction->payMessage.senderDetails, 1) < paymentTransaction->payMessage.amountToPay + Min_Coins_Random_Staking_Pool_Operator)
			return 0;

		//check if the new amounts are correct
		if (moneySender != paymentTransaction->newAmountSender or moneyReceiver != paymentTransaction->newAmountReceiver)
			return 0;

		//adds the pay message to the queue and map
		addMessageInd(shaOfPay, Get_Time(), 1);
	}

	//set the new amounts
	setAmountMoneyInd(&paymentTransaction->payMessage.senderDetails, moneySender, 1);
	setAmountMoneyInd(&paymentTransaction->payMessage.receiverDetails, moneyReceiver, 1);

	//check if the sender is a staking pool operator and update the amount of money
	if (isInTreapInd(&paymentTransaction->payMessage.senderDetails, 1))
		updateCoinAmount(&paymentTransaction->payMessage.senderDetails, askNumberOfCoins(&paymentTransaction->payMessage.senderDetails, 1) - paymentTransaction->payMessage.amountToPay, 1);

	//check if the receiver is a staking pool operator and update the amount of money
	if (isInTreapInd(&paymentTransaction->payMessage.receiverDetails, 1))
		updateCoinAmount(&paymentTransaction->payMessage.receiverDetails, askNumberOfCoins(&paymentTransaction->payMessage.receiverDetails, 1) + paymentTransaction->payMessage.amountToPay, 1);

	return 2;
}

int checkBindStakingPoolOperator(Contract* check, unsigned long long time, bool isConfirmed)
{
	//check if the bind of a staking pool operator is ok
	//check if the message needs confirmation
	if (!isConfirmed)
	{
		//check if the message id is valid
		if (check->contract.messageId != BIND_STAKING_POOL_OPERATOR)
			return 0;

		//check if the Bind_Staking_Pool_Operator is ok
		if (!verifySignature((const unsigned char*)check, offsetof(Bind_Staking_Pool_Operator, signature), (const unsigned char*)&(check->contract.newStakingPoolOperator.nodeID), (const unsigned char*)&(check->contract.signature)))
			return 0;

		//check if the user is already a staking pool operator
		if (isInTreapInd(&check->contract.newStakingPoolOperator, 1))
			return 0;

		//check if the time of applying is ok
		if (check->contract.startTime % Time_Block != 0 or check->contract.startTime < time or check->contract.untilTime % Time_Block != 0 or check->contract.startTime >= check->contract.untilTime)
			return 0;

		//check if the payment is applied at the right time
		if (check->contract.startTime != time)
			return 1;

		//check if the user has enough money to become a staking pool operator
		if (Is_Staking_Pool_Operator and getAmountOfMoneyInd(&check->contract.newStakingPoolOperator, 1) < Min_Coins_Staking_Pool_Operator)
			return 0;
	}

	//apply the contract
	addToTree(&check->contract.newStakingPoolOperator, check->amountOfMoney, 1, 0, NULL);
	setVariableIndPlace(&check->contract.newStakingPoolOperator, 1, 0, check->contract.untilTime);
	updateCoinAmount(&check->contract.newStakingPoolOperator, check->amountOfMoney, 1);

	return 2;
}

int checkBindRandomStakingPoolOperator(Contract_Random* check, unsigned long long time, bool isConfirmed)
{
	//check if the Bind_Staking_Pool_Operator is ok
	//check if the user is the first bootnode and the bind is his
	if (isFirstAll and memcmp(&My_Details, &check->contract.newStakingPoolOperator, sizeof(NodeDetails)) == 0)
		isConfirmed = true;

	//check if the message needs confirmation
	if (!isConfirmed)
	{
		//check if the message id is valid
		if (check->contract.messageId != BIND_RANDOM_STAKING_POOL_OPERATOR)
			return 0;

		//check if the Bind_Staking_Pool_Operator is ok
		if (!verifySignature((const unsigned char*)check, offsetof(Bind_Random_Staking_Pool_Operator, signature), (const unsigned char*)&(check->contract.newStakingPoolOperator.nodeID), (const unsigned char*)&(check->contract.signature)))
			return 0;

		//check if the user is already a staking pool operator
		if (isInTreapInd(&check->contract.newStakingPoolOperator, 1))
			return 0;

		//check if the time of applying is ok
		if (check->contract.startTime % Time_Block != 0 or check->contract.startTime < time or check->contract.untilTime % Time_Block != 0 or check->contract.startTime >= check->contract.untilTime)
			return 0;

		//check if the payment is applied at the right time
		if (check->contract.startTime != time)
			return 1;

		//check if the user has enough money to become a staking pool operator
		if (getAmountOfMoneyInd(&check->contract.newStakingPoolOperator, 1) < Min_Coins_Random_Staking_Pool_Operator)
			return 0;

		//check if the amount of money is correct
		if (getAmountOfMoneyInd(&check->contract.newStakingPoolOperator, 1) != check->amountOfMoney)
			return 0;
	}

	//check if the contract is already known to the user
	char shaOfContract[32];
	SHA256((char*)check, sizeof(Bind_Random_Staking_Pool_Operator), shaOfContract);
	DataCompare idThisContract(&check->contract.newStakingPoolOperator, 32, shaOfContract);

	//add the random staking pool operator to the treap of all staking pool operators now
	addToTree(&check->contract.newStakingPoolOperator, check->amountOfMoney, 1, 0, NULL);
	setVariableIndPlace(&check->contract.newStakingPoolOperator, 1, 0, check->contract.untilTime);
	updateCoinAmount(&check->contract.newStakingPoolOperator, check->amountOfMoney, 1);

	if (!Is_Staking_Pool_Operator)
		return 2;

	if (isInTreapInd(idThisContract, 4))
	{
		//add the random staking pool operator to the treap of all random staking pool operators now
		addToTree(&check->contract.newStakingPoolOperator, getAmountOfMoneyInd(&check->contract.newStakingPoolOperator, 1), 3, 0, NULL);
		setVariableIndPlace(&check->contract.newStakingPoolOperator, 3, 0, check->contract.untilTime);
		char valueRandom[32];
		setVariableIndPlace(idThisContract, 3, 0, getVariableIndPlace(idThisContract, 4, 0));
		setVariableIndPlace(idThisContract, 3, 1, getVariableIndPlace(idThisContract, 4, 1, valueRandom, 32));
		setVariableIndPlace(idThisContract, 3, 2, getVariableIndPlace(idThisContract, 4, 2), valueRandom, 32);
	}
	else
	{
		//add the random staking pool operator to the treap of all random staking pool operators now
		addToTree(&check->contract.newStakingPoolOperator, getAmountOfMoneyInd(&check->contract.newStakingPoolOperator, 1), 3, 0, NULL);
		setVariableIndPlace(&check->contract.newStakingPoolOperator, 3, 0, check->contract.untilTime);
		setVariableIndPlace(&check->contract.newStakingPoolOperator, 3, 1, check->contract.startTime - Time_Block, check->contract.randomValueCommit, 32);
		char tempval[32];
		getVariableIndPlace(&check->contract.newStakingPoolOperator, 3, 1, tempval, 32);

		//add the random staking pool operator to the treap of all random staking pool operators
		addToTree(&check->contract.newStakingPoolOperator, getAmountOfMoneyInd(&check->contract.newStakingPoolOperator, 1), 4, 32, shaOfContract);
		setVariableIndPlace(idThisContract, 4, 0, check->contract.untilTime);
		setVariableIndPlace(idThisContract, 4, 1, check->contract.startTime - Time_Block, check->contract.randomValueCommit, 32);
	}

	return 2;
}

void ReverseBlockUntil(char* message, int len, pair <int, int> untilWhere)
{
	//reverses the actions in the block until a certain point
	Block* m = (Block*)message;

	//reverse the money added to the block creator
	setAmountMoneyInd(&m->BlockCreator, getAmountOfMoneyInd(&m->BlockCreator, 1) - Number_Coins_Per_Block, 1);

	//initialize a pointer to the place where the contents of the block starts
	char* tempPointer = message + sizeof(Block) + m->HowmanyFromEachType[0] * (sizeof(NodeDetails) + 32);

	//check if there in nothing else to reverse
	if (untilWhere.first == 0)
		return;

	//if this is the place to stop reversing at, set it
	int until = m->HowmanyFromEachType[1];
	if (untilWhere.first == 1)
		until = untilWhere.second;

	for (int a = 0; a < until; a++)
	{
		//return the amount of money before the punishment
		unsigned long long amountMoney = getAmountOfMoneyInd((NodeDetails*)tempPointer, 1);
		amountMoney += Punishment_Not_Reveal;

		//save the amounts before the payment
		setAmountMoneyInd((NodeDetails*)tempPointer, amountMoney, 1);

		//update the pointer
		tempPointer += sizeof(NodeDetails);
	}

	//check if the function needs to return
	if (untilWhere.first == 1)
		return;

	//if this is the place to stop reversing at, set it
	until = m->HowmanyFromEachType[2];
	if (untilWhere.first == 2)
		until = untilWhere.second;

	//reverse the payments
	for (int a = 0; a < until; a++)
	{
		//get the amounts of money
		unsigned long long moneySender = getAmountOfMoneyInd(&((Pay*)tempPointer)->senderDetails, 1);
		unsigned long long moneyReceiver = getAmountOfMoneyInd(&((Pay*)tempPointer)->receiverDetails, 1);

		//get the original amounts
		moneySender += ((Pay*)tempPointer)->amountToPay;
		moneyReceiver -= ((Pay*)tempPointer)->amountToPay;

		//save the amounts before the payment
		setAmountMoneyInd(&((Pay*)tempPointer)->senderDetails, moneySender, 1);
		setAmountMoneyInd(&((Pay*)tempPointer)->receiverDetails, moneyReceiver, 1);

		//update the pointer
		tempPointer += sizeof(Transaction);
	}
}

bool checkRandomReveal(NodeDetails* sender, char* newRandomValue, unsigned long long timeSent, char* hashOfContract)
{
	//checks if the random number is known to the user, if it valid and if so updates it
	lock_guard <mutex> lock2(canReceiveRandom);

	//initialize variables
	unsigned long long timeReceived = Get_Time();
	char randomBefore[32], tempValue[32];
	unsigned long long last = getVariableIndPlace(sender, 2, 1, randomBefore, 32);
	copy(newRandomValue, newRandomValue + 32, tempValue);

	//update the values in the treap of the approved random staking pool operators
	if (last != 0 and last <= timeSent)
	{
		//calculate the value this should reveal about
		for (int a = 0; a < (timeSent - last) / Time_Block; a++)
			SHA256(tempValue, 32, tempValue);

		//update the random value and last time updated
		if (memcmp(tempValue, randomBefore, 32) == 0)
		{
			setVariableIndPlace(sender, 2, 1, timeSent, newRandomValue, 32);
			setVariableIndPlace(sender, 2, 2, timeReceived);
			if (lastTimeReceivedRandom < timeSent)
			{
				lastTimeReceivedRandom = timeSent;
				setTimeApproved(timeSent);
			}
		}
	}

	//initialize variables
	last = getVariableIndPlace(sender, 3, 1, randomBefore, 32);
	copy(newRandomValue, newRandomValue + 32, tempValue);

	//update the values in the treap of the approved random staking pool operators now
	if (last != 0 and last <= timeSent)
	{
		//calculate the value this should reveal about
		for (int a = 0; a < (timeSent - last) / Time_Block; a++)
			SHA256(tempValue, 32, tempValue);

		//update the random value and last time updated
		if (memcmp(tempValue, randomBefore, 32) == 0)
		{
			setVariableIndPlace(sender, 3, 1, timeSent, newRandomValue, 32);
			setVariableIndPlace(sender, 3, 2, timeReceived);
			if (lastTimeReceivedRandom < timeSent)
			{
				lastTimeReceivedRandom = timeSent;
				setTimeApproved(timeSent);
			}
		}
	}

	//check if needs to change in treap of all
	if (hashOfContract != NULL)
	{
		//initialize variables
		last = getVariableIndPlace(DataCompare(sender, 32, hashOfContract), 4, 1, randomBefore, 32);
		copy(newRandomValue, newRandomValue + 32, tempValue);

		//check if this reveal is familiar
		if (last > timeSent or last == 0)
			return false;

		//calculate the value this should reveal about
		for (int a = 0; a < (timeSent - last) / Time_Block; a++)
			SHA256(tempValue, 32, tempValue);

		//update the random value and last time updated
		if (memcmp(tempValue, randomBefore, 32) == 0)
		{
			setVariableIndPlace(DataCompare(sender, 32, hashOfContract), 4, 1, timeSent, newRandomValue, 32);
			setVariableIndPlace(DataCompare(sender, 32, hashOfContract), 4, 2, timeReceived);
		}
	}

	return true;
}

void applyOneBlock(char* startOfBlock, int sizeOfBlock)
{
	//apply a given block
	Block* blockPointer = (Block*)(startOfBlock);

	//removes staking pool operators that their contract has ended
	deleteAllNodeIndPlace(1, 0, blockPointer->TimeAtCreation - Time_Block);
	deleteAllNodeIndPlace(3, 0, blockPointer->TimeAtCreation - Time_Block);

	char* tempPointer = startOfBlock + sizeof(Block) + blockPointer->HowmanyFromEachType[0] * sizeof(Random_Reveal);

	//update the new amount of money of the block creator
	updateCoinAmount(&blockPointer->BlockCreator, askNumberOfCoins(&blockPointer->BlockCreator, 1) + Number_Coins_Per_Block, 1);
	setAmountMoneyInd(&blockPointer->BlockCreator, blockPointer->newAmountCreator, 1);

	//apply the punishments for random staking pool operators who didn't reveal their number
	for (int a = 0; a < blockPointer->HowmanyFromEachType[1]; a++, tempPointer += sizeof(NodeDetails))
	{
		//calculate the new amounts of money
		unsigned long long amountOfMoney = getAmountOfMoneyInd((NodeDetails*)tempPointer, 1);
		amountOfMoney -= Punishment_Not_Reveal;

		//removes the punished random staking pool operator from their treap if needed
		if (amountOfMoney < Min_Coins_Random_Staking_Pool_Operator)
			removeNode((NodeDetails*)tempPointer, 3);

		//set the new amount of money for the random staking pool operator
		setAmountMoneyInd((NodeDetails*)tempPointer, amountOfMoney, 1);
	}

	//apply the payments
	for (int a = 0; a < blockPointer->HowmanyFromEachType[2]; a++, tempPointer += sizeof(Transaction))
		checkPayment((Transaction*)tempPointer, 0, true);

	//apply the contracts that bind random staking pool operators
	for (int a = 0; a < blockPointer->HowmanyFromEachType[3]; a++, tempPointer += sizeof(Contract_Random))
		checkBindRandomStakingPoolOperator((Contract_Random*)tempPointer, 0, true);

	//apply the contracts that bind staking pool operators
	for (int a = 0; a < blockPointer->HowmanyFromEachType[4]; a++, tempPointer += sizeof(Contract))
		checkBindStakingPoolOperator((Contract*)tempPointer, 0, true);

	//removes staking pool operators that their contract has ended
	deleteAllNodeIndPlace(1, 0, blockPointer->TimeAtCreation);
	deleteAllNodeIndPlace(3, 0, blockPointer->TimeAtCreation);
}

void initializeTreapsAll()
{
	//initialize the treaps
	copyTreap(0, 1);
	copyTreap(2, 3);
}

void applyBlockFake(char* shaOfBlock)
{
	//get the path to the head
	pair <char*, int> answer = getPathToNode(shaOfBlock);
	initializeTreapsAll();
	
	//apply the blocks on the path
	for (int a = answer.second - 1; a >= 0; a--)
	{
		BlockTreeNode* blockNow = ((BlockTreeNode**)answer.first)[a];
		applyOneBlock(blockNow->startOfBlock, blockNow->sizeOfBlock);
	}

	//free the memory allocated for passing the information
	if (answer.second != 0)
		free(answer.first);
}

void reversePath(char* shaOfBlock, bool include)
{
	//reverse blocks on a path to a block
	pair <char*, int> answer = getPathToNode(shaOfBlock);

	int start = 0;
	if (!include)
		start = 1;

	for (int a = start; a < answer.second; a++)
	{
		BlockTreeNode* blockNow = ((BlockTreeNode**)answer.first)[a];
		ReverseBlockUntil(blockNow->startOfBlock, blockNow->sizeOfBlock, { -1, -1 });
	}
}

void applyBlockReal(char* shaOfBlock)
{
	//apply a block to the real state
	//applies the blocks
	pair <char*, int> blockApproved = getBlock(shaOfBlock);
	
	//applyBlockFake(shaOfBlock);
	applyBlockFake(((Block*)blockApproved.first)->SHA256OfParent);

	//switches the treaps of the fake and real states
	swapTreaps(0, 1);
	swapTreaps(2, 3);

	//set that the initiation process is completed
	isFirstAll = false;

	//update the last time of an approved block
	lastTimeApproved = ((Block*)blockApproved.first)->TimeAtCreation - Time_Block;

	//set the amount of money for this user
	Number_Coins = getAmountOfMoneyInd(&My_Details, 1);
	loadIntoFile();
	pair <char*, int> theNewHead = getBlock(shaOfBlock);
	blockNumberApproved = ((Block*)theNewHead.first)->BlockNumber;

	deleteAllNodeIndPlace(4, 0, ((Block*)theNewHead.first)->TimeAtCreation - Time_Block);

	makeNewRoot(shaOfBlock);
}

void tryApproveBlock()
{
	//tries to approve the block
	//check every 4 seconds for 20 seconds if there are enough signatures
	for (int a = 0; a < 5; a++)
	{
		this_thread::sleep_for(chrono::milliseconds(4000));

		pair <char*, int> message = Handle_Confirm_Block_All_Create();
		if (Handle_Confirm_Block_All_Process(message.first, message.second))
		{
			free(message.first);
			break;
		}
		free(message.first);
	}

	isThisUserCreating = false;
	Signatures_For_Confirm_Message = {};
}

void tryCreateNextBlock()
{
	//make sure checking this will not cause errors
	lock_guard <mutex> lock1(canUseBlockTreeActions);

	//find the best parent for a new block and simulate what happens after it
	BlockTreeNode* parentOfNew = createOnThisBlock();
	applyBlockFake(parentOfNew->sha256OfBlock);

	//removes staking pool operators that their contract has ended
	unsigned long long temp = Get_Time();
	temp -= temp % Time_Block + Time_Block;
	deleteAllNodeIndPlace(1, 0, temp);
	deleteAllNodeIndPlace(3, 0, temp);

	//initialize variable
	char nextCreator[32]{ 0 };

	//get the xor of all random numbers that were revealed
	unsigned long long timeNow = Get_Time() - Time_Block;
	timeNow -= timeNow % Time_Block;
	getXorAll(timeNow, nextCreator);

	//checks if there is an error with receiving the random number
	char zeroForCheck[32]{ 0 };
	if (memcmp(zeroForCheck, nextCreator, 32) == 0)
	{
		//check if the info arrived without problems
		if (!Is_Bootnode or Number_Of_Bootnodes > 1)
			askInformation(true);
		else
			cout << "Error the random number is 0" << '\n';
	}

	//get the next creator
	NodeDetails answer;
	getNextBlockCreator(nextCreator, &answer);

	//check if this user is the next creator and if it is, create it
	if (memcmp(&My_Details, &answer, sizeof(NodeDetails)) == 0)
	{
		cout << "i am the block creator!!!" << '\n';

		//create the new block
		pair <char*, int> blockMessage = Handle_Block_Create(parentOfNew->sha256OfBlock, ((Block*)parentOfNew->startOfBlock)->BlockNumber + 1);
		spreadMessage(blockMessage.first, blockMessage.second);

		//free the memory
		free(blockMessage.first);
	}

	//reverse the simulation of the state before the block
	reversePath(parentOfNew->sha256OfBlock, true);
}
