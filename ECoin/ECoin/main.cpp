#include <iostream>
#include <sodium.h>
#include <random>
#include "Header.h"
using namespace std;

int main() 
{
    // Initialize libsodium
    if (sodium_init() < 0) 
    {
        std::cerr << "Failed to initialize libsodium." << '\n';
        return 1;
    }

    // create the keys
    unsigned char public_key[crypto_sign_PUBLICKEYBYTES];
    unsigned char secret_key[crypto_sign_SECRETKEYBYTES];
    createKeys(public_key, secret_key);
    int counti = 0;
    while (true)
    {
        string message = "its working!";
        //sign the message
        unsigned char signature[64];
        signMessage(reinterpret_cast<const unsigned char*>(message.c_str()), message.length(), secret_key, signature);

        // Verify the signature
        if (!verifySignature(reinterpret_cast<const unsigned char*>(message.c_str()), message.length(), public_key, signature))
            std::cerr << "Signature verification failed." << '\n';
        counti++;
        if (counti % 100 == 0)
            cout << counti << '\n';
    }
    return 0;
}