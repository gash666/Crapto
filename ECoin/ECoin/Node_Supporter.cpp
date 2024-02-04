#include <iostream>
#include <vector>
#include <cstdint>
#include <climits>
#include <mutex>
#include <boost/asio.hpp>
#include <iomanip>
#include "H_Node_Supporter.h"
#include "H_Constants.h"
#include "H_Variables.h"

using namespace std;

//the array of buckets
vector <pair <NodeDetails, unsigned long long>> buckets[256];

int getBucket(char* id)
{
	//returns the bucket for node with certain id
	int ind = 0;
	while (ind < 32 and id[ind] == My_Details.nodeID[ind])
		ind++;
	if (ind == 32)
		return 255;
	int tempCompare = id[ind] ^ My_Details.nodeID[ind];
	for (int a = 7; a > 0; a--)
		if (((tempCompare >> a) & 1) == 1)
			return ind * 8 + 7 - a;
	return ind * 8 + 7;
}

pair <int, int> getIndex(char* id)
{
	//returns the index of a node in the bucket list
	int bucket = getBucket(id);
	int ind = -1;
	for (int a = 0; a < buckets[bucket].size() and ind == -1; a++)
		if (memcmp(id, buckets[bucket][a].first.nodeID, 32) == 0)
			ind = a;
	return { bucket, ind };
}

bool getbit(char* value, int position)
{
	//returns the bit at position 256 - position in value
	return (value[(256 - position) / 8] >> (7 - (256 - position) % 8)) & 1;
}

bool removeFromBucket(char removeID[32])
{
	//remove the node from his bucket
	pair <int, int> indexi = getIndex(removeID);
	if (indexi.second == -1)
	{
		cout << "error Node_Supporter.cpp removeFromBucket caused by headOfTree->deleteNode(val)" << '\n';
		return false;
	}
	swap(buckets[indexi.first][buckets[indexi.first].size() - 1], buckets[indexi.first][indexi.second]);
	buckets[indexi.first].pop_back();
	return true;
}

void eraseAll(Node* now)
{
	//erases all nodes in the subtree of now
	if (now == NULL)
		return;
	if (now->lp != NULL)
	{
		eraseAll(now->lp);
		eraseAll(now->rp);
	}
	delete now;
}

void Tree::initTree(bool isNew)
{
	//initialize the data
	if (!isNew)
		eraseAll(headOfTree);
	char tempID[32];
	headOfTree = new Node(256, tempID, this);
	if (checkBucket)
		for (int a = 0; a < 256; a++)
			buckets[a] = {};
}

bool Tree::addNodeToTree(NodeDetails* newNode)
{
	//adds a new node to the tree
	if (checkBucket)
	{
		//checks if the node has a limit for entering
		pair <int, int> indexOfID = getIndex(newNode->nodeID);
		if (indexOfID.second != -1)
			return true;
		else if (buckets[getBucket(newNode->nodeID)].size() < Bucket_Size)
		{
			//adds the node to the tree
			headOfTree->addNode(newNode);
			buckets[getBucket((*newNode).nodeID)].push_back({ *newNode, 0 });
			return true;
		}
		return false;
	}
	headOfTree->addNode(newNode);
	return true;
}

void Tree::fillList(int howmany, char* val, NodeDetails* placeList)
{
	//wrapper function for findClose, allows it to change the place to put the nodes
	nodes = placeList;
	placeNow = 0;
	headOfTree->findClose(howmany, val);
}

