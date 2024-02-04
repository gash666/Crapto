#pragma once
#include <iostream>
#include "H_Node_Supporter.h"
#include "H_Constants.h"
#include "H_Message_Structure.h"

//the user's details
extern NodeDetails My_Details;

//the node's private key
extern char My_Private_Key[64];

//the number of the message, added to the milliseconds count times 10,000
extern long long Number_Now;

//the number of coins the user holds
extern double Number_Coins;

//number of bootnodes
#define Number_Of_Bootnodes 1

//details about the bootnodes
extern NodeDetails Bootnode_Details[Number_Of_Bootnodes];

//buffer for receiving messages
extern char Buffer_For_Receiving_Messages[Maximum_Message_Size];

//saves true if the user is the bootnode
extern bool Is_Bootnode;

//a vector for communication between threads
extern vector <Communication_With_Threads> commWithThreadsDetails;

//mutex for changing the vector for communication between threads
extern mutex CanChangeCommWithThreads;

//node that is full with zeroes for comparing
extern NodeDetails zeroNode;
