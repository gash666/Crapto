#pragma once
#include <boost/multiprecision/cpp_int.hpp>
#include "H_Node_Supporter.h"
#include "H_Node_Supporter.h"

#define Number_Of_Variables 3

using namespace boost::multiprecision;

struct DataCompare
{
	NodeDetails* NodeId = NULL;
	int sizeOfData = 0;
	char* dataStarts = NULL;
	DataCompare(NodeDetails* node, int sizeData, char* startData)
	{
		NodeId = node;
		sizeOfData = sizeData;
		dataStarts = (char*)malloc(sizeData);
		copy(startData, startData + sizeData, dataStarts);
	}
	DataCompare(NodeDetails* node)
	{
		NodeId = node;
	}
};

struct Treap_Node
{
	unsigned long long sumCoins = 0;
	unsigned long long coinsAmount;
	NodeDetails valueHere;
	Treap_Node* lp = NULL, * rp = NULL;
	unsigned int yValue;
	unsigned long long variables[Number_Of_Variables];
	char* valueOfVariable = NULL;
	bool isTherePlace = false;
	DataCompare compareWithOthers = DataCompare(NULL);

	Treap_Node(unsigned long long amountOfCoins, NodeDetails* addNewNode, unsigned int randomNumber, int sizeOfCompare, char* dataToCopy)
	{
		coinsAmount = amountOfCoins;
		sumCoins = amountOfCoins;
		copy(addNewNode, (NodeDetails*)((char*)addNewNode + sizeof(NodeDetails)), &valueHere);
		compareWithOthers = DataCompare(&valueHere, sizeOfCompare, dataToCopy);
		yValue = randomNumber;
		for (int a = 0; a < Number_Of_Variables; a++)
			variables[a] = 0;
	}
};

//0 - staking pool operators approved
//1 - staking pool operators now
//2 - random staking pool operators approved
//3 - random staking pool operators now
//4 - all random staking pool operators - with special compare
//5 - all staking pool operators who signed the message already

//variables[0] - time to end contract
//variables[1] - signed already on the block / last time revealed random number
//variables[2] - / the time the random reveal arrived

void initTreap();
void addToTree(NodeDetails* addNewNode, unsigned long long amountOfCoins, int ind, int sizeCompare, char* startCompare, bool shoudTakeMutex = true);
void removeNode(DataCompare whoToRemove, int ind);
void updateCoinAmount(DataCompare whoToUpdate, unsigned long long newCoinAmount, int ind);
unsigned long long askNumberOfCoins(DataCompare whoToAskOn, int ind);
bool isInTreapInd(DataCompare who, int ind);
void getNextBlockCreator(char* xorAll, NodeDetails* placeTheAnswer);
unsigned long long getVariableIndPlace(DataCompare whoToAskOn, int ind, int place, char* copyTo = NULL, int sizeToCop = 0);
void setVariableIndPlace(DataCompare whoToChange, int ind, int place, unsigned long long value, char* copyFrom = NULL, int sizeOfVariable = 0, bool shoudTakeMutex = true);
unsigned long long getSumCoins(int ind);
int copyDetailsFromTreapInd(int ind, char* placeToCopy, unsigned long long time, bool isSame);
void copyInfoAnswer(int ind);
void copyInfoRandom(int ind);
int getSizeTreapInd(int ind);
void deleteTreapAll(int ind);
void deleteAllNodeIndPlace(int ind, int place, unsigned long long value);
void swapTreaps(int firstTreap, int secondTreap);
void copyTreap(int copyFrom, int copyTo);
void getXorAll(unsigned long long timeUntil, char* placeXor);
void printDataTreap();
