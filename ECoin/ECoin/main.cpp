#include <iostream>
#include <bitset>
#include <random>
#include "H_ECDSA.h"
#include "H_Node_Supporter.h"
#include "H_Network_Interface.h"
#include "H_Init_Functions.h"
#include "H_Message_Structure.h"
#include "H_Variables.h"

using namespace std;

int main() 
{
    int n;
    cin >> n;
    if (n == 1)
    {
        My_Ip[0] = 77;
        My_Ip[1] = 139;
        My_Ip[2] = 1;
        My_Ip[3] = 166;
        My_Port = 51648;
        Is_Bootnode = true;
        createKeys((unsigned char*)My_Id, (unsigned char*)My_Private_Key);

        if (initSocket(true, My_Port))
        {
            cout << "the port is: " << getPort() << '\n';
            vector <char> message;
            int t;
            while (true)
            {
                message = {};
                receiveMessage(&message);
                handleMessage(message.data(), message.size());
            }
        }
        closeSocket();
    }
    else
    {
        if (initSocket(true))
        {
            cout << "the port is: " << getPort() << '\n';
            wstring username;
            wcin >> username;
            if (!initValues(username))
                cout << "init went wrong" << '\n';
        }
        closeSocket();
    }
}