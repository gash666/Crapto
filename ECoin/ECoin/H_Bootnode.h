#pragma once
#ifndef BOOTNODEH
#define BOOTNODEH
#include <iostream>
#include "H_Node_Supporter.h"

//number of bootnodes
#define Number_Of_Bootnodes 1

//details about the bootnodes
extern NodeDetails Bootnode_Details[Number_Of_Bootnodes] = { {} };

//saves true if the user is the bootnode
extern bool Is_Bootnode = false;

#endif