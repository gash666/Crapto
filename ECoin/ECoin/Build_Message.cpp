#include <iostream>
#include <vector>
#include <chrono>
#include <iomanip>
#include <array>
#include <algorithm>
#include "H_Variables.h"
#include "H_Constants.h"
#include "H_Node_Supporter.h"
#include "H_General_Functions.h"
#include "H_ECDSA.h"
#include "H_Message_Structure.h"
#include "H_Network_Interface.h"
#include "H_Maintain_Blockchain.h"
#include "H_Block_Tree.h"
#include "H_Treap.h"

using namespace std;

void Convert_Ip_To_Char_Array(string ip, char* save)
{
    //converts an ip from string to char array
    int place = 0;
    for (int a = 0; a < 4; a++)
    {
        int now = 0;
        while (place < ip.length() and ip[place] != '.')
        {
            now *= 10;
            now += ip[place] - '0';
            place++;
        }
        place++;
        save[a] = now;
    }
}

unsigned long long Get_Message_Number()
{
    //gets the number of milliseconds since jan 1970
    Number_Now++;
    if (Number_Now == 100000)
        Number_Now = 0;
    auto currentTimePoint = chrono::system_clock::now();
    auto durationSinceEpoch = currentTimePoint.time_since_epoch();
    int64_t millisecondsSinceEpoch = chrono::duration_cast<chrono::milliseconds>(durationSinceEpoch).count();

    return millisecondsSinceEpoch * 100000 + Number_Now;
}

void Handle_Connect_Create(Connect_0* ans)
{
    //creates the message to send in order to connect
    ans->messageId = CONNECT;
    ans->messageNumber = Get_Message_Number();
    copy(My_Details.nodeID, My_Details.nodeID + sizeof(NodeDetails::nodeID), ans->nodeId);
    signMessage((const unsigned char*)ans, offsetof(Connect_0, signature), (unsigned char*)ans->signature);
}

void Handle_Answer_Connect_Create(char* idAsk, Answer_Connect_1* ans)
{
    //creates the answer to send after receiving connect
    ans->messageId = ANSWER_CONNECT;
    ans->messageNumber = Get_Message_Number();
    copy((char*) & My_Details, ((char*)&My_Details + sizeof(NodeDetails)), (char*)&ans->senderDetails);
    pair <string, int> address = getAddress();
    Convert_Ip_To_Char_Array(address.first, ans->answerIdentity.ip);
    ans->answerIdentity.port = (short)address.second;
    copy(idAsk, idAsk + 32, ans->answerIdentity.nodeID);
    signMessage((const unsigned char*)ans, offsetof(Answer_Connect_1, signature), (unsigned char*)&(ans->signature));
}

void Handle_Ask_Close_Create(char* closeTo, Ask_Close_2* ans)
{
    //creates the question to ask who is close to a certain node
    ans->messageId = ASK_CLOSE;
    ans->messageNumber = Get_Message_Number();
    copy(&My_Details, (NodeDetails*)((char*)&My_Details + sizeof(NodeDetails)), &ans->senderDetails);
    copy(closeTo, closeTo + 32, ans->target);
    signMessage((const unsigned char*)ans, offsetof(Ask_Close_2, signature), (unsigned char*)&(ans->signature));
}

void Handle_Answer_Close_Create(char* closeTo, Answer_Close_3* ans)
{
    //creates a message that answers the question in the message received
    ans->messageId = ANSWER_CLOSE;
    ans->messageNumber = Get_Message_Number();
    copy(&My_Details, (NodeDetails*)((char*)&My_Details + sizeof(NodeDetails)), &ans->senderDetails);
    for (int a = 0; a < Bucket_Size; a++)
        ans->answerClose[a].port = 0;
    fillListInd(Bucket_Size, closeTo, ans->answerClose, 0);
    copy(closeTo, closeTo + 32, ans->target);
    signMessage((const unsigned char*)ans, offsetof(Answer_Close_3, signature), (unsigned char*)&(ans->signature));
}

void Handle_Ask_Ping_Create(NodeDetails* sendTo, Ask_Ping_4* ans)
{
    //creates a message that pings another user
    ans->messageId = ASK_PING;
    ans->messageNumber = Get_Message_Number();
    copy(&My_Details, (NodeDetails*)((char*)&My_Details + sizeof(NodeDetails)), &ans->senderDetails);
    copy(sendTo, (NodeDetails*)((char*)sendTo + sizeof(NodeDetails)), &ans->receiverDetails);
    signMessage((const unsigned char*)ans, offsetof(Ask_Ping_4, signature), (unsigned char*)&(ans->signature));
}

