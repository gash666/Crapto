#pragma once
#include <random>
#include <algorithm>

#ifndef SODIUM
#define SODIUM
#define SODIUM_STATIC
#include "sodium.h"
#endif

using namespace std;

//from ECDSA.cpp
void createKeys(unsigned char* public_key_buf, unsigned char* secret_key_buf);
void signMessage(const unsigned char* message, size_t message_len, unsigned char* signature);
bool verifySignature(const unsigned char* message, size_t message_len, const unsigned char* public_key, const unsigned char* signature);
unsigned char* getPublicKey();
unsigned char* getPrivateKey();
void setKey(unsigned char* pubKey, unsigned char* privKey);

//from SHA256.cpp
void SHA256(char* input, int inputLen, char* output);
