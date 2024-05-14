#include "H_Treap.h"
#include <mutex>
#include <boost/multiprecision/cpp_int.hpp>
#include "H_General_Functions.h"
#include "H_Network_Operations.h"

using namespace boost::multiprecision;

vector <Treap_Node*> Trees;
vector <mutex*> mutexVectorTreap;
vector <unsigned int> sizesOfTreaps;

int getRelationTwoNodes(DataCompare first, DataCompare second)
{
	//returns the relation betweeen the values of two nodes
	int ans = memcmp(first.NodeId->nodeID, second.NodeId->nodeID, 32);
	if (ans == 0 and first.sizeOfData != 0)
		ans = memcmp(first.dataStarts, second.dataStarts, first.sizeOfData);
	return ans;
}

Treap_Node* merge(Treap_Node* leftNode, Treap_Node* rightNode)
{
	//function that merges two treaps
	//check stopping condition
	if (leftNode == NULL)
		return rightNode;
	if (rightNode == NULL)
		return leftNode;

	//right is above
	if (leftNode->yValue > rightNode->yValue)
		swap(leftNode, rightNode);

	//get the answer from the lower layers
	int temp = getRelationTwoNodes(rightNode->compareWithOthers, leftNode->compareWithOthers);
	if (temp > 0)
		rightNode->lp = merge(leftNode, rightNode->lp);//right is larger, go left
	else
		rightNode->rp = merge(leftNode, rightNode->rp);//right is smaller, go right

	//return the result
	rightNode->sumCoins = rightNode->coinsAmount;
	if (rightNode->lp != NULL)
		rightNode->sumCoins += rightNode->lp->sumCoins;
	if (rightNode->rp != NULL)
		rightNode->sumCoins += rightNode->rp->sumCoins;
	return rightNode;
}

pair <Treap_Node*, Treap_Node*> split(Treap_Node* nodeNow, DataCompare splitBy, bool whatToDoSame)
{
	//function that splits a treap into two treaps
	if (nodeNow == NULL)
		return { NULL, NULL };

	//get the answer from the lower layers
	pair <Treap_Node*, Treap_Node*> answer;
	int temp = getRelationTwoNodes(nodeNow->compareWithOthers, splitBy);
	if (temp > 0 or (temp == 0 and whatToDoSame))
	{
		//split the left node from here
		answer = split(nodeNow->lp, splitBy, whatToDoSame);
		nodeNow->lp = answer.second;

		//update the sum of coins
		nodeNow->sumCoins = nodeNow->coinsAmount;
		if (nodeNow->lp != NULL)
			nodeNow->sumCoins += nodeNow->lp->sumCoins;
		if (nodeNow->rp != NULL)
			nodeNow->sumCoins += nodeNow->rp->sumCoins;

		answer.second = nodeNow;
	}
	else
	{
		//split the right node from here
		answer = split(nodeNow->rp, splitBy, whatToDoSame);
		nodeNow->rp = answer.first;

		//update the sum of coins
		nodeNow->sumCoins = nodeNow->coinsAmount;
		if (nodeNow->lp != NULL)
			nodeNow->sumCoins += nodeNow->lp->sumCoins;
		if (nodeNow->rp != NULL)
			nodeNow->sumCoins += nodeNow->rp->sumCoins;

		answer.first = nodeNow;
	}
	return answer;
}

Treap_Node* getTreapNode(Treap_Node* nodeNow, DataCompare whoToAsk)
{
	//gets a pointer to a Treap_Node with certain value
	//stopping condition
	if (nodeNow == NULL)
		return NULL;
	
	//get the answer from the lower layers
	int temp = getRelationTwoNodes(nodeNow->compareWithOthers, whoToAsk);

	if (temp > 0)
		return getTreapNode(nodeNow->lp, whoToAsk);//the target is to the left
	else if (temp < 0)
		return getTreapNode(nodeNow->rp, whoToAsk);//the target is to the right
	else
		return nodeNow;
}

