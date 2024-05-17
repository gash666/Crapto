#include <map>
#include <queue>
#include <mutex>
#include "H_ECDSA.h"
#include "H_General_Functions.h"

using namespace std;

struct Hash256
{
	char value[32];
	char* dataMessage;

	bool operator<(const Hash256& other) const
	{
		return memcmp(value, other.value, 32) < 0;
	}
};

vector <pair <unsigned long long, unsigned long long>> times;
vector <int> sizesOfMessages;
vector <map <Hash256, unsigned long long>> mapForSame;
vector <queue <pair <Hash256, unsigned long long>>> queueForSame;
vector <mutex*> mutexVectorRecentMessages;

void addNewMapQueue(pair <unsigned long long, unsigned long long> time, int sizeOfMessage)
{
	//adds a new structure for checking if messages were already sent
	mapForSame.push_back({});
	queueForSame.push_back({});
	times.push_back(time);
	sizesOfMessages.push_back(sizeOfMessage);
	mutex* newMutex = new mutex();
	mutexVectorRecentMessages.push_back(newMutex);
}

void addMessageInd(char* message, unsigned long long time, int ind)
{
	//adds a message to a map and queue in an index
	lock_guard <mutex> lock(*mutexVectorRecentMessages[ind]);
	
	//initialize the new value to add
	Hash256 addData;
	addData.dataMessage = (char*) malloc(sizesOfMessages[ind]);
	copy(message, message + sizesOfMessages[ind], addData.dataMessage);
	SHA256(message, sizesOfMessages[ind], addData.value);

	//adds the data to the queue and map
	queueForSame[ind].push({ addData, time });
	mapForSame[ind].insert({ addData, time });
}

bool getFirstAdded(int ind, char* placeTheAnswer)
{
	//get the first added message
	lock_guard <mutex> lock(*mutexVectorRecentMessages[ind]);

	//remove the timed out messages from the first queue
	while (!queueForSame[ind].empty())
	{
		//get the time right now
		unsigned long long timeNow = Get_Time();

		//check if the last one is good
		map <Hash256, unsigned long long>::iterator iter = mapForSame[ind].find(queueForSame[ind].front().first);
		if (queueForSame[ind].front().second + times[ind].first >= timeNow)
			break;

		queueForSame[ind].pop();
		mapForSame[ind].erase(iter);
	}

	//check if the queue is not empty
	if (queueForSame[ind].empty())
		return false;

	copy(queueForSame[ind].front().first.dataMessage, queueForSame[ind].front().first.dataMessage + sizesOfMessages[ind], placeTheAnswer);
	return true;
}

void moveFirstLast(int ind)
{
	//moves the first added message to be last
	lock_guard <mutex> lock(*mutexVectorRecentMessages[ind]);

	//moves the first to be last
	auto temp = queueForSame[ind].front();
	queueForSame[ind].push(temp);
	queueForSame[ind].pop();
}

void popFrontInd(int ind, bool moveSecond)
{
	//removes the first element from the first queue
	lock_guard <mutex> lock(*mutexVectorRecentMessages[ind]);

	//removes the value from the queue
	map <Hash256, unsigned long long>::iterator iter = mapForSame[ind].find(queueForSame[ind].front().first);
	queueForSame[ind].pop();
	mapForSame[ind].erase(iter);
}

bool isAlreadyIn(char* message, int ind, bool isSHA256)
{
	//returns true if the data is already known
	lock_guard <mutex> lock(*mutexVectorRecentMessages[ind]);

	//initialize variable for finding the value
	Hash256 tempForSearch;
	tempForSearch.dataMessage = NULL;

	//get the SHA256 of the data to search for it
	if (isSHA256)
		copy(message, message + 32, tempForSearch.value); //the pointer is to the SHA256 value of the message
	else
		SHA256(message, sizesOfMessages[ind], tempForSearch.value); //the pointer is to the value before it is hashed

	//return the answer
	return mapForSame[ind].find(tempForSearch) != mapForSame[ind].end();
}

int getSizeInd(int ind)
{
	//returns the size of the first queue
	lock_guard <mutex> lock(*mutexVectorRecentMessages[ind]);
	return queueForSame[ind].size();
}

void reset(int ind)
{
	//clear the values in the queue and map
	lock_guard <mutex> lock(*mutexVectorRecentMessages[ind]);
	queueForSame[ind] = {};
	mapForSame[ind].clear();
}
