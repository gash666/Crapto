#include <iostream>
#include <bitset>
#include <iomanip>
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
    srand(time(NULL));
    int n;
    cin >> n;
    Is_Bootnode = false;
    if (n == 1)
        Is_Bootnode = true;
    wstring username;
    wcin >> username;
    if (!initValues(username))
        cout << "init went wrong" << '\n';
    closeSocket();
}