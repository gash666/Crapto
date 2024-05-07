#include <iostream>
#include <bitset>
#include <iomanip>
#include <random>
#include "H_Variables.h"
#include "H_ECDSA.h"
#include "H_Node_Supporter.h"
#include "H_Network_Interface.h"
#include "H_General_Functions.h"
#include "H_Message_Structure.h"
#include "H_Treap.h"
#include "H_Block_Tree.h"
#include <deque>

using namespace std;

void runAllTheTime()
{
    chrono::milliseconds sleepNoMessageWaiting(75);

    while (true)
    {
        int messageLength = receiveMessage(Buffer_For_Receiving_Messages);
        if (messageLength != -1)
            handleMessage(Buffer_For_Receiving_Messages, messageLength);
        else
            this_thread::sleep_for(sleepNoMessageWaiting);
    }
}

int main()
{
    //second to show
    /*
    srand((unsigned int)time(NULL));
    initTreap();
    deque <pair <NodeDetails, unsigned long long>> inTheTree;
    for (int a = 0; a < 300; a++)
    {
        NodeDetails temp;
        for (int a = 0; a < sizeof(NodeDetails); a++)
            ((char*)&temp)[a] = rand() % 256;
        inTheTree.push_back({ temp, (unsigned long long)rand() });
        addToTree(&temp, inTheTree.back().second, 0, 0, NULL);
        updateCoinAmount(&temp, inTheTree.back().second, 0);
    }
    int counti = 0;
    while (true)
    {
        auto answer = inTheTree.front();
        removeNode(&inTheTree.front().first, 0);
        inTheTree.pop_front();

        NodeDetails tempi;
        for (int a = 0; a < sizeof(NodeDetails); a++)
            ((char*)&tempi)[a] = rand() % 256;
        inTheTree.push_back({ tempi, (unsigned long long) rand() });
        addToTree(&tempi, inTheTree.back().second, 0, 0, NULL);
        updateCoinAmount(&tempi, inTheTree.back().second, 0);

        int randomNum = rand() % inTheTree.size();
        if (inTheTree[randomNum].second != askNumberOfCoins(&inTheTree[randomNum].first, 0))
        {
            cout << counti << '\n';
            cout << inTheTree[randomNum].second << " " << askNumberOfCoins(&inTheTree[randomNum].first, 0) << '\n';
            return 0;
        }

        if (counti % 100000 == 0)
            cout << counti << '\n';
        counti += 4;
    }//*/
    //*
    //first to show
    srand((unsigned int) time(NULL));
    Is_Bootnode = false;
    wstring username;
    wcin >> username;
    if (username == L"bootnode")
        Is_Bootnode = true;
    if (!initValues(username))
        cout << "init went wrong" << '\n';
    else if (isFirstAll)
    {
        //start receiving messages
        post(ThreadPool, []() { runAllTheTime(); });

        //wait until the messages are received
        this_thread::sleep_for(chrono::milliseconds(1000));

        //create the block
        char hashAllZero[32]{ 0 };

        pair <char*, int> temp = Handle_Block_Create(hashAllZero, 100000);

        //calculate the hash of the new block
        char blockHash[32];
        SHA256(temp.first, temp.second, blockHash);

        //add the block to the tree block
        addBlock(hashAllZero, blockHash, temp.first, temp.second, true);

        //start receiving commands
        processCommands();
    }
    else
    {
        post(ThreadPool, []() { runAllTheTime(); });
        processCommands();
    }

    closeSocket();
    //*/
}