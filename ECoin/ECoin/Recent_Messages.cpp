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
vector <map <Hash256, pair <unsigned long long, bool>>> mapForSame;
vector <queue <pair <Hash256, unsigned long long>>> queueForSame1, queueForSame2;
vector <mutex*> mutexVectorRecentMessages;

void addNewMapQueue(pair <unsigned long long, unsigned long long> time, int sizeOfMessage)
{
	//adds a new structure for testing if messages were already sent
	mapForSame.push_back({});
	queueForSame1.push_back({});
	queueForSame2.push_back({});
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
	queueForSame1[ind].push({ addData, time });
	mapForSame[ind].insert({ addData, { time, true } });
}

bool getFirstAdded(int ind, char* placeTheAnswer)
{
	//get the first added message
	lock_guard <mutex> lock(*mutexVectorRecentMessages[ind]);

	//remove the timed out messages from the first queue
	while (!queueForSame1[ind].empty())
	{
		//get the time right now
		unsigned long long timeNow = Get_Time();

		//check if the last one is good
		map <Hash256, pair <unsigned long long, bool>>::iterator iter = mapForSame[ind].find(queueForSame1[ind].front().first);
		if (queueForSame1[ind].front().second + times[ind].first >= timeNow and iter != mapForSame[ind].end() and (*iter).second.second)
			break;

		//moves the data to the second queue and deletes it from the first queue
		queueForSame2[ind].push(queueForSame1[ind].front());
		queueForSame1[ind].pop();
	}

	//remove the timed out messages from the second queue
	while (!queueForSame2[ind].empty())
	{
		//get the time right now
		unsigned long long timeNow = Get_Time();

		//initialize variables
		bool wasin = false;
		map <Hash256, pair <unsigned long long, bool>>::iterator it;

		//check if the last one is good
		if (queueForSame2[ind].front().second + times[ind].first + times[ind].second >= timeNow)
		{
			it = mapForSame[ind].find(queueForSame2[ind].front().first);
			if (it != mapForSame[ind].end())
				break;
			wasin = true;
		}

		//makes sure that the iterator is initialized
		if (!wasin)
			it = mapForSame[ind].find(queueForSame2[ind].front().first);

		//deletes the data from the first queue
		queueForSame2[ind].pop();
		if (it == mapForSame[ind].end())
			mapForSame[ind].erase(it);
	}

	//check if the queue is not empty
	if (queueForSame1[ind].empty())
		return false;

	copy(queueForSame1[ind].front().first.dataMessage, queueForSame1[ind].front().first.dataMessage + sizesOfMessages[ind], placeTheAnswer);
	return true;
}

void moveFirstLast(int ind)
{
	//moves the first added message to be last
	lock_guard <mutex> lock(*mutexVectorRecentMessages[ind]);

	//moves the first to be last
	auto temp = queueForSame1.front();
	queueForSame1.push_back(temp);
	queueForSame1.pop_back();
}

void popFrontInd(int ind, bool moveSecond)
{
	//removes the first element from the first queue
	mutexVectorRecentMessages[ind]->lock();

	if (!queueForSame1[ind].empty())
	{
		//moves the value on the front to the second queue
		if (moveSecond)
			queueForSame2[ind].push(queueForSame1[ind].front());

		//removes the value from the queue
		queueForSame1[ind].pop();
	}

	mutexVectorRecentMessages[ind]->unlock();
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

void removeFromList(char* message, int ind, bool isSHA256)
{
	//removes a value from the map
	lock_guard <mutex> lock(*mutexVectorRecentMessages[ind]);

	//initialize variable for finding the value
	Hash256 tempForSearch;
	tempForSearch.dataMessage = NULL;

	//get the SHA256 of the data to search for it
	if (isSHA256)
		copy(message, message + 32, tempForSearch.value);
	else
		SHA256(message, sizesOfMessages[ind], tempForSearch.value);

	//deletes the value from the map
	auto it = mapForSame[ind].find(tempForSearch);
	if (it != mapForSame[ind].end())
		mapForSame[ind].erase(it);
}

void setWas(char* message, int ind, bool isSHA256)
{
	//set the value in map to be false
	lock_guard <mutex> lock(*mutexVectorRecentMessages[ind]);

	//initialize variable for finding the value
	Hash256 tempForSearch;
	tempForSearch.dataMessage = NULL;

	//get the SHA256 of the data to search for it
	if (isSHA256)
		copy(message, message + 32, tempForSearch.value);
	else
		SHA256(message, sizesOfMessages[ind], tempForSearch.value);

	//set the value in the map to be false
	auto it = mapForSame[ind].find(tempForSearch);
	if (it != mapForSame[ind].end())
		(*it).second.second = false;
}

bool getWas(char* message, int ind, bool isSHA256)
{
	//set the value in map to be false
	lock_guard <mutex> lock(*mutexVectorRecentMessages[ind]);

	//initialize variable for finding the value
	Hash256 tempForSearch;
	tempForSearch.dataMessage = NULL;

	//get the SHA256 of the data to search for it
	if (isSHA256)
		copy(message, message + 32, tempForSearch.value);
	else
		SHA256(message, sizesOfMessages[ind], tempForSearch.value);

	//return the value in the map
	auto it = mapForSame[ind].find(tempForSearch);
	if (it != mapForSame[ind].end())
		return (*it).second.second;

	return true;
}

int getSizeInd(int ind)
{
	//returns the size of the first queue
	lock_guard <mutex> lock(*mutexVectorRecentMessages[ind]);
	return queueForSame1[ind].size();
}

void reset(int ind)
{
	//clear the values in the queue and map
	lock_guard <mutex> lock(*mutexVectorRecentMessages[ind]);
	queueForSame1[ind] = {};
	mapForSame[ind].clear();
}