void Handle_Answer_Ping_Create(NodeDetails* sendTo, Answer_Ping_5* ans)
{
    //creates a message that answers a ping from another user
    ans->messageId = ANSWER_PING;
    ans->messageNumber = Get_Message_Number();
    copy(&My_Details, (NodeDetails*)((char*)&My_Details + sizeof(NodeDetails)), &ans->senderDetails);
    copy(sendTo, (NodeDetails*)((char*)sendTo + sizeof(NodeDetails)), &ans->receiverDetails);
    signMessage((const unsigned char*)ans, offsetof(Answer_Ping_5, signature), (unsigned char*)&(ans->signature));
}

void Handle_Pay_Create(NodeDetails* payTo, unsigned long long amountPay, Pay_6* ans)
{
    //creates a message that pays another user
    ans->messageId = PAY;
    ans->messageNumber = Get_Message_Number();
    ans->timeApply = Get_Time();
    ans->timeApply += 2 * Time_Block - ans->timeApply % Time_Block;
    copy(&My_Details, (NodeDetails*)((char*)&My_Details + sizeof(NodeDetails)), &ans->senderDetails);
    copy(payTo, (NodeDetails*)((char*)payTo + sizeof(NodeDetails)), &ans->receiverDetails);
    ans->amountToPay = amountPay;
    signMessage((const unsigned char*)ans, offsetof(Pay_6, signature), (unsigned char*)&(ans->signature));
}

pair <char*, int> Handle_Block_Create(char* shaOfParent, unsigned long long blockNumber)
{
    //creates the next block to be on the blockchain
    Block_7* ans = (Block_7*)malloc(Maximum_Message_Size);

    //check if the memory allocation worked
    if (ans == nullptr)
    {
        cout << "error in memory allocation while allocating memory for block creation" << '\n';
        return { NULL, 0 };
    }

    //initialize details about the block
    ans->messageId = BLOCK;
    copy(shaOfParent, shaOfParent + 32, ans->SHA256OfParent);
    ans->BlockNumber = blockNumber;
    ans->TimeAtCreation = Get_Time();
    ans->TimeAtCreation -= ans->TimeAtCreation % Time_Block;
    copy(&My_Details, (NodeDetails*)((char*)&My_Details + sizeof(NodeDetails)), &ans->BlockCreator);

    //set the new amount of money for the block creator
    ans->newAmountCreator = getAmountOfMoneyInd(&My_Details, 1) + Number_Coins_Per_Block;
    setAmountMoneyInd(&My_Details, ans->newAmountCreator, 1);

    //initialize a pointer to the place where the contents of the block starts
    char* tempPointer = (char*) ans + sizeof(Block_7);

    //add the random numbers that are revealed to the block
    ans->HowmanyFromEachType[0] = (unsigned char)copyDetailsFromTreapInd(2, tempPointer, ans->TimeAtCreation - Time_Block, true);
    tempPointer += ans->HowmanyFromEachType[0] * sizeof(Random_Reveal);

    //add the users who didn't send their random numbers to the block
    ans->HowmanyFromEachType[1] = (unsigned char)copyDetailsFromTreapInd(1, tempPointer, ans->TimeAtCreation - Time_Block, false);
    tempPointer += ans->HowmanyFromEachType[1] * sizeof(NodeDetails);

    //add the payments to the block
    int allIn = getSizeInd(0), a = 0;
    ans->HowmanyFromEachType[2] = Max_Number_Payments_Block;
    for (int t = 0; a < Max_Number_Payments_Block and t < allIn; a++, t++)
        if (getFirstAdded(0, tempPointer))
        {
            //calculate the new amounts of money of the sender and receiver
            unsigned long long moneySender = getAmountOfMoneyInd(&((Pay_6*)tempPointer)->senderDetails, 1) - ((Pay_6*)tempPointer)->amountToPay;
            unsigned long long moneyReceiver = getAmountOfMoneyInd(&((Pay_6*)tempPointer)->receiverDetails, 1) + ((Pay_6*)tempPointer)->amountToPay;

            //copy the new amounts of money
            copy((char*)&moneySender, (char*)&moneySender + sizeof(unsigned long long), tempPointer + sizeof(Pay_6));
            copy((char*)&moneyReceiver, (char*)&moneyReceiver + sizeof(unsigned long long), tempPointer + sizeof(Pay_6) + sizeof(unsigned long long));

            //check if the transaction is ok
            int result = checkPayment((Transaction*)tempPointer, ans->TimeAtCreation);

            //check if it will be ok in the future
            if (result == 1)
            {
                a--;
                moveFirstLast(0);
                continue;
            }

            //check if it will never be ok
            if (result == 0)
            {
                a--;
                popFrontInd(0);
                continue;
            }

            //remove this payment message from the queue and update the pointer
            popFrontInd(0);
            tempPointer += sizeof(Transaction);
        }
        else
            break;
    ans->HowmanyFromEachType[2] = a;

    //add the random staking pool operators binds to the block
    allIn = getSizeInd(2); a = 0;
    ans->HowmanyFromEachType[3] = Max_Number_Bind_Random_Staking_Pool_Operator_Block;
    for (int t = 0; a < Max_Number_Bind_Random_Staking_Pool_Operator_Block and t < allIn; a++, t++)
        if (getFirstAdded(2, tempPointer))
        {
            //check if the Bind_Random_Staking_Pool_Operator is ok
            int result = checkBindRandomStakingPoolOperator((Bind_Random_Staking_Pool_Operator_9*)tempPointer, ans->TimeAtCreation);

            //check if it will be ok in the future
            if (result == 1)
            {
                a--;
                moveFirstLast(2);
                continue;
            }

            //check if it will never be ok
            if (result == 0)
            {
                a--;
                popFrontInd(2);
                continue;
            }

            //update the pointer and remove the first message
            tempPointer += sizeof(Bind_Random_Staking_Pool_Operator_9);
            popFrontInd(2);
        }
        else
            break;
    ans->HowmanyFromEachType[3] = a;

    //add the staking pool operators binds to the block
    allIn = getSizeInd(3); a = 0;
    ans->HowmanyFromEachType[4] = Max_Number_Bind_Staking_Pool_Operator_Block;
    for (int t = 0; a < Max_Number_Bind_Staking_Pool_Operator_Block and t < allIn; a++, t++)
        if (getFirstAdded(3, tempPointer))
        {
            //check if the Bind_Staking_Pool_Operator is ok
            int result = checkBindStakingPoolOperator((Bind_Staking_Pool_Operator_10*)tempPointer, ans->TimeAtCreation);

            //check if it will be ok in the future
            if (result == 1)
            {
                a--;
                moveFirstLast(3);
                continue;
            }

            //check if it will never be ok
            if (result == 0)
            {
                a--;
                popFrontInd(3);
                continue;
            }

            //update the pointer and remove the first message
            tempPointer += sizeof(Bind_Staking_Pool_Operator_10);
            popFrontInd(3);
        }
        else
            break;
    ans->HowmanyFromEachType[4] = a;

    //sign the block
    signMessage((const unsigned char*)ans, tempPointer - (char*)ans, (unsigned char*)tempPointer);

    //revese the actions done while creating this block
    ReverseBlockUntil((char*)ans, (int)(tempPointer - (char*)ans + 64), { -1, -1 });

    //return a pointer to the start of the block and its size
    return { (char*) ans, (int)(tempPointer - (char*)ans) + 64 };
}

