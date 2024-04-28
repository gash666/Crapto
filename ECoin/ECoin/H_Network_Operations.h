#pragma once
#include "H_Node_Supporter.h"

void Update_Ping_Timer(NodeDetails* update);
void MakeThreadsExit();
void MonitorBeforePing();
void MonitorAfterPing();
bool connectToBootnode(bool isBootnode);
void getClosest(char target[32], int triesImprove, NodeDetails* answer = NULL);
void discoverNodesInTheNetwork();
void spreadMessage(char* message, int messageSize);
void spreadRandomNumbers(unsigned long long timeStartRandom, unsigned long long timeEndRandom, char hashOfContract[32]);
void handleStakingPoolOperator(unsigned long long timeEnd);
void initializeSendAllData(NodeDetails* whoAsked);
void copyDataToMessage(char* copyFrom, int copyAmount, int indexAdd);
void endSendAllData();