void changeTreapNode(Treap_Node* nodeNow, DataCompare whoToAsk, unsigned long long newCoinsAmount)
{
	//changes the amount of coins to a treapNode
	//stopping condition
	if (nodeNow == NULL)
		return;

	//get the answer from the lower layers
	int temp = getRelationTwoNodes(nodeNow->compareWithOthers, whoToAsk);

	if (temp > 0)
		changeTreapNode(nodeNow->lp, whoToAsk, newCoinsAmount);//the target is to the left
	else if (temp < 0)
		changeTreapNode(nodeNow->rp, whoToAsk, newCoinsAmount);//the target is to the right
	else
		nodeNow->coinsAmount = newCoinsAmount;//changes the coin amount

	//updates the sum of coins before leaving
	nodeNow->sumCoins = nodeNow->coinsAmount;
	if (nodeNow->lp != NULL)
		nodeNow->sumCoins += nodeNow->lp->sumCoins;
	if (nodeNow->rp != NULL)
		nodeNow->sumCoins += nodeNow->rp->sumCoins;
}

Treap_Node* getTreapNodeBySum(Treap_Node* nodeNow, unsigned long long sumNow)
{
	//returns a pointer to the treap node by the sum of coins
	//gets the amount of coins in the left branch
	unsigned long long lsize = 0;
	if (nodeNow->lp != NULL)
		lsize = nodeNow->lp->sumCoins;

	//get the answer from the lower layers
	if (nodeNow->lp != NULL and sumNow <= lsize)
		return getTreapNodeBySum(nodeNow->lp, sumNow);
	else if (sumNow <= lsize + nodeNow->coinsAmount)
		return nodeNow;
	else
		return getTreapNodeBySum(nodeNow->rp, sumNow - lsize - nodeNow->coinsAmount);
}

void deleteTreap(Treap_Node* nodeNow)
{
	//deletes the treap
	//delete the subtree
	if (nodeNow == NULL)
		return;

	deleteTreap(nodeNow->lp);
	deleteTreap(nodeNow->rp);

	//delete this node
	if (nodeNow->isTherePlace)
		free(nodeNow->valueOfVariable);
	delete(nodeNow);
}

bool isInTreap(Treap_Node* nodeNow, DataCompare who)
{
	//asks if a node is in the treap
	//stopping condition
	if (nodeNow == NULL)
		return false;

	//get the answer from the lower layers
	int temp = getRelationTwoNodes(nodeNow->compareWithOthers, who);
	if (temp > 0)
		return isInTreap(nodeNow->lp, who);//the target is to the left
	else if (temp < 0)
		return isInTreap(nodeNow->rp, who);//the target is to the right

	return true;
}

vector <Treap_Node*> listOfNodes;

void returnNodes(Treap_Node* nodeNow, unsigned long long value, int place)
{
	//fills a vector with pointer to all treap nodes that satisfy a certain condition
	if (nodeNow == NULL)
		return;

	//check if this node needs to be added
	if (nodeNow->variables[place] <= value)
		listOfNodes.push_back(nodeNow);

	//get the answer from the lower layers
	returnNodes(nodeNow->lp, value, place);
	returnNodes(nodeNow->rp, value, place);
}

void copyInfo(Treap_Node* nodeNow)
{
	//copy the nodeDetails and end time of all nodes in the treap
	//copy the info into an array
	char dataToCopy[sizeof(NodeDetails) + 2 * sizeof(unsigned long long)];
	copy(&nodeNow->valueHere, (NodeDetails*)((char*)&nodeNow->valueHere + sizeof(NodeDetails)), (NodeDetails*)dataToCopy);
	*(unsigned long long*)& dataToCopy[sizeof(NodeDetails)] = nodeNow->coinsAmount;
	*(unsigned long long*)& dataToCopy[sizeof(NodeDetails) + sizeof(unsigned long long)] = nodeNow->variables[0];

	//copy the data to the rest of the message
	copyDataToMessage(dataToCopy, sizeof(NodeDetails) + 2 * sizeof(unsigned long long), 1);

	//get the answer from the subtree
	if (nodeNow->lp != NULL)
		copyInfo(nodeNow->lp);

	if (nodeNow->rp != NULL)
		copyInfo(nodeNow->rp);
}

char* whereToCopyVariable;

