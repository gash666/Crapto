#include "H_Message_Structure.h"
#pragma once

//max attempts to bind to a port
#define Max_Attempts 30

//attempts to improve when searching for closest to this node
#define Tries_Close_This_Node 4

//attempts to improve when searching for closest to other node
#define Tries_Close_Other_Node 2

//minimum message size
#define Minimum_Message_Size sizeof(Connect)

//bucket size for kademlia
#define Bucket_Size 5

//exit code for functions run on other threads
#define Need_Exit 0

//time for the bot to sleep between each payment
#define time_To_Sleep_Bot 8000

//maximum amount for the bot to pay
#define Maximum_Bot_Payment 100

//time in milliseconds that passes from the last message from a user until you ping him
#define Time_To_Ping 60000

//time for thread to try and block in the mutex before it needs to read about a ping
#define Time_Before 1000

//general time to wait for an answer to a message
#define Response_Time 1000

//time to sleep if no pings are waiting to be answered right now
#define Sleep_No_Ping_Waiting 100

//number of times to send message to each bootnode before deciding the network is down
#define Tries_For_Each_Bootnode 5

//the number of threads in the thread pool
#define Number_Of_Threads 6

//size of database file
#define Data_Base_File_Size 2 * 64 + 2 * 32 + 2 * sizeof(unsigned long long) + 2

//number of random values
#define Number_Random_Values 1000

//time in milliseconds for message to be valid
#define Time_Message_Valid 400000

//time in milliseconds for message to spread
#define Max_Time_Spread 10000

//how much time between blocks in milliseconds
#define Time_Block 30000

//amount of time that can be related to as infinite
#define Infinite_Time 1000000000000000000

//the number of coins created in each block
#define Number_Coins_Per_Block 1000000000

//the minimum amount of coins in order to become a staking pool operator
#define Min_Coins_Staking_Pool_Operator 10000000000

//the minimum amount of coins in order to become a random staking pool operator
#define Min_Coins_Random_Staking_Pool_Operator 10000000000000

//the amount of money the bootnode starts with
#define Bootnode_Start_Money 100000000000000

//sets the number of minutes the bootnode is run for
#define Number_Minutes_Bootnode_Active 1440

//the maximum number of binds of random staking pool operators per block
#define Max_Number_Bind_Random_Staking_Pool_Operator_Block 5

//the maximum number of binds of staking pool operators per block
#define Max_Number_Bind_Staking_Pool_Operator_Block 15

//the maximum number of payments per block
#define Max_Number_Payments_Block 150

//maximum message size
#define Maximum_Message_Size sizeof(Block) + Max_Number_Payments_Block * sizeof(Transaction) + Max_Number_Bind_Staking_Pool_Operator_Block * sizeof(Contract) + Max_Number_Bind_Random_Staking_Pool_Operator_Block * sizeof(Contract_Random) + 256 * sizeof(Random_Reveal) + 64

//punishment for not revealing random numbers
#define Punishment_Not_Reveal 20 * unsigned long long(Number_Coins_Per_Block)

//the factor to multiply the time difference between the first random number received and the proposed block
#define Factor_Time_Approved_Until 80

//the number of blocks need to be more deep than others to be approved
#define Number_Deepest_Approve 7
