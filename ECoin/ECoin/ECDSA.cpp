#include <iostream>
#include <sodium.h>

unsigned char public_key[crypto_sign_PUBLICKEYBYTES];
unsigned char secret_key[crypto_sign_SECRETKEYBYTES];

void createKeys(unsigned char* public_key_buf, unsigned char* secret_key_buf)
{
    //function that generates public and private keys
    crypto_sign_keypair(public_key, secret_key);
    for (int a = 0; a < crypto_sign_PUBLICKEYBYTES; a++)
        public_key_buf[a] = public_key[a];
    for (int a = 0; a < crypto_sign_SECRETKEYBYTES; a++)
        secret_key_buf[a] = secret_key[a];
}

void signMessage(const unsigned char* message, size_t message_len, const unsigned char* secret_key, unsigned char* signature)
{
    //function that signs a message
    crypto_sign_detached(signature, nullptr, message, message_len, secret_key);
}

bool verifySignature(const unsigned char* message, size_t message_len, const unsigned char* public_key, const unsigned char* signature)
{
    //function that verifies a signature
    return crypto_sign_verify_detached(signature, message, message_len, public_key) == 0;
}

unsigned char* getPublicKey()
{
    //return the public key
    return public_key;
}

unsigned char* getPrivateKey()
{
    //return the private key
    return secret_key;
}