int copyDetails(Treap_Node* nodeNow, unsigned long long time, bool isSame)
{
	//copies details from the treap to a given place
	//initialize variable
	int sum = 0;

	//stopping condition
	if (nodeNow == NULL)
		return 0;

	//get the answer from the left subtree
	sum = copyDetails(nodeNow->lp, time, isSame);

	//copy the variable from here
	if ((nodeNow->variables[1] == time and isSame) or (nodeNow->variables[1] != time and !isSame))
	{
		//update the answer
		sum++;

		//copy the node details to the relevant place
		copy(&nodeNow->valueHere, (NodeDetails*)((char*)&nodeNow->valueHere + sizeof(NodeDetails)), (NodeDetails*)whereToCopyVariable);
		whereToCopyVariable += sizeof(NodeDetails);

		//check if you need to copy the variable
		if (isSame)
		{
			copy(nodeNow->valueOfVariable, nodeNow->valueOfVariable + 32, whereToCopyVariable);
			whereToCopyVariable += 32;
		}
	}

	//get the answer from the right subtree
	sum += copyDetails(nodeNow->rp, time, isSame);

	return sum;
}

unsigned int getRandomInt()
{
	//returns a random integer for the y values
	int temp;
	fillRandom((unsigned char*) &temp, 4);
	return temp;
}

void initTreap()
{
	//adds a new treap
	mutex* newMutex = new mutex();
	mutexVectorTreap.push_back(newMutex);
	sizesOfTreaps.push_back(0);
	Trees.push_back(NULL);
}

void addToTree(NodeDetails* addNewNode, unsigned long long amountOfCoins, int ind, int sizeCompare, char* startCompare, bool shouldTakeMutex)
{
	//add a node to the treap
	if (shouldTakeMutex)
		lock_guard <mutex> lock(*mutexVectorTreap[ind]);

	//creates the new treap node
	Treap_Node* newNodeToTree = new Treap_Node(amountOfCoins, addNewNode, getRandomInt(), sizeCompare, startCompare);
	
	//checks if the node is the first to be added to the tree
	if (sizesOfTreaps[ind] == 0)
	{
		Trees[ind] = newNodeToTree;
		sizesOfTreaps[ind]++;
		return;
	}

	//splits the treap tree and merges it puting the new node in the middle
	pair <Treap_Node*, Treap_Node*> temp = split(Trees[ind], addNewNode, true);
	pair <Treap_Node*, Treap_Node*> temp2 = split(temp.second, addNewNode, false);
	Trees[ind] = merge(temp.first, newNodeToTree);
	Trees[ind] = merge(Trees[ind], temp2.second);

	//updates the size of the treap
	sizesOfTreaps[ind]++;
}

void removeNode(DataCompare whoToRemove, int ind)
{
	//removes a node from the treap
	//splits the treap tree and merges it without the node that needs to be deleted
	lock_guard <mutex> lock(*mutexVectorTreap[ind]);
	pair <Treap_Node*, Treap_Node*> temp = split(Trees[ind], whoToRemove, true);
	pair <Treap_Node*, Treap_Node*> temp2 = split(temp.second, whoToRemove, false);
	Trees[ind] = merge(temp.first, temp2.second);

	//checks if this node was in the treap
	if (temp2.first == NULL)
		return;

	//delete the removed node
	if (temp2.first->isTherePlace)
		free(temp2.first->valueOfVariable);
	delete(temp2.first);

	//update the size of the treap
	sizesOfTreaps[ind]--;
}

void updateCoinAmount(DataCompare whoToUpdate, unsigned long long newCoinAmount, int ind)
{
	//update the amount of coins of a node in the treap
	lock_guard <mutex> lock(*mutexVectorTreap[ind]);
	changeTreapNode(Trees[ind], whoToUpdate, newCoinAmount);
}

unsigned long long askNumberOfCoins(DataCompare whoToAskOn, int ind)
{
	//ask about the amount of coins a node has
	lock_guard <mutex> lock(*mutexVectorTreap[ind]);
	Treap_Node* askOn = getTreapNode(Trees[ind], whoToAskOn);
	if (askOn == NULL)
		return 0;
	return askOn->coinsAmount;
}

bool isInTreapInd(DataCompare who, int ind)
{
	//check if the node is in the treap
	lock_guard <mutex> lock(*mutexVectorTreap[ind]);
	return isInTreap(Trees[ind], who);
}

int numberOutNow;

void printCoinsUser(Treap_Node* nodeNow)
{
	//check if needs to return
	if (nodeNow == NULL)
		return;

	//print the data
	cout << numberOutNow << ". id: ";
	char id[64];
	turnToASCII(id, nodeNow->valueHere.nodeID, 32);
	for (int a = 0; a < 64; a++)
		cout << id[a];
	cout << " coins: " << nodeNow->coinsAmount << '\n';

	//increases the counter
	numberOutNow++;

	//call the action for the lower layers
	printCoinsUser(nodeNow->rp);
	printCoinsUser(nodeNow->lp);
}

