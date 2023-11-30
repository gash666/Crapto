#pragma once
#include <boost/multiprecision/cpp_int.hpp>

using namespace std;
using namespace boost::multiprecision;

string SHA256(string);
void createKeys(unsigned char*, unsigned char*);
uint256_t quickPower(uint512_t, uint256_t, uint256_t);
void signMessage(const unsigned char* message, size_t message_len, const unsigned char* secret_key, unsigned char* signature);
bool verifySignature(const unsigned char*, size_t, const unsigned char*, const unsigned char*);
pair <uint256_t, uint256_t> getPublicKey();