//message with id 8 - contract

void Handle_Bind_Random_Staking_Pool_Operator_Create(char* randomVal, unsigned long long timeStart, unsigned long long timeEnd, Bind_Random_Staking_Pool_Operator_9* ans)
{
    //creates a contract that binds a user to be a staking pool operator that produces random numbers
    ans->messageId = BIND_RANDOM_STAKING_POOL_OPERATOR;
    ans->untilTime = timeEnd;
    ans->startTime = timeStart;
    copy(randomVal, randomVal + 32, ans->randomValueCommit);
    copy(&My_Details, (NodeDetails*)((char*)&My_Details + sizeof(NodeDetails)), &ans->newStakingPoolOperator);
    signMessage((const unsigned char*)ans, offsetof(Bind_Random_Staking_Pool_Operator_9, signature), (unsigned char*)&(ans->signature));
}

void Handle_Bind_Staking_Pool_Operator_Create(unsigned long long timeStart, unsigned long long timeEnd, Bind_Staking_Pool_Operator_10* ans)
{
    //creates a contract that binds a user to be a staking pool operator that produces random numbers
    ans->messageId = BIND_RANDOM_STAKING_POOL_OPERATOR;
    ans->startTime = timeStart;
    ans->untilTime = timeEnd;
    copy(&My_Details, (NodeDetails*)((char*)&My_Details + sizeof(NodeDetails)), &ans->newStakingPoolOperator);
    signMessage((const unsigned char*)ans, offsetof(Bind_Staking_Pool_Operator_10, signature), (unsigned char*)&(ans->signature));
}