void getNextBlockCreator(char* xorAll, NodeDetails* placeTheAnswer)
{
	//returns the details of the next block creator
	lock_guard <mutex> lock(*mutexVectorTreap[1]);

	//convert the xor value into uint256_t
	uint256_t randomNumber = 0, temp = Trees[1]->sumCoins;

	for (int a = 0; a < 32; a++)
		randomNumber = (randomNumber << 8) | xorAll[a];

	//get the next block creator and return the answer
	unsigned long long getNumberCoins = (unsigned long long)(randomNumber % temp);

	Treap_Node* nextCreator = getTreapNodeBySum(Trees[1], getNumberCoins);
	copy(&nextCreator->valueHere, (NodeDetails*)((char*)&nextCreator->valueHere + sizeof(NodeDetails)), placeTheAnswer);
}

unsigned long long getVariableIndPlace(DataCompare whoToAskOn, int ind, int place, char* copyTo, int sizeToCopy)
{
	//returns the variable in a certain index and place in the array
	lock_guard <mutex> lock(*mutexVectorTreap[ind]);
	Treap_Node* node = getTreapNode(Trees[ind], whoToAskOn);
	if (node == NULL)
		return 0;
	if (copyTo != NULL)
		copy(node->valueOfVariable, node->valueOfVariable + sizeToCopy, copyTo);
	return node->variables[place];
}

void setVariableIndPlace(DataCompare whoToChange, int ind, int place, unsigned long long value, char* copyFrom, int sizeOfVariable, bool shouldTakeMutex)
{
	//sets the variable in a certain index and place in the array
	if (shouldTakeMutex)
		lock_guard <mutex> lock(*mutexVectorTreap[ind]);

	Treap_Node* node = getTreapNode(Trees[ind], whoToChange);
	if (node == NULL)
		return;

	node->variables[place] = value;

	if (copyFrom == NULL)
		return;

	//copy the value
	if (!node->isTherePlace)
	{
		node->valueOfVariable = (char*)malloc(sizeOfVariable);
		node->isTherePlace = true;
	}
	copy(copyFrom, copyFrom + sizeOfVariable, node->valueOfVariable);
}

unsigned long long getSumCoins(int ind)
{
	//returns the sum of coins in a certain treap
	lock_guard <mutex> lock(*mutexVectorTreap[ind]);
	return Trees[ind]->sumCoins;
}

int copyDetailsFromTreapInd(int ind, char* placeToCopy, unsigned long long time, bool isSame)
{
	//copies the wanted details from the treap
	lock_guard <mutex> lock(*mutexVectorTreap[ind]);
	whereToCopyVariable = placeToCopy;
	return copyDetails(Trees[ind], time, isSame);
}

void deleteTreapAll(int ind)
{
	//delete the treap
	deleteTreap(Trees[ind]);
	sizesOfTreaps[ind] = 0;
	Trees[ind] = NULL;
}

void copyInfoAnswer(int ind)
{
	//copy information about all staking pool operators
	lock_guard <mutex> lock(*mutexVectorTreap[ind]);
	copyInfo(Trees[ind]);
}

void copyInfoRandom(Treap_Node* nodeNow)
{
	//copy the nodeDetails and end time of all nodes in the treap
	//copy the info into an array
	if (isInTreapInd(&nodeNow->valueHere, 2))
	{
		char dataToCopy[sizeof(Info_Random)];
		Info_Random* temp = (Info_Random*)dataToCopy;
		copy(&nodeNow->valueHere, (NodeDetails*)((char*)&nodeNow->valueHere + sizeof(NodeDetails)), &temp->identity);
		temp->timeLastReveal = nodeNow->variables[1];
		copy(nodeNow->valueOfVariable, nodeNow->valueOfVariable + 32, temp->lastRandomRevealed);
		copy(nodeNow->compareWithOthers.dataStarts, nodeNow->compareWithOthers.dataStarts + 32, temp->shaOfContract);

		//copy the data to the rest of the message
		copyDataToMessage(dataToCopy, sizeof(Info_Random), 2);
	}
	
	//get the answer from the subtree
	if (nodeNow->lp != NULL)
		copyInfo(nodeNow->lp);

	if (nodeNow->rp != NULL)
		copyInfo(nodeNow->rp);
}