void Tree::removeNodeFromTree(char* val)
{
	//remove node from the tree
	needUp = NULL;
	bool remove = headOfTree->deleteNode(val);
	if (!remove)
		cout << "no node was removed" << '\n';
	if (remove)
	{
		//remove the node from the buckets list
		if (checkBucket)
			removeFromBucket(val);

		if (needUp->tsize == 0)
		{
			//if there are no values needed to be raised
			if (needUp->level == 256)
			{
				//if the node to remove is the root, act accordingly
				eraseAll(needUp);
				initTree(false);
			}
			else
				eraseAll(needUp);
		}
		else if (needUp->tsize == 1)
		{
			//delete the values below and set this as the one that goes up
			needUp->raiseLevels();
			eraseAll(needUp->lp);
			needUp->lp = NULL;
			eraseAll(needUp->rp);
			needUp->rp = NULL;
			needUp->nodeInfo = goesUp;
			needUp->hasSent = goesUpHasSent;
		}
		else
			cout << "error returned too big" << '\n';
	}
}

void Tree::setHasSent(NodeDetails* changeAbout)
{
	//sets that the user already sent a message with what he knows
	headOfTree->changeSent(changeAbout);
}

bool Tree::getHasSent(NodeDetails* askAbout)
{
	//returns whether the user already sent a message
	return headOfTree->askSent(askAbout);
}


void Node::addToList()
{
	//adds this node to a list of nodes
	aboveAll->nodes[aboveAll->placeNow] = nodeInfo;
	aboveAll->placeNow++;
}

void Node::findClose(int howmany, char* val)
{
	//finds the howmany closest node ids in xor distance to *val
	if (tsize == 0)
		cout << "error went to 0" << '\n';
	if (rp != NULL and lp != NULL)
	{
		if (getbit(val, level))
		{
			//if rp is closer, go to it first
			rp->findClose(min(howmany, rp->tsize), val);
			if (howmany > rp->tsize)
				lp->findClose(howmany - rp->tsize, val);
			return;
		}
		//if lp is closer, go to it first
		lp->findClose(min(howmany, lp->tsize), val);
		if (howmany > lp->tsize)
			rp->findClose(howmany - lp->tsize, val);
		return;
	}
	else if (rp != NULL)
	{
		//if rp is the only way forward go to it
		rp->findClose(howmany, val);
		return;
	}
	else if (lp != NULL)
	{
		//if lp is the only way forward go to it
		lp->findClose(howmany, val);
		return;
	}
	//if both way aren't possible, this is the last so add it
	addToList();
}

void Node::addNodeRight()
{
	//create a new node from the right of this one
	char temp = value[(256 - level) / 8];
	value[(256 - level) / 8] |= 1 << (7 - ((256 - level) % 8));
	rp = new Node(level - 1, value, aboveAll);
	value[(256 - level) / 8] = temp;
}

void Node::addNodeLeft()
{
	//create a new node from the left of this one
	lp = new Node(level - 1, value, aboveAll);
}

void Node::changeAddress(NodeDetails* change)
{
	//changes a nodes ip and port by its id
	if (lp == NULL and rp == NULL)
	{
		//if reached the end
		nodeInfo = *change;
		return;
	}
	//checks which way to move next
	if (getbit(change->nodeID, level))
		rp->changeAddress(change);
	else
		lp->changeAddress(change);
}

void Node::lowerLevels(NodeDetails* node1, NodeDetails* node2, bool firstSent)
{
	//add the two nodes in the right places
	if (getbit(node1->nodeID, level) == getbit(node2->nodeID, level))
	{
		//their last bit is the same, continue going down
		if (getbit(node1->nodeID, level))
		{
			//if both of them need right, go right
			addNodeRight();
			rp->tsize = 2;
			rp->lowerLevels(node1, node2, firstSent);
			return;
		}
		//if both of them need left, go left
		addNodeLeft();
		lp->tsize = 2;
		lp->lowerLevels(node1, node2, firstSent);
		return;
	}
	//their last bits are different, create places for both of them
	addNodeRight();
	addNodeLeft();
	if (!getbit(node1->nodeID, level))
	{
		//put the first left and the second right
		copy(node1, (NodeDetails*)((char*)node1 + sizeof(NodeDetails)), &lp->nodeInfo);
		copy(node2, (NodeDetails*)((char*)node2 + sizeof(NodeDetails)), &rp->nodeInfo);
		lp->hasSent = firstSent;
	}
	else
	{
		//put the first right and the second left
		copy(node2, (NodeDetails*)((char*)node2 + sizeof(NodeDetails)), &lp->nodeInfo);
		copy(node1, (NodeDetails*)((char*)node1 + sizeof(NodeDetails)), &rp->nodeInfo);
		rp->hasSent = firstSent;
	}
	//set their sizes
	lp->tsize = 1;
	rp->tsize = 1;
}

