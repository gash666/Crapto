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
    chrono::milliseconds sleepNoMessageWaiting(50);
    int count = 0;
    char tempForClear[sizeof(Reveal)];

    while (true)
    {
        int messageLength = receiveMessage(Buffer_For_Receiving_Messages);
        if (messageLength != -1)
            handleMessage(Buffer_For_Receiving_Messages, messageLength);
        else
        {
            this_thread::sleep_for(sleepNoMessageWaiting);

            //clears old messages from memory
            count++;
            if (count == 400)
            {
                count = 0;
                for (int a = 0; a < 7; a++)
                    getFirstAdded(a, tempForClear);
            }
        }
    }
}

int main()
{
    srand((unsigned int) time(NULL));

    //check if the user should run locally or not
    cout << "Do you want the code to run locally?" << '\n';
    string ans;
    cin >> ans;
    if (ans == "YES")
    {
        char tempIp[4] = { (char)127, (char)0, (char)0, (char)1 };
        memcpy(Bootnode_Details[0].ip, tempIp, 4);
        Bootnode_Details[0].port = (unsigned short)51647;
    }
    else
    {
        //check if the ip and port of the bootnode should be the default
        cout << "Would you like to have the default ip and port for the bootnode?" << '\n';
        cin >> ans;
        if (ans == "YES")
        {
            //set the ip and port of the bootnode to be the default
            char tempIp[4] = { (char)77, (char)139, (char)1, (char)166 };
            memcpy(Bootnode_Details[0].ip, tempIp, 4);
            Bootnode_Details[0].port = (unsigned short)51647;
        }
        else
        {
            //get the ip of the bootnode
            cout << "Enter the ip of the bootnode" << '\n';
            cin >> ans;
            Convert_Ip_To_Char_Array(ans, Bootnode_Details[0].ip);

            //get the port of the bootnode
            cout << "Enter the port of the bootnode" << '\n';
            cin >> Bootnode_Details[0].port;
        }
    }

    //get the username
    cout << "Enter your username" << '\n';
    wstring username;
    wcin >> username;
    
    //update whether the user is the bootnode
    Is_Bootnode = false;
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
}