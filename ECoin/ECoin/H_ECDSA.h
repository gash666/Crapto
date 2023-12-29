#pragma once
#include <random>
#include <algorithm>
#include <sodium.h>
#include <openssl/rand.h>
#include <boost/multiprecision/cpp_int.hpp>

using namespace std;
using namespace boost::multiprecision;

//from ECDSA.cpp
void createKeys(unsigned char*, unsigned char*);
void signMessage(const unsigned char* message, size_t message_len, const unsigned char* secret_key, unsigned char* signature);
bool verifySignature(const unsigned char*, size_t, const unsigned char*, const unsigned char*);
pair <uint256_t, uint256_t> getPublicKey();
unsigned char* getPrivateKey();

//from SHA256.cpp
string SHA256(string);