bool Node::addNode(NodeDetails* val)
{
	//adds a node to the tree
	if (lp == NULL and rp == NULL)
	{
		//checks if the node is the first in the tree
		if (level == 256)
		{
			bool isTheFirst = true;
			for (int a = 0; a < 32 and isTheFirst; a++)
				if (nodeInfo.nodeID[a] != 0)
					isTheFirst = false;
			if (isTheFirst)
			{
				copy(val, (NodeDetails*)((char*)val + sizeof(NodeDetails)), &nodeInfo);
				tsize++;
				return true;
			}
		}

		//adds the node to the tree
		if (memcmp(nodeInfo.nodeID, val->nodeID, 32) != 0)
		{
			//seperate the new and old values
			lowerLevels(&nodeInfo, val, hasSent);
			nodeInfo = { 0 };
			tsize++;
			return true;
		}
	}
	else if (!getbit(val->nodeID, level))
	{
		//move to the left or add there
		if (lp == NULL)
		{
			addNodeLeft();
			lp->tsize = 1;
			copy(val, (NodeDetails*)((char*)val + sizeof(NodeDetails)), &lp->nodeInfo);
			tsize++;
			return true;
		}
		else if (lp->addNode(val))
		{
			tsize++;
			return true;
		}
	}
	else
	{
		//move to the right or add there
		if (rp == NULL)
		{
			addNodeRight();
			rp->tsize = 1;
			copy(val, (NodeDetails*)((char*)val + sizeof(NodeDetails)), &rp->nodeInfo);
			tsize++;
			return true;
		}
		else if (rp->addNode(val))
		{
			tsize++;
			return true;
		}
	}
	return false;
}

void Node::raiseLevels()
{
	//stores the value that needs to go up
	if (lp == NULL and rp == NULL)
	{
		//if this is the one that needs to go up
		aboveAll->goesUp = nodeInfo;
		aboveAll->goesUpHasSent = hasSent;
		return;
	}
	if (rp != NULL and rp->tsize != 0)
	{
		//if the way is to rp
		rp->raiseLevels();
		return;
	}
	//if the way is to lp
	lp->raiseLevels();
}

bool Node::deleteNode(char* val)
{
	//deletes the node with the id stored in val if it exists
	if (lp == NULL and rp == NULL)
	{
		//if both ways are blocked, try to delete here
		if (memcmp(val, nodeInfo.nodeID, 32) == 0)
		{
			//if this is the right value, delete it
			nodeInfo = { 0 };
			tsize--;
			aboveAll->needUp = this;
			return true;
		}
		return false;
	}
	bool remove;
	if (!getbit(val, level))
	{
		//try to move left if needed
		if (lp == NULL)
			return false;
		remove = lp->deleteNode(val);
		if (tsize != 0 and lp->tsize == 0)
			lp = NULL;
	}
	else
	{
		//try to move right if needed
		if (rp == NULL)
			return false;
		remove = rp->deleteNode(val);
		if (tsize != 0 and rp->tsize == 0)
			rp = NULL;
	}

	//if a value was deleted, adjust tsize
	if (remove)
		tsize--;
	if (tsize <= 1)
		aboveAll->needUp = this;
	return remove;
}

void Node::changeSent(NodeDetails* changeAbout)
{
	//change that the node has sent a message
	if (lp == NULL and rp == NULL)
	{
		//set that the user has sent a message
		if (memcmp(changeAbout->nodeID, nodeInfo.nodeID, 32) == 0)
			hasSent = true;
	}
	if (!getbit(changeAbout->nodeID, level))
	{
		//go left towards the node
		if (lp == NULL)
			return;
		lp->changeSent(changeAbout);
	}
	else
	{
		//go right towards the node
		if (rp == NULL)
			return;
		rp->changeSent(changeAbout);
	}
}