void copyInfoRandom(int ind)
{
	//copy information about the random staking pool operator
	lock_guard <mutex> lock(*mutexVectorTreap[ind]);
	copyInfoRandom(Trees[ind]);
}

int getSizeTreapInd(int ind)
{
	//returns the size of a treap
	lock_guard <mutex> lock(*mutexVectorTreap[ind]);
	return sizesOfTreaps[ind];
}

mutex forUsingVector;

void deleteAllNodeIndPlace(int ind, int place, unsigned long long value)
{
	//deletes all the nodes that their value of the specified variable is small enough
	//get a list of all this nodes
	(*mutexVectorTreap[ind]).lock();
	forUsingVector.lock();
	listOfNodes = {};
	returnNodes(Trees[ind], value, place);
	(*mutexVectorTreap[ind]).unlock();

	//removes all the nodes in the list from the treap
	for (auto a : listOfNodes)
		removeNode(a->compareWithOthers, ind);

	forUsingVector.unlock();
}

vector <Treap_Node*> allNodesInTreap;

void getAllTreapNodes(Treap_Node* nodeNow)
{
	//check if you need to stop
	if (nodeNow == NULL)
		return;

	//save this node
	allNodesInTreap.push_back(nodeNow);

	//get the answer from the lower layers
	getAllTreapNodes(nodeNow->lp);
	getAllTreapNodes(nodeNow->rp);
}

void swapTreaps(int firstTreap, int secondTreap)
{
	//swaps two treaps
	lock_guard <mutex> lock1(*mutexVectorTreap[firstTreap]);
	lock_guard <mutex> lock2(*mutexVectorTreap[secondTreap]);
	swap(Trees[firstTreap], Trees[secondTreap]);
	swap(sizesOfTreaps[firstTreap], sizesOfTreaps[secondTreap]);
}

void copyTreap(int copyFrom, int copyTo)
{
	//copies the first treap to the second treap
	lock_guard <mutex> lock1(*mutexVectorTreap[copyFrom]);
	lock_guard <mutex> lock2(*mutexVectorTreap[copyTo]);

	//delete the treap that is copied to
	deleteTreap(Trees[copyTo]);
	sizesOfTreaps[copyTo] = 0;
	Trees[copyTo] = NULL;

	allNodesInTreap = {};
	getAllTreapNodes(Trees[copyFrom]);
	
	//add the nodes to the tree
	for (auto a : allNodesInTreap)
	{
		addToTree(a->compareWithOthers.NodeId, a->coinsAmount, copyTo, a->compareWithOthers.sizeOfData, a->compareWithOthers.dataStarts, false);
		for (int b = 1; b < Number_Of_Variables; b++)
			setVariableIndPlace(&a->valueHere, copyTo, b, a->variables[b], NULL, 0, false);
		if (a->isTherePlace)
			setVariableIndPlace(&a->valueHere, copyTo, 0, a->variables[0], a->valueOfVariable, 32, false);
		else
			setVariableIndPlace(&a->valueHere, copyTo, 0, a->variables[0], NULL, 0, false);
	}
	sizesOfTreaps[copyTo] = sizesOfTreaps[copyFrom];
	if (sizesOfTreaps[copyTo] == 0)
		Trees[copyTo] = NULL;
}

void calcXorAll(Treap_Node* nodeNow, unsigned long long timeUntil, char* placeXor)
{
	//calculate the xor of all revealed values
	if (nodeNow->variables[1] == timeUntil)
		for (int a = 0; a < 32; a++)
			placeXor[a] ^= nodeNow->valueOfVariable[a];

	//get the answer from the lower layers
	if (nodeNow->lp != NULL)
		calcXorAll(nodeNow->lp, timeUntil, placeXor);
	if (nodeNow->rp != NULL)
		calcXorAll(nodeNow->rp, timeUntil, placeXor);
}

void getXorAll(unsigned long long timeUntil, char* placeXor)
{
	//get the xor of all revealed values
	lock_guard <mutex> lock(*mutexVectorTreap[3]);
	calcXorAll(Trees[3], timeUntil, placeXor);
}

void printDataTreap()
{
	//print the data of the treap
	numberOutNow = 1;
	printCoinsUser(Trees[0]);
}
