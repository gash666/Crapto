#include <iostream>
#include <vector>
#include <queue>
#include <chrono>
#include <iomanip>
#include <map>
#include <mutex>
#include "H_Variables.h"
#include <windows.h>
#include "H_Node_Supporter.h"
#include "H_Constants.h"
#include "H_Network_Interface.h"
#include "H_Message_Structure.h"
#include "H_ECDSA.h"
#include "H_Maintain_Blockchain.h"

using namespace std;

unsigned long long Get_Time()
{
    //gets the number of milliseconds since jan 1970
    auto currentTimePoint = chrono::system_clock::now();
    auto durationSinceEpoch = currentTimePoint.time_since_epoch();
    int64_t millisecondsSinceEpoch = chrono::duration_cast<chrono::milliseconds>(durationSinceEpoch).count();
    return millisecondsSinceEpoch;
}

mutex canChangePingedQueue;
queue <pair <NodeDetails, unsigned long long>> sendPing;
map <NodeDetails, unsigned long long> returnedAnswerPing, waitForAnswer;
bool needExit = false;

//time to sleep if there are no ping not answered
chrono::milliseconds sleepNoPingWaiting(Sleep_No_Ping_Waiting);

void MonitorAfterPing()
{
    //a function that continuesly monitors the nodes that need to answer a ping
    while (true)
    {
        canChangePingedQueue.lock();
        if (!sendPing.empty())
        {
            //gets the next node that needs to answer the ping message
            pair <NodeDetails, unsigned long long> nodeNow = sendPing.front();
            sendPing.pop();
            canChangePingedQueue.unlock();

            //check if thread needs to quit
            if (needExit)
                return;

            //sleeps until the ping needs to be answered
            unsigned long long temp = Get_Time();
            if (nodeNow.second > temp)
                this_thread::sleep_for(chrono::milliseconds(nodeNow.second - temp));

            //checks if the node answered the ping in time
            canChangePingedQueue.lock();
            if (returnedAnswerPing[nodeNow.first] == 0)
            {
                //removes the node from the tree and map
                waitForAnswer.erase(nodeNow.first);
                canChangePingedQueue.unlock();
                removeNodeFromTreeInd(nodeNow.first.nodeID, 0);

                //ping the user again after deleting him
                Ask_Ping* message = new Ask_Ping{};
                Handle_Ask_Ping_Create(&(nodeNow.first), message);
                sendMessage((char*)message, sizeof(Ask_Ping), nodeNow.first.ip, nodeNow.first.port);
            }
            else
            {
                //remove the node from the maps
                returnedAnswerPing.erase(nodeNow.first);
                waitForAnswer.erase(nodeNow.first);
                canChangePingedQueue.unlock();
            }
        }
        else
        {
            //no ping to be checked, sleep until there might be one
            canChangePingedQueue.unlock();
            this_thread::sleep_for(sleepNoPingWaiting);
        }
    }
}

//time to wait if there are no nodes needed to potentially be pinged
chrono::milliseconds sleepDuration(Time_To_Ping - Time_Before);
queue <pair <NodeDetails, unsigned long long>> activeUsers;
mutex canTakeFromQueue;

void Update_Ping_Timer(NodeDetails* update)
{
    //updates the time for a node after he sent a message
    lock_guard <mutex> lock(canTakeFromQueue);
    lock_guard <mutex> lock1(canChangePingedQueue);

    //check if the node is in the tree or can be added to it
    if (!addNodeToTreeInd(update, 0))
        return;

    //update the time for the node in the tree
    unsigned long long timeIn = Get_Time();
    activeUsers.push({ *update , Time_To_Ping + timeIn });
    setTime(update, timeIn);

    //if the node was pinged, update that he is active
    if (waitForAnswer[*update] != 0)
        returnedAnswerPing[*update] = timeIn;
}

void MakeThreadsExit()
{
    //makes the threads for monitoring ping exit
    needExit = true;
}

void MonitorBeforePing()
{
    //a function that continuesly monitors the nodes that need to be pinged
    while (true)
    {
        canTakeFromQueue.lock();
        if (!activeUsers.empty())
        {
            //gets the next node to try and check if not active
            pair <NodeDetails, unsigned long long> nodeNow = activeUsers.front();
            activeUsers.pop();
            canTakeFromQueue.unlock();

            //check if thread needs to quit
            if (needExit)
                return;

            //sleeps until the node needs to be pinged
            unsigned long long temp = Get_Time();
            if (nodeNow.second > temp)
                this_thread::sleep_for(chrono::milliseconds(nodeNow.second - temp));

            //check if user really needs to be pinged
            if (nodeNow.second - Time_To_Ping == getLastTime(&(nodeNow.first)))
            {
                //sends the ping message
                Ask_Ping* message = new Ask_Ping{};
                Handle_Ask_Ping_Create(&(nodeNow.first), message);
                sendMessage((char*)message, sizeof(Ask_Ping), nodeNow.first.ip, nodeNow.first.port);

                //adds the node to the queue of pinged nodes
                canChangePingedQueue.lock();
                unsigned long long timeNow = Get_Time();
                sendPing.push({ nodeNow.first, timeNow + Response_Time });
                waitForAnswer[nodeNow.first] = (unsigned long long)(timeNow + Response_Time);
                canChangePingedQueue.unlock();
            }
        }
        else
        {
            //no node to be checked, can block until one can be needed to be pinged
            canTakeFromQueue.unlock();
            this_thread::sleep_for(sleepDuration);
        }
    }
}

chrono::milliseconds timeToWaitForResponse(Response_Time);
bool connectToBootnode(bool isBootnode)
{
    //connects to the bootnode and gets the external ip and port
    Connect* message = new Connect{};
    for (int a = 0; a < Tries_For_Each_Bootnode; a++)
        for (int b = 0; b < Number_Of_Bootnodes; b++)
        {
            //tries for the a + 1 time to connect through bootnode number b + 1
            //if this is a bootnode, dont try to connect to yourself
            if (isBootnode and memcmp(Bootnode_Details[b].nodeID, My_Details.nodeID, 32) == 0)
                continue;

            //send a message to try to connect
            Handle_Connect_Create(message);
            if (!sendMessage((char*)message, sizeof(Connect), Bootnode_Details[b].ip, Bootnode_Details[b].port))
                continue;

            //receive the message
            this_thread::sleep_for(timeToWaitForResponse);
            int messageLength = receiveMessage(Buffer_For_Receiving_Messages);
            if (messageLength != -1)
            {
                //the connect was successful
                handleMessage(Buffer_For_Receiving_Messages, messageLength);
                return true;
            }
        }

    //the connect was not successful
    return false;
}

void getClosest(char target[32], int triesImprove, NodeDetails* answer = NULL)
{
    //finds the closest nodes to a certain id and adds them to the tree
    //initialize and find the closest to the id that the user knows about
    NodeDetails closestNow[Bucket_Size], closestBefore[Bucket_Size];
    for (int a = 0; a < Bucket_Size; a++)
    {
        closestNow[a].port = 0;
        closestBefore[a].port = 0;
    }
    fillListInd(Bucket_Size, target, closestNow, 0);
    Ask_Close* ans = new Ask_Close{};

    //adds details about the thread running
    Communication_With_Threads threadDetails;
    threadDetails.threadMessageId = ANSWER_CLOSE;
    threadDetails.threadId = GetCurrentThreadId();
    mutex* newMutex = new mutex();
    threadDetails.canAccessThis = newMutex;
    threadDetails.whereToCompare = target;

    //creates a tree
    char indexForTree = occupyNewTree();
    threadDetails.whereToAnswer = &indexForTree;
    for (int a = 0; a < Bucket_Size; a++)
        if (closestNow[a].port != 0)
            addNodeToTreeInd(&closestNow[a], indexForTree);
    setHasSentInd(&My_Details, indexForTree);

    //adds the struct to the vector
    CanChangeCommWithThreads.lock();
    commWithThreadsDetails.push_back(threadDetails);
    CanChangeCommWithThreads.unlock();

    int timesSame = 0;

    //find the closest with the help of other users
    while (timesSame < triesImprove)
    {
        int numMessagesSent = 0;
        for (int a = 0; a < Bucket_Size; a++)
        {
            //checks if the node exists
            if (closestNow[a].port == 0)
                break;

            //check if the node already answered to this message
            if (!getHasSentInd(&closestNow[a], indexForTree))
            {
                //sends a message to the node
                numMessagesSent++;
                Handle_Ask_Close_Create(target, ans);
                sendMessage((char*)ans, sizeof(Ask_Close), closestNow[a].ip, closestNow[a].port);
            }
        }
        swap(closestBefore, closestNow);

        //wait for answers to arrive
        if (numMessagesSent != 0)
            this_thread::sleep_for(timeToWaitForResponse);

        fillListInd(Bucket_Size, target, closestNow, indexForTree);
        if (memcmp(closestNow, closestBefore, sizeof(NodeDetails) * Bucket_Size) != 0)
            timesSame = 0;
        else
            timesSame++;
    }

    //adds the closest nodes to the tree
    for (int a = 0; a < Bucket_Size; a++)
    {
        if (closestNow[a].port == 0)
            break;
        else if (memcmp(&closestNow[a], &My_Details, sizeof(NodeDetails)) != 0)
            Update_Ping_Timer(&closestNow[a]);
        if (answer != NULL and memcmp(&closestNow[a].nodeID, target, 32) == 0)
            copy(&closestNow[a], (NodeDetails*)((char*)&closestNow[a] + sizeof(NodeDetails)), answer);
    }

    //frees the tree so others could use it
    freeTree(indexForTree);

    //deletes the communication between this thead and the main thread
    CanChangeCommWithThreads.lock();
    for (int a = 0; a < commWithThreadsDetails.size(); a++)
        if (commWithThreadsDetails[a].threadMessageId == ANSWER_CLOSE and commWithThreadsDetails[a].whereToCompare == target)
        {
            //swaps it with the back and deletes it
            swap(commWithThreadsDetails[a], commWithThreadsDetails.back());
            commWithThreadsDetails.pop_back();
            break;
        }
    CanChangeCommWithThreads.unlock();
}

void discoverNodesInTheNetwork()
{
    //fills the bucket list with nodes from the network
    char targetForCloseTo[32];
    copy(My_Details.nodeID, My_Details.nodeID + 32, targetForCloseTo);
    getClosest(targetForCloseTo, Tries_Close_This_Node);
    int temp = getLastBucket();

    //asks about the closest for each bucket with nodes
    for (int a = temp; a >= 0; a--)
    {
        targetForCloseTo[(255 - a) / 8] ^= char (1 << ((7 - (256 - a) % 8)));
        post(ThreadPool, [targetForCloseTo] { getClosest((char*)targetForCloseTo, Tries_Close_Other_Node); });
        targetForCloseTo[(255 - a) / 8] ^= char (1 << ((7 - (256 - a) % 8)));
    }
}

void spreadMessage(char* message, int messageSize)
{
    //sends a message to other nodes so it would get to everyone
    //initialize variables
    NodeDetails closestToUser[Bucket_Size];
    for (int a = 0; a < Bucket_Size; a++)
        closestToUser[a].port = 0;

    //send the message to the closest nodes to this one
    fillListInd(Bucket_Size, My_Details.nodeID, closestToUser, 0);
    for (int a = 0; a < Bucket_Size; a++)
    {
        if (closestToUser[a].port == 0)
            break;
        sendMessage(message, messageSize, closestToUser[a].ip, closestToUser[a].port);
    }

    //send the message to one node from each bucket
    sendMessageBuckets(message, messageSize);
}

void spreadRandomNumbers(unsigned long long timeStartRandom, unsigned long long timeEndRandom, char hashOfContract[32])
{
    //spreads random numbers until the end of the contract
    //wait until the potential start of the contract
    if (timeStartRandom > Get_Time())
        this_thread::sleep_for(chrono::milliseconds(timeStartRandom - Get_Time()));

    while (Get_Time() < timeEndRandom)
    {
        //wait until it is time to reveal the next random number
        this_thread::sleep_for(chrono::milliseconds((Time_Block / 3) - Get_Time() % (Time_Block / 3)));

        //create reveal message spread it and process it
        Reveal* message = new Reveal{};
        Handle_Reveal_Create(&revealNumbers[indexLastReveal][0], hashOfContract, message);
        handleMessage((char*)message, sizeof(Reveal));
        indexLastReveal--;

        //wait until it is time to try to be the next block creator
        this_thread::sleep_for(chrono::milliseconds(Time_Block - Get_Time() % Time_Block));
        
        //try to be the next block creator
        tryCreateNextBlock();
    }

    //sleep until no help is needed from this random staking pool operator in approving blocks
    while (lastTimeApproved < timeEndRandom)
        this_thread::sleep_for(chrono::milliseconds(2000));

    hasContract = false;
    Is_Staking_Pool_Operator = false;
}

void handleStakingPoolOperator(unsigned long long timeStart, unsigned long long timeEnd)
{
    //sleep until the start of the contract
    if (Get_Time() < timeStart)
        this_thread::sleep_for(chrono::milliseconds(timeStart - Get_Time()));

    //wait until the user is not a staking pool operator
    while (Get_Time() < timeEnd)
    {
        //wait until it is time to try to be the next block creator
        this_thread::sleep_for(chrono::milliseconds(Time_Block - Get_Time() % Time_Block));

        //try to be the next block creator
        tryCreateNextBlock();
    }

    //sleep until no help is needed from this staking pool operator in approving blocks
    while (lastTimeApproved < timeEnd)
        this_thread::sleep_for(chrono::milliseconds(2000));

    hasContract = false;
    Is_Staking_Pool_Operator = false;
}

pair <char*, int> messageNow;
Answer_All_Info* m;
char* copyHere;

void initializeSendAllData(NodeDetails* whoAsked)
{
    //initializes the place to copy the data until it is sent
    messageNow = Handle_Answer_All_Info_Create(whoAsked);
    copyHere = messageNow.first + messageNow.second;
    m = (Answer_All_Info*)messageNow.first;
    m->howmanyEachType[0] = 0;
    m->howmanyEachType[1] = 0;
    m->howmanyEachType[2] = 0;
}

void copyDataToMessage(char* copyFrom, int copyAmount, int indexAdd)
{
    //copies the data and updates variables
    m->howmanyEachType[indexAdd]++;
    copy(copyFrom, copyFrom + copyAmount, copyHere);
    copyHere += copyAmount;
    messageNow.second += copyAmount;

    //check if the message is too long
    if (messageNow.second + 250 > Maximum_Message_Size)
    {
        signMessage((const unsigned char*)messageNow.first, messageNow.second, (unsigned char*)copyHere);
        sendMessage(messageNow.first, messageNow.second + 64, m->ReceiverDetails.ip, m->ReceiverDetails.port);

        //initialize variables for the next message
        m->howmanyEachType[0] = 0;
        m->howmanyEachType[1] = 0;
        m->howmanyEachType[2] = 0;
        messageNow.second = sizeof(Answer_All_Info);
        copyHere = messageNow.first + messageNow.second;
    }
}

void endSendAllData()
{
    //sends a message that ends 
    Answer_All_Info* m = (Answer_All_Info*)messageNow.first;
    m->isLast = true;
    signMessage((const unsigned char*)messageNow.first, messageNow.second, (unsigned char*)copyHere);
    sendMessage(messageNow.first, messageNow.second + 64, m->ReceiverDetails.ip, m->ReceiverDetails.port);
    free(messageNow.first);
}
