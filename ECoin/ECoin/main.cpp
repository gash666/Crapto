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

/*uint256_t secureRandomNum()
{
    //returns cryptographic secure random number
    if (RAND_status() != 1)
        cout << "OpenSSL PRNG not properly initialized." << '\n';
    unsigned char temp[32];
    if (RAND_bytes(temp, 32) != 1)
        cout << "Error generating cryptographic random number." << '\n';
    uint256_t randomValue = uint256_t(0);
    for (int a = 0; a < 32; a++)
    {
        randomValue <<= 8;
        randomValue += uint256_t(temp[a]);
    }
    return randomValue;
}*/

bool comperasion(const NodeDetails& x, const NodeDetails& y) { return x.nodeID < y.nodeID; }

bool testTree(vector <uint256_t> numbers)
{
    vector <NodeDetails> nodes;
    int t = rand() % min((int)numbers.size(), 30);
    //uint256_t temp = secureRandomNum();
    //fillList(t, &temp, &nodes);
    /*sort(nodes.begin(), nodes.end(), comperasion);
    vector <uint256_t> numbersnow;
    for (int a = 0; a < numbers.size(); a++)
        numbersnow.push_back(numbers[a] ^ temp);
    sort(numbersnow.begin(), numbersnow.end());
    swap(numbersnow, numbers);
    numbersnow = {};
    for (int a = 0; a < t and a < numbers.size(); a++)
        numbersnow.push_back(numbers[a] ^ temp);
    sort(numbersnow.begin(), numbersnow.end());

    /*cout << '\n' << '\n';
    for (auto a : numbersnow)
        cout << hex << a << '\n';
    cout << '\n' << '\n';//

    bool good = true;
    for (int a = 0; a < t and a < nodes.size(); a++)
    {
        //cout << hex << numbersnow[a] << '\n';
        //cout << nodes[a].nodeID << '\n' << '\n' << '\n';
        if (numbersnow[a] != nodes[a].nodeID)
            good = false;
    }
    if (nodes.size() < t)
    {
        cout << "not enough returned" << '\n';
        return false;
    }
    return good;*/
    return true;
}

int main() 
{
    srand(time(NULL));

    int n;
    cin >> n;
    if (n == 1)
    {
        My_Ip[0] = 127;
        My_Ip[0] = 0;
        My_Ip[0] = 0;
        My_Ip[0] = 1;
        My_Port = 47832;
        Is_Bootnode = true;
        createKeys((unsigned char*)My_Id, (unsigned char*)My_Private_Key);

        if (initSocket(true, My_Port))
        {
            cout << "the ip is: " << getIp() << '\n';
            cout << "the port is: " << getPort() << '\n';
            vector <char> message;
            int t;
            while (true)
            {
                cin >> t;
                if (t == -1)
                    break;
                message = {};
                receiveMessage(&message);
                handleMessage(message.data(), message.size());
                cout << '\n';
            }
        }
        closeSocket();
    }
    else
    {
        if (initSocket(true))
        {
            cout << "the ip is: " << getIp() << '\n';
            cout << "the port is: " << getPort() << '\n';
            initValues();
        }
        closeSocket();
    }
    /*int n, counti = 0;
    srand(time(NULL));
    cin >> n;
    vector <uint256_t> numbers;
    initTree();
    for (int a = 0; a < n; a++)
    {
        uint256_t temp = secureRandomNum();
        NodeDetails tempi = { temp };
        addNodeToTree(&tempi);
        numbers.push_back(temp);
    }
    while (true)
    {
        int randomPlace = rand() % numbers.size();
        removeNodeFromTree(&numbers[randomPlace]);
        numbers.erase(randomPlace + numbers.begin());

        uint256_t temp = secureRandomNum();
        NodeDetails tempi = { temp };
        addNodeToTree(&tempi);
        numbers.push_back(temp);

        //if (numbers.size() != n or numbers.size() != getTreeSize())
        //    cout << "error!" << '\n';

        bool good = testTree(numbers);
        //if (!good)
        //    cout << "fail" << '\n';
        counti++;
        if (counti % 100 == 0)
            cout << counti << '\n';
    }
    // Initialize libsodium
    if (sodium_init() < 0) 
    {
        std::cerr << "Failed to initialize libsodium." << '\n';
        return 1;
    }

    // create the keys
    unsigned char public_key[crypto_sign_PUBLICKEYBYTES];
    unsigned char secret_key[crypto_sign_SECRETKEYBYTES];
    createKeys(public_key, secret_key);
    int counti = 0;
    while (true)
    {
        string message = "its working!";
        message += to_string(counti);
        //sign the message
        unsigned char signature[64];
        signMessage(reinterpret_cast<const unsigned char*>(message.c_str()), message.length(), secret_key, signature);

        // Verify the signature
        if (!verifySignature(reinterpret_cast<const unsigned char*>(message.c_str()), message.length(), public_key, signature))
            std::cerr << "Signature verification failed." << '\n';
        counti++;
        if (counti % 500 == 0)
            cout << counti << '\n';
    }
    return 0;*/
}