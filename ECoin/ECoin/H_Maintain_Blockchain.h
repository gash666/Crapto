#pragma once

#include "H_Message_Structure.h"

using namespace std;

void ReverseBlockUntil(char* message, int len, pair <int, int> untilWhere);
int checkPayment(Transaction* paymentTransaction, unsigned long long time, bool isConfirmed = false);// 0 - wrong, 1 - maybe in the future, 2 - ok now
int checkBindStakingPoolOperator(Contract* check, unsigned long long time, bool isConfirmed = false);// 0 - wrong, 1 - maybe in the future, 2 - ok now
int checkBindRandomStakingPoolOperator(Contract_Random* check, unsigned long long time, bool isConfirmed = false);// 0 - wrong, 1 - maybe in the future, 2 - ok now
bool checkRandomReveal(NodeDetails* sender, char* newRandomValue, unsigned long long timeSent, char* hashOfContract = NULL);
void initializeTreapsAll();
void applyOneBlock(char* startOfBlock, int sizeOfBlock);
void applyBlockFake(char* shaOfBlock);
void applyBlockReal(char* shaOfBlock);
void reversePath(char* shaOfBlock, bool include = true);
void tryApproveBlock();
void tryCreateNextBlock();
