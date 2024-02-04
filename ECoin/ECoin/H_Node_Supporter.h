#pragma once
#include <iostream>
#include <vector>

using namespace std;

//from Node_Supporter.cpp
struct NodeDetails
{
	char nodeID[32];
	char ip[4];
	unsigned short port;

	//operator for map
	bool operator<(const NodeDetails& other) const
	{
		return memcmp(nodeID, other.nodeID, sizeof(nodeID)) < 0;
	}
};

struct Node
{
	int tsize = 0, level;
	bool hasSent = false;
	char value[32];
	NodeDetails nodeInfo = { 0 };
	Node* lp = NULL, * rp = NULL;
	struct Tree* aboveAll;

	Node(int lvl, char* val, Tree* head)
	{
		//intializes the node
		level = lvl;
		memcpy(value, val, 32);
		aboveAll = head;
	}

	void addToList();
	void findClose(int howmany, char* val);
	void addNodeRight();
	void addNodeLeft();
	void lowerLevels(NodeDetails* node1, NodeDetails* node2, bool firstSent);
	bool addNode(NodeDetails* val);
	void raiseLevels();
	bool deleteNode(char* val);
	void changeAddress(NodeDetails* change);
	void changeSent(NodeDetails* changeAbout);
	bool askSent(NodeDetails* askAbout);
};

struct Tree
{
	//the head of the tree
	bool checkBucket = false;
	Node* headOfTree;
	NodeDetails* nodes;
	Node* needUp;
	NodeDetails goesUp;
	bool goesUpHasSent;
	int placeNow = 0;

	void initTree(bool isNew);
	bool addNodeToTree(NodeDetails* newNode);
	void fillList(int howmany, char* val, NodeDetails* placeList);
	void removeNodeFromTree(char* val);
	bool getHasSent(NodeDetails* askAbout);
	void setHasSent(NodeDetails* changeAbout);
};

struct NodeDetails;
int occupyNewTree();
void freeTree(int ind);
void fillListInd(int howmany, char* val, NodeDetails* placeList, int ind);
bool addNodeToTreeInd(NodeDetails* newNode, int ind);
void removeNodeFromTreeInd(char* val, int ind);
int getTreeSizeInd(int ind);
unsigned long long getLastTime(NodeDetails* askAbout);
void setTime(NodeDetails* setAbout, unsigned long long timeToSet);
void setHasSentInd(NodeDetails* changeAbout, int ind);
bool getHasSentInd(NodeDetails* askAbout, int ind);