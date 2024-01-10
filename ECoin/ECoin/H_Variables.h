#pragma once
#ifndef VARIABLESH
#define VARIABLESH
#include <iostream>
#include <boost/multiprecision/cpp_int.hpp>
#include "H_Node_Supporter.h"

//the node's id as a uint256_t variable
extern uint256_t My_Id_As_Number;

//the node's id
extern char My_Id[32];

//the node's private key
extern char My_Private_Key[64];

//the node's ip
extern char My_Ip[4];

//the node's port
extern unsigned short My_Port;

//the number of the message, added to the milliseconds count times 10,000
extern long long Number_Now;

//the number of coins the user holds
extern double Number_Coins;

//number of bootnodes
#define Number_Of_Bootnodes 1

//structure for easier storage of data about nodes
struct NodeData
{
	unsigned short nodePort;
	string nodeIp;
	char nodeId[32];
};

//details about the bootnodes
extern NodeData Bootnode_Details[Number_Of_Bootnodes];

//saves true if the user is the bootnode
extern bool Is_Bootnode;

#endif