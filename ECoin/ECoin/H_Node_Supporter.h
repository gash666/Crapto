#pragma once
#include <iostream>
#include <boost/multiprecision/cpp_int.hpp>

using namespace std;
using namespace boost::multiprecision;

//from Node_Supporter.cpp
struct NodeDetails
{
	uint256_t nodeID;
	string ip;
	int port;
};

struct NodeDetails;
void initTree();
void fillList(int, uint256_t*, vector <NodeDetails>*);
void addNodeToTree(NodeDetails*);
void removeNodeFromTree(uint256_t*);
int getTreeSize();