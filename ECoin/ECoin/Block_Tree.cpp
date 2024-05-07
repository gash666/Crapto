#include "H_Variables.h"
#include "H_Treap.h"
#include "H_Block_Tree.h"
#include "H_Network_Interface.h"
#include "H_General_Functions.h"
#include "H_Maintain_Blockchain.h"
#include <iomanip>

using namespace std;

BlockTreeNode* headNow = NULL;
BlockTreeNode* dfsSearch(BlockTreeNode* blockNow, char* shaOfBlockNow)
{
	//search for a certain block in the block tree
	//check if blockNow is not NULL
	if (blockNow == NULL)
		return NULL;

	//if this is the wanted block, return a pointer to it
	if (memcmp(blockNow->sha256OfBlock, shaOfBlockNow, 32) == 0)
		return blockNow;

	blockNow->isAprovedByThisUser;

	//search the subtree of this block for the wanted block
	for (int a = 0; a < blockNow->blocksAfterThis.size(); a++)
	{
		BlockTreeNode* temp = dfsSearch(blockNow->blocksAfterThis[a], shaOfBlockNow);
		if (temp != NULL)
			return temp;
	}

	//the block was not found here, return this result
	return NULL;
}

void dfsDelete(BlockTreeNode* blockNow, char* shaOfBlockNow)
{
	//leave a certain node as the head of the tree
	//if this is the block to become head, return
	if (memcmp(blockNow->sha256OfBlock, shaOfBlockNow, 32) == 0)
		return;

	//delete the subtree of this block
	for (int a = 0; a < blockNow->blocksAfterThis.size(); a++)
		dfsDelete(blockNow->blocksAfterThis[a], shaOfBlockNow);

	//delete this block node
	free(blockNow->startOfBlock);
	delete(blockNow);
}

BlockTreeNode* deepest;
int depthDeepest;
int depthSecondDeepest;

void dfsDeepest(BlockTreeNode* blockNow, int depthNow, BlockTreeNode* blockNotPass = NULL)
{
	//returns the deepest block that is confirmed by this user
	//initialize variable
	bool validChildren = false;

	//get the answer from the lower layers
	for (int a = 0; a < blockNow->blocksAfterThis.size(); a++)
	{
		//check if the block shouldn't be approved and set it
		if (Get_Time() > blockNow->blocksAfterThis[a]->timeUntilApproved)
			blockNow->blocksAfterThis[a]->isAprovedByThisUser = false;

		//check if the block is approved
		if (blockNow->blocksAfterThis[a]->isAprovedByThisUser and blockNow->blocksAfterThis[a] != blockNotPass)
		{
			validChildren = true;
			dfsDeepest(blockNow->blocksAfterThis[a], depthNow + 1, blockNotPass);
		}
	}

	//update the answer
	if (!validChildren)
	{
		depthSecondDeepest = depthDeepest + depthSecondDeepest + depthNow - max(depthNow, depthDeepest) - min(depthSecondDeepest, depthNow);
		if (depthNow > depthDeepest)
		{
			deepest = blockNow;
			depthDeepest = depthNow;
		}
	}
}

NodeDetails* whoToSendBlocks;

void dfsSendDetails(BlockTreeNode* blockNow)
{
	//send this block
	sendMessage(blockNow->startOfBlock, blockNow->sizeOfBlock, whoToSendBlocks->ip, whoToSendBlocks->port);

	//do this for all of this subtree
	for (int a = 0; a < blockNow->blocksAfterThis.size(); a++)
		dfsSendDetails(blockNow->blocksAfterThis[a]);
}

void dfsSetTimeValid(BlockTreeNode* blockNow, unsigned long long timeSet, unsigned long long timeNow)
{
	//sets the time the blocks proposed in certain times are valid for
	//set the time for this block
	if (((Block*)blockNow->startOfBlock)->TimeAtCreation == timeSet)
		blockNow->timeUntilApproved = timeNow + (timeNow - blockNow->timeBlockArrived) * Factor_Time_Approved_Until;

	//do this for all of this subtree
	for (int a = 0; a < blockNow->blocksAfterThis.size(); a++)
		dfsSetTimeValid(blockNow->blocksAfterThis[a], timeSet, timeNow);
}

void makeNewRoot(char* shaOfBlock)
{
	//makes the node with the given SHA256 value into the new head of the tree
	//get a pointer to the block
	BlockTreeNode* pointerToBlockNode = dfsSearch(headNow, shaOfBlock);

	//delete the blocks that are approved or will never be used
	dfsDelete(headNow, shaOfBlock);

	//set the confirmed node as the new head
	headNow = pointerToBlockNode;
	headNow->parentBlock = NULL;
}

pair <char*, int> getPathToNode(char* shaOfBlock)
{
	//get a pointer to the block
	BlockTreeNode* pointerToBlockNode = dfsSearch(headNow, shaOfBlock);

	//check if the tree has blocks in it
	if (headNow == NULL)
		return { NULL, 0 };

	//get a list of the approved blocks
	vector <BlockTreeNode*> blocksOnPath;
	while (pointerToBlockNode != NULL)
	{
		blocksOnPath.push_back(pointerToBlockNode);
		pointerToBlockNode = pointerToBlockNode->parentBlock;
	}

	//copy the values to an array
	char* ans = (char*)malloc(blocksOnPath.size() * sizeof(BlockTreeNode*));
	copy((char*)&blocksOnPath[0], (char*)&blocksOnPath[0] + sizeof(BlockTreeNode*) * blocksOnPath.size(), ans);
	return { ans, (int)blocksOnPath.size() };
}

void addBlock(char* shaOfParent, char* shaOfBlock, char* blockStart, int blockSize, bool isGood)
{
	//adds a block to the tree
	BlockTreeNode* pointerToBlockNodeParent = dfsSearch(headNow, shaOfParent);

	//check if the added block is not the first
	if (headNow != NULL and pointerToBlockNodeParent == NULL)
		return;

	//set the variables for the new block node
	BlockTreeNode* newBlockNode = new BlockTreeNode{};
	newBlockNode->startOfBlock = (char*) malloc(blockSize);
	copy(blockStart, blockStart + blockSize, newBlockNode->startOfBlock);
	newBlockNode->sizeOfBlock = blockSize;
	newBlockNode->parentBlock = pointerToBlockNodeParent;
	newBlockNode->isAprovedByThisUser = isGood;
	newBlockNode->timeBlockArrived = Get_Time();
	copy(shaOfBlock, shaOfBlock + 32, newBlockNode->sha256OfBlock);

	//add the block node to the list of nodes after his block parent
	if (headNow != NULL)
		pointerToBlockNodeParent->blocksAfterThis.push_back(newBlockNode);
	else
		headNow = newBlockNode;
}

pair <char*, int> getBlock(char* shaOfBlock)
{
	//returns the block if it is known to the user
	BlockTreeNode* answer = dfsSearch(headNow, shaOfBlock);
	if (answer == NULL)
		return { NULL, 0 };
	return { answer->startOfBlock, answer->sizeOfBlock };
}

void shouldSignBlock(char* shaOfBlock)
{
	//returns whether the asked block should be signed by this user
	//get the deepest block
	deepest = NULL;
	depthSecondDeepest = 0;
	depthDeepest = 0;
	dfsDeepest(headNow, 0);

	//check if there is a block that could be confirmed
	BlockTreeNode* maybeApprove = deepest;
	for (int a = 0; a < Number_Deepest_Approve and maybeApprove != NULL; a++)
		maybeApprove = maybeApprove->parentBlock;

	if (maybeApprove == NULL)
		return;

	//check if the new block to be confirmed will be unique
	if (depthSecondDeepest == depthDeepest or memcmp(shaOfBlock, deepest->sha256OfBlock, 32) != 0)
		return;

	//save the depth of the suggested block to be confirmed
	int depthTemp = depthDeepest - Number_Deepest_Approve;
	
	//get the deepest node without the block that will maybe be confirmed
	deepest = NULL;
	depthSecondDeepest = 0;
	depthDeepest = 0;
	dfsDeepest(headNow, 0, maybeApprove);

	if (depthDeepest <= depthTemp)
	{
		//check if this user is the creator of the block that is being approved
		if (memcmp(&My_Details, &((Block*)maybeApprove->startOfBlock)->BlockCreator, sizeof(NodeDetails)) == 0)
		{
			//set the sha256 of the block
			copy(maybeApprove->sha256OfBlock, maybeApprove->sha256OfBlock + 32, hashBlockCreating);

			//initialize variables
			deleteTreapAll(5);
			isThisUserCreating = true;

			//call the function that tries to collect signatures to approve the blocks
			post(ThreadPool, []() { tryApproveBlock(); });
		}

		//sends a message that approves the block
		Confirm_Block* ans = new Confirm_Block{};
		Handle_Confirm_Block_Create(maybeApprove->sha256OfBlock, ans);
		sendMessage((char*)ans, sizeof(Confirm_Block), ((Block*)maybeApprove->startOfBlock)->BlockCreator.ip, ((Block*)maybeApprove->startOfBlock)->BlockCreator.port);
	}
}

BlockTreeNode* createOnThisBlock()
{
	//returns the deepest valid block to be a parent of a new one
	deepest = NULL;
	depthSecondDeepest = -1;
	depthDeepest = -1;
	dfsDeepest(headNow, 0);
	return deepest;
}

void setTimeApproved(unsigned long long timeBlockCreated)
{
	//sets the time a block is valid until
	dfsSetTimeValid(headNow, timeBlockCreated, Get_Time());
}

void sendBlockTree(NodeDetails* senderDetails)
{
	//sends the whole block tree to another user
	whoToSendBlocks = senderDetails;
	dfsSendDetails(headNow);
}

void setShaOfHead(char* placeTheAnswer)
{
	//copies the SHA256 of the head of the block tree
	copy(headNow->sha256OfBlock, headNow->sha256OfBlock + 32, placeTheAnswer);
}
