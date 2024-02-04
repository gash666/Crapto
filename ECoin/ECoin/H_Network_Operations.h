#pragma once
#include "H_Node_Supporter.h"

void Update_Ping_Timer(NodeDetails* update);
void MakeThreadsExit();
void MonitorBeforePing();
void MonitorAfterPing();
bool connectToBootnode(bool isBootnode);
void discoverNodesInTheNetwork();
