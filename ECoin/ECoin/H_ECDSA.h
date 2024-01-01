#pragma once
#ifndef ECDSAH
#define ECDSAH
#include <random>
#include <algorithm>
#include <sodium.h>
#include <openssl/rand.h>
#include <boost/multiprecision/cpp_int.hpp>

using namespace std;
using namespace boost::multiprecision;

//from ECDSA.cpp
void createKeys(unsigned char* public_key_buf, unsigned char* secret_key_buf);
void signMessage(const unsigned char* message, size_t message_len, unsigned char* signature);
bool verifySignature(const unsigned char* message, size_t message_len, const unsigned char* public_key, const unsigned char* signature);
unsigned char* getPublicKey();
unsigned char* getPrivateKey();

//from SHA256.cpp
string SHA256(string);

#endif