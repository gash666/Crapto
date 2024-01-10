#include <iostream>
#include <vector>
#include <cstdint>
#include <climits>
#include <boost/multiprecision/cpp_int.hpp>
#include "H_Node_Supporter.h"
#include "H_Constants.h"
#include "H_Variables.h"

using namespace std;
using namespace boost::multiprecision;

struct Node;

vector <NodeDetails> *nodes;
Node* needUp;
NodeDetails goesUp;

struct Node
{
	int tsize = 0, level;
	uint256_t value;
	NodeDetails nodeInfo = { 0 };
	Node* lp = NULL, * rp = NULL;

	Node(int lvl, uint256_t val)
	{
		//intializes the node
		level = lvl;
		value = val;
	}

	void addToList()
	{
		//adds this node to a list of nodes
		(*nodes).push_back(nodeInfo);
	}

	void findClose(int howmany, uint256_t* val)
	{
		//finds the howmany closest node ids in xor distance to *val
		if (tsize == 0)
			cout << "error went to 0" << '\n';
		if (rp != NULL and lp != NULL)
		{
			if ((*val ^ lp->value) > (*val ^ rp->value))
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

	void addNodeRight()
	{
		//create a new node from the right of this one
		rp = new Node(level - 1, value + (((uint256_t)(1)) << (level - 1)));
	}

	void addNodeLeft()
	{
		//create a new node from the left of this one
		lp = new Node(level - 1, value);
	}

	void lowerLevels(NodeDetails* node1, NodeDetails* node2)
	{
		//add the two nodes in the right places
		if (((node1->nodeID >> (level - 1)) & 1) == ((node2->nodeID >> (level - 1)) & 1))
		{
			//their last bit is the same, continue going down
			if (((node1->nodeID >> (level - 1)) & 1) == 1)
			{
				//if both of them need right, go right
				addNodeRight();
				rp->tsize = 2;
				rp->lowerLevels(node1, node2);
				return;
			}
			//if both of them need left, go left
			addNodeLeft();
			lp->tsize = 2;
			lp->lowerLevels(node1, node2);
			return;
		}
		//their last bits are different, create places for both of them
		addNodeRight();
		addNodeLeft();
		if (((node1->nodeID >> (level - 1)) & 1) == 0)
		{
			//put the first left and the second right
			lp->nodeInfo = *node1;
			rp->nodeInfo = *node2;
		}
		else
		{
			//put the first right and the second left
			rp->nodeInfo = *node1;
			lp->nodeInfo = *node2;
		}
		//set their sizes
		lp->tsize = 1;
		rp->tsize = 1;
	}

	void addNode(NodeDetails* val)
	{
		//adds a node to the tree
		tsize++;
		if (lp == NULL and rp == NULL)
		{
			//if both ways are blocked, add here
			if (nodeInfo.nodeID != 0 and (*val).nodeID != nodeInfo.nodeID)
			{
				//seperate the new and old values
				lowerLevels(&nodeInfo, val);
				nodeInfo = { 0 };
				return;
			}
			nodeInfo = *val;
		}
		else if (((val->nodeID >> (level - 1)) & 1) == 0)
		{
			//move to the left or add there
			if (lp == NULL)
				addNodeLeft();
			lp->addNode(val);
		}
		else
		{
			//move to the right or add there
			if (rp == NULL)
				addNodeRight();
			rp->addNode(val);
		}
	}

	void raiseLevels()
	{
		//stores the value that needs to go up
		if (lp == NULL and rp == NULL)
		{
			//if this is the one that needs to go up
			goesUp = nodeInfo;
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

	bool deleteNode(uint256_t *val)
	{
		//deletes the node with the id stored in val if it exists
		if (lp == NULL and rp == NULL)
		{
			//if both ways are blocked, try to delete here
			if (*val == nodeInfo.nodeID)
			{
				//if this is the right value, delete it
				nodeInfo = { 0 };
				tsize--;
				needUp = this;
				return true;
			}
			return false;
		}
		bool remove;
		if (((*val >> (level - 1)) & 1) == 0)
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
			needUp = this;
		return remove;
	}

	void changeAddress(NodeDetails* change)
	{
		//changes a nodes ip and port by its id
		if (lp == NULL and rp == NULL)
		{
			//if reached the end
			nodeInfo = *change;
			return;
		}
		//checks which way to move next
		if ((*change).nodeID >> (level - 1) == 1)
			rp->changeAddress(change);
		else
			lp->changeAddress(change);
	}
};

//the array of buckets
vector <NodeDetails> buckets[256];

//the tree of the nodes
Node* tree;

#define ULL unsigned long long

int getBucket(uint256_t id)
{
	//returns the bucket for node with certain id
	id ^= My_Id_As_Number;
	if (id >> (64 * 3) != 0)
		return 255 - _lzcnt_u64((ULL)(id >> (64 * 3) & 0xFFFFFFFFFFFFFFFF));
	if (id >> (64 * 2) != 0)
		return 255 - 64 - _lzcnt_u64((ULL)(id >> (64 * 2) & 0xFFFFFFFFFFFFFFFF));
	if (id >> 64 != 0)
		return 255 - 64 * 2 - _lzcnt_u64((ULL)(id >> 64 & 0xFFFFFFFFFFFFFFFF));
	return 255 - 64 * 3 - _lzcnt_u64((ULL)(id));
}

bool canEnterBucket(NodeDetails addnew)
{
	//returns whether the new node can enter the bucket
	//checks if the node appears in the bucket list
	int bucketNumber = getBucket(addnew.nodeID);
	for (int a = 0; a < buckets[bucketNumber].size(); a++)
		if (buckets[bucketNumber][a].nodeID == addnew.nodeID)
			return false;
	//checks if the bucket is full
	if (buckets[bucketNumber].size() >= Bucket_Size)
		return false;
	return true;
}

void removeFromBucket(uint256_t removeID)
{
	//remove the node from his bucket
	int place = getBucket(removeID), ind = -1;
	for (int a = 0; a < buckets[place].size(); a++)
		if (removeID == buckets[place][a].nodeID)
			ind = a;
	if (ind == -1)
		cout << "error Node_Supporter.cpp removeFromBucket" << '\n';
	swap(buckets[place][buckets[place].size() - 1], buckets[place][ind]);
	buckets[place].pop_back();
}

void eraseAll(Node* now)
{
	//erases all nodes in the subtree of now
	if (now == NULL)
		return;
	eraseAll(now->lp);
	eraseAll(now->rp);
	delete now;
}

void initTree()
{
	//initialize the data
	eraseAll(tree);
	tree = new Node(256, 0);
	for (int a = 0; a < 256; a++)
		buckets[a] = {};
}

void fillList(int howmany, uint256_t* val, vector <NodeDetails>* placeList)
{
	//wrapper function for findClose, allows it to change the place to put the nodes
	nodes = placeList;
	tree->findClose(howmany, val);
}

void addNodeToTree(NodeDetails *newNode)
{
	//adds a new node to the tree
	if (canEnterBucket(*newNode))
	{
		tree->addNode(newNode);
		buckets[getBucket((*newNode).nodeID)].push_back(*newNode);
	}
}

void removeNodeFromTree(uint256_t* val)
{
	//remove node from the tree
	needUp = NULL;
	bool remove = tree->deleteNode(val);
	if (!remove)
		cout << "there was no node removed" << '\n';
	if (remove)
	{
		//remove the node from the buckets list
		removeFromBucket(*val);

		if (needUp->tsize == 0)
		{
			//if there are no values needed to be raised
			if (needUp->level == 256)
			{
				//if the node to remove is the root, act accordingly
				eraseAll(needUp);
				initTree();
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
		}
		else
			cout << "error returned too big" << '\n';
	}
	else
		cout << "node wasnt there" << '\n';
}

int getTreeSize()
{
	//returns the size of the tree
	return tree->tsize;
}