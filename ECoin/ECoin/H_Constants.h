#pragma once

//max attempts to bind to a port
#define Max_Attempts 30

//maximum message size
#define Maximum_Message_Size 1024

//minimum message size
#define Minimum_Message_Size sizeof(Connect_0)

//bucket size for kademlia
#define Bucket_Size 10

//constant so that threads know when to quit
#define Need_Exit 0

//time in milliseconds that passes from the last message from a user until you ping him
#define Time_To_Ping 20000

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
#define Data_Base_File_Size 2 * 32 + 2 * 64 + 2 * sizeof(double) + 2
