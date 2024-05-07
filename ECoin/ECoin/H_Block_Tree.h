#pragma once

#include <vector>
#include "H_Message_Structure.h"

using namespace std;

struct BlockTreeNode
{
	char* startOfBlock;
	int sizeOfBlock;
	vector <BlockTreeNode*> blocksAfterThis;
	BlockTreeNode* parentBlock;
	bool isAprovedByThisUser;
	unsigned long long timeBlockArrived;
	unsigned long long timeUntilApproved = Infinite_Time;
	char sha256OfBlock[32];
};

void makeNewRoot(char* shaOfBlock);
pair <char*, int> getPathToNode(char* shaOfBlock);
void addBlock(char* shaOfParent, char* shaOfBlock, char* blockStart, int blockSize, bool isGood);
pair <char*, int> getBlock(char* shaOfBlock);
void shouldSignBlock(char* shaOfBlock);
void setTimeApproved(unsigned long long timeBlockCreated);
void sendBlockTree(NodeDetails* senderDetails);
void setShaOfHead(char* placeTheAnswer);
BlockTreeNode* createOnThisBlock();
