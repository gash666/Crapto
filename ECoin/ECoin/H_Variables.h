#pragma once
#include <boost/asio.hpp>
#include <iostream>
#include <array>
#include <queue>
#include "H_Node_Supporter.h"
#include "H_Constants.h"
#include "H_Message_Structure.h"

//the thread pool
extern boost::asio::thread_pool ThreadPool;

//the user's details
extern NodeDetails My_Details;

//the node's private key
extern char My_Private_Key[64];

//the number of the message, added to the milliseconds count times 10,000
extern long long Number_Now;

//the number of coins the user holds
extern unsigned long long Number_Coins;

//number of bootnodes
#define Number_Of_Bootnodes 1

//details about the bootnodes
extern NodeDetails Bootnode_Details[Number_Of_Bootnodes];

//buffer for receiving messages
extern char Buffer_For_Receiving_Messages[Maximum_Message_Size];

//saves true if the user is the bootnode
extern bool Is_Bootnode;

//saves true if the user is staking pool operator
extern bool Is_Staking_Pool_Operator;

//a vector for communication between threads
extern vector <Communication_With_Threads> commWithThreadsDetails;

//mutex for changing the vector for communication between threads
extern mutex CanChangeCommWithThreads;

//node that is full with zeroes for comparing
extern NodeDetails zeroNode;

//saves if the user has an ongoing contract
extern bool hasContract;

//saves if the user is one that creates random numbers
extern bool isCreatingRandom;

//saves random numbers to reveal for the blockchain
extern vector <array<char, 32>> revealNumbers;

//saves the last index that was revealed
extern int indexLastReveal;

//the hash of the block that this user is creating
extern char hashBlockCreating[32];

//saves if this user has lastly created a block
extern bool isThisUserCreating;

//the vector containing the signatures before copying them to a message
extern vector <pair <NodeDetails, array<char, 64>>> Signatures_For_Confirm_Message;

//queue containing users that asked for infotmation and might need more
extern deque <pair <NodeDetails, unsigned long long>> May_Need_Information;

//SHA256 of block that is approved by the bootnode when he sends all info
extern char ShaOfHeadBlockInitializing[32];

//saves the last time a random number was received
extern unsigned long long lastTimeReceivedRandom;

//mutex for using some actions on the block tree
extern mutex canUseBlockTreeActions;

//mutex for receiving reveal random numbers
extern mutex canReceiveRandom;

//saves if the user is a bootnode and the first one to connect
extern bool isFirstAll;

//saves if the user has the needed info to check if blocks are valid
extern bool hasInfo;

//saves the last time that a block was approved
extern unsigned long long lastTimeApproved;

//the last block number to be approved
extern unsigned long long blockNumberApproved;