void Handle_Reveal_Create(char* randomVal, char* hashOfContract, Reveal_11* ans)
{
    //creates a message that reveals a random number
    ans->messageId = REVEAL;
    ans->timeWhenSend = Get_Time();
    ans->timeWhenSend -= ans->timeWhenSend % Time_Block;
    copy(&My_Details, (NodeDetails*)((char*)&My_Details + sizeof(NodeDetails)), &ans->senderDetails);
    copy(randomVal, randomVal + 32, ans->randomValue);
    copy(hashOfContract, hashOfContract + 32, ans->HashOfContract);
    signMessage((const unsigned char*)ans, offsetof(Reveal_11, signature), (unsigned char*)&(ans->signature));
}

void Handle_Confirm_Block_Create(char* blockHash, Confirm_Block_12* ans)
{
    //creates a message that confirms a block
    ans->messageId = CONFIRM_BLOCK;
    ans->messageNumber = Get_Message_Number();
    copy(&My_Details, (NodeDetails*)((char*)&My_Details + sizeof(NodeDetails)), &ans->senderDetails);
    signMessage((const unsigned char*)blockHash, 32, (unsigned char*)&(ans->BlockSignature));
    signMessage((const unsigned char*)ans, offsetof(Confirm_Block_12, signature), (unsigned char*)&(ans->signature));
}

pair <char*, int> Handle_Confirm_Block_All_Create()
{
    //creates a message that proves the block is valid
    Confirm_Block_All_13* ans = (Confirm_Block_All_13*)malloc(Signatures_For_Confirm_Message.size() * (sizeof(NodeDetails) + 64) + sizeof(Confirm_Block_All_13) + 64);

    //check if the memory allocation worked
    if (ans == nullptr)
    {
        cout << "error in memory allocation while allocating memory for block confirmation" << '\n';
        return { NULL, 0 };
    }

    //sets the variables
    ans->messageId = CONFIRM_BLOCK_ALL;
    ans->messageNumber = Get_Message_Number();
    copy(hashBlockCreating, hashBlockCreating + 32, ans->BlockHash);
    copy(&My_Details, (NodeDetails*)((char*)&My_Details + sizeof(NodeDetails)), &ans->senderDetails);
    ans->numberOfSignatures = (unsigned int) Signatures_For_Confirm_Message.size();

    //initializes a pointer
    char* tempPointer = (char*) ans + sizeof(Confirm_Block_All_13);

    //sets the signatures
    for (int a = 0; a < Signatures_For_Confirm_Message.size(); a++)
    {
        copy(&(Signatures_For_Confirm_Message[a].first), (NodeDetails*)((char*)&(Signatures_For_Confirm_Message[a].first) + sizeof(NodeDetails)), (NodeDetails*) tempPointer);
        tempPointer += sizeof(NodeDetails);
        copy(Signatures_For_Confirm_Message[a].second.begin(), Signatures_For_Confirm_Message[a].second.end(), tempPointer);
        tempPointer += 64;
    }

    //sign the message
    signMessage((const unsigned char*)ans, Signatures_For_Confirm_Message.size() * (sizeof(NodeDetails) + 64) + sizeof(Confirm_Block_All_13), (unsigned char*) tempPointer);

    return { (char*) ans, (int) (Signatures_For_Confirm_Message.size() * (sizeof(NodeDetails) + 64) + sizeof(Confirm_Block_All_13) + 64) };
}

void Handle_Ask_All_Info_Create(Ask_All_Info_14* ans, bool isAll)
{
    //creates a message that asks for all the information about the blockchain
    ans->messageId = ASK_ALL_INFO;
    ans->messageNumber = Get_Message_Number();
    ans->allOrNot = isAll;
    copy(&My_Details, (NodeDetails*)((char*)&My_Details + sizeof(NodeDetails)), &ans->senderDetails);
    signMessage((const unsigned char*)ans, offsetof(Ask_All_Info_14, signature), (unsigned char*)&(ans->signature));
}

pair <char*, int> Handle_Answer_All_Info_Create(NodeDetails* whoAsked)
{
    //initialize pointer
    Answer_All_Info_15* m = (Answer_All_Info_15*)malloc(Maximum_Message_Size);

    //check if the memory allocation worked
    if (m == nullptr)
    {
        cout << "error in memory allocation while allocating memory for Answer_All_Info message" << '\n';
        return { NULL, 0 };
    }

    //set variables
    m->messageId = ANSWER_ALL_INFO;
    m->messageNumber = Get_Message_Number();
    setShaOfHead(m->shaOfHeadBlock);
    copy(&My_Details, (NodeDetails*)((char*)&My_Details + sizeof(NodeDetails)), &m->senderDetails);
    copy(whoAsked, (NodeDetails*)((char*)whoAsked + sizeof(NodeDetails)), &m->ReceiverDetails);
    m->isLast = false;

    return { (char*)m, (int)sizeof(Answer_All_Info_15) };
}
