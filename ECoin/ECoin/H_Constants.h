#pragma once
#ifndef CONSTANTSH
#define CONSTANTSH
//max attempts to bind to a port
#define Max_Attempts 30

//maximum message size
#define Maximum_Message_Size 1024

//minimum message size
#define Minimum_Message_Size 105

//bucket size for kademlia
#define Bucket_Size 15

//size of database file
#define Data_Base_File_Size 2 * 32 + 2 * 64 + 2 * sizeof(double) + 2

#endif