bool Node::askSent(NodeDetails* askAbout)
{
	//ask if the node has sent a message
	if (lp == NULL and rp == NULL)
	{
		//return whether the user has sent a message
		if (memcmp(askAbout->nodeID, nodeInfo.nodeID, 32) == 0)
			return hasSent;
		return false;
	}
	if (!getbit(askAbout->nodeID, level))
	{
		//go left towards the node
		if (lp == NULL)
			return false;
		return lp->askSent(askAbout);
	}
	else
	{
		//go right towards the node
		if (rp == NULL)
			return false;
		return rp->askSent(askAbout);
	}
}

vector <bool> exists;
vector <Tree*> trees;
vector <mutex*> mutexVector;
mutex canChange;

int occupyNewTree()
{
	//adds a new tree to the vector
	lock_guard <mutex> lock(canChange);
	int ind = -1;
	for (int a = 0; a < exists.size(); a++)
		if (exists[a])
			ind = a;
	bool isNewTree = false;
	if (ind == -1)
	{
		ind = (int) trees.size();
		mutex* newMutex = new mutex();
		mutexVector.push_back(newMutex);
		exists.push_back(false);
		trees.push_back(new Tree);
		isNewTree = true;
	}
	lock_guard <mutex> lock2(*mutexVector[ind]);
	if (ind == 0)
		trees[ind]->checkBucket = true;
	trees[ind]->initTree(isNewTree);
	return ind;
}

void freeTree(int ind)
{
	//frees the tree so others can use it
	lock_guard <mutex> lock(canChange);
	lock_guard <mutex> lock2(*mutexVector[ind]);
	exists[ind] = true;
}

void fillListInd(int howmany, char* val, NodeDetails* placeList, int ind)
{
	//wrapper function for findClose, allows it to change the place to put the nodes
	lock_guard <mutex> lock(*mutexVector[ind]);
	trees[ind]->fillList(howmany, val, placeList);
}

bool addNodeToTreeInd(NodeDetails* newNode, int ind)
{
	//adds a new node to the tree
	lock_guard <mutex> lock(*mutexVector[ind]);
	return trees[ind]->addNodeToTree(newNode);
}

void removeNodeFromTreeInd(char* val, int ind)
{
	//removes a node from the tree
	lock_guard <mutex> lock(*mutexVector[ind]);
	trees[ind]->removeNodeFromTree(val);
}

int getTreeSizeInd(int ind)
{
	//returns the size of the tree
	lock_guard <mutex> lock(*mutexVector[ind]);
	return trees[ind]->headOfTree->tsize;
}

void setTime(NodeDetails* setAbout, unsigned long long timeToSet)
{
	//sets the last time a node sent a message
	lock_guard <mutex> lock(*mutexVector[0]);
	pair <int, int> ind = getIndex(setAbout->nodeID);
	if (ind.second == -1)
		return;
	buckets[ind.first][ind.second].second = timeToSet;
}

unsigned long long getLastTime(NodeDetails* askAbout)
{
	//returns the last time a node sent a message
	lock_guard <mutex> lock(*mutexVector[0]);
	pair <int, int> ind = getIndex(askAbout->nodeID);
	if (ind.second == -1)
		return -1;
	return buckets[ind.first][ind.second].second;
}

void setHasSentInd(NodeDetails* changeAbout, int ind)
{
	//sets that the user already sent a message with what he knows
	lock_guard <mutex> lock(*mutexVector[ind]);
	trees[ind]->setHasSent(changeAbout);
}

bool getHasSentInd(NodeDetails* askAbout, int ind)
{
	//returns whether the user already sent a message
	lock_guard <mutex> lock(*mutexVector[ind]);
	return trees[ind]->getHasSent(askAbout);
}
