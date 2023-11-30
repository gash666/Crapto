#include <iostream>
#include <random>
#include <openssl/rand.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>
#include <openssl/bn.h>
#include <boost/multiprecision/cpp_int.hpp>

#include "Header.h"

using namespace boost::multiprecision;
using namespace std;

uint256_t gx("0x79be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798");
uint256_t gy("0x483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8");
uint256_t p("0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFC2F");
uint256_t n("0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141");
uint256_t privateKey;
pair <uint256_t, uint256_t> publicKey;

void getcoeff(int256_t number1, int256_t number2, pair <int256_t, int256_t>& coeffs)
{
	//compute the inverse using euclidean algorithm
	if (number2 % number1 == 0)
	{
		coeffs.first = 1;
		coeffs.second = 0;
		return;
	}
	int512_t d = number2 / number1;
	getcoeff(number2 % number1, number1, coeffs);
	int256_t temp = coeffs.first;
	coeffs.first = (uint256_t)((coeffs.second - (d * (uint512_t)coeffs.first) % p) + p) % p;
	coeffs.second = temp;
}

uint256_t calcInverse(uint256_t num, uint256_t mod)
{
	//calculates inverse of num modulu mod
	if (num == 0)
	{
		cout << "bug, not supposed to be 0." << '\n';
		return 0;
	}
	pair <int256_t, int256_t> coeffs;
	getcoeff(num % mod, mod, coeffs);
	return uint256_t(coeffs.first);
}

uint256_t quickPower(uint512_t a, uint256_t b, uint256_t m)
{
	//compute a ^ b % m quickly
	uint512_t mul = 1;
	while (b != 0)
	{
		if (b % 2)
		{
			mul *= a;
			mul %= m;
		}
		a *= a;
		a %= m;
		b /= 2;
	}
	return uint256_t(mul);
}

uint256_t secureRandomNum()
{
	//returns cryptographic secure random number
	if (RAND_status() != 1)
		cout << "OpenSSL PRNG not properly initialized." << '\n';
	unsigned char temp[32];
	if (RAND_bytes(temp, 32) != 1)
		cout << "Error generating cryptographic random number." << '\n';
	uint256_t randomValue = uint256_t(0);
	for (int a = 0; a < 32; a++)
	{
		randomValue <<= 8;
		randomValue += uint256_t(temp[a]);
	}
	return randomValue;
}

pair <uint256_t, uint256_t> addPoint(pair <uint512_t, uint512_t> point)
{
	//compute the other point that the tangent line intersects with
	uint512_t temp = p;
	uint512_t s = (3 * point.first % temp * point.first % temp) * (uint512_t)(quickPower(2 * point.second % temp, p - 2, p)) % temp;
	pair <uint512_t, uint512_t> pointC;
	pointC.first = (s * s % temp + 2 * temp - 2 * point.first) % temp;
	pointC.second = (point.second + (s * ((temp + pointC.first - point.first) % temp) % temp)) % temp;
	return { (uint256_t)pointC.first, (uint256_t)(p - pointC.second) };
}

pair <uint256_t, uint256_t> addPoints(pair <uint512_t, uint512_t> pointA, pair <uint512_t, uint512_t> pointB)
{
	//compute the other point that the line interstects with
	uint512_t temp = p;
	uint512_t s = (temp + pointB.second - pointA.second) % temp * (uint512_t)(quickPower((temp + pointB.first - pointA.first) % temp, p - 2, p)) % temp;
	pair <uint512_t, uint512_t> pointC;
	pointC.first = (s * s % temp + temp * 2 - pointA.first - pointB.first) % temp;
	pointC.second = (pointA.second + (s * ((temp + pointC.first - pointA.first) % temp) % temp)) % temp;
	return { (uint256_t)pointC.first, (uint256_t)(p - pointC.second) };
}

void multPoint(uint256_t p)
{
	//compute the origin point multiplied by the private key
	pair <uint256_t, uint256_t> doubling;
	doubling.first = gx;
	doubling.second = gy;
	bool was = false;
	while (p != 0)
	{
		if (p % 2 == 1)
		{
			if (!was)
			{
				publicKey.first = doubling.first;
				publicKey.second = doubling.second;
				was = true;
			}
			else
				publicKey = addPoints(publicKey, doubling);
		}
		doubling = addPoint(doubling);
		p >>= 1;
	}
}

pair <uint256_t, uint256_t> getPublicKey()
{
	//return the public key
	return publicKey;
}

unsigned char prKey[32];
unsigned char puKeyX[32];
unsigned char puKeyY[32];
unsigned char hashedData[32];
string strSign;
EC_KEY* ecKey = EC_KEY_new_by_curve_name(NID_secp256k1);
BIGNUM* privKey;

void preCalc()
{
	//reformat parameters
	uint256_t prKeyt = privateKey, puKeyXt = publicKey.first, puKeyYt = publicKey.second;
	for (int a = 31; a >= 0; a--, prKeyt >> 8, puKeyXt >> 8, puKeyYt >> 8)
	{
		prKey[a] = unsigned char(prKeyt & 255);
		puKeyX[a] = unsigned char(puKeyXt & 255);
		puKeyY[a] = unsigned char(puKeyYt & 255);
	}

	//set private key
	privKey = BN_bin2bn(prKey, sizeof(prKey), NULL);
	EC_KEY_set_private_key(ecKey, privKey);
	BN_free(privKey);

	//set public key
	EC_POINT* pubKey = EC_POINT_new(EC_KEY_get0_group(ecKey));
	BIGNUM* pubKeyX = BN_bin2bn(puKeyX, sizeof(puKeyX), NULL);
	BIGNUM* pubKeyY = BN_bin2bn(puKeyY, sizeof(puKeyY), NULL);
	EC_POINT_set_affine_coordinates_GFp(EC_KEY_get0_group(ecKey), pubKey, pubKeyX, pubKeyY, NULL);
	EC_KEY_set_public_key(ecKey, pubKey);
	EC_POINT_free(pubKey);
	BN_free(pubKeyX);
	BN_free(pubKeyY);

	//
}

void createKeys()
{
	//create private and public keys
	EC_KEY_free(ecKey);
	EVP_cleanup();
	uint256_t d;
	do
	{
		d = secureRandomNum();
	} while (d >= n or d == 0);
	do
	{
		privateKey = (uint256_t)d;
		multPoint(d);
	} while (publicKey.first == 0 or publicKey.second == 0);
	preCalc();
	cout << "public key: " << hex << publicKey.first << '\n' << hex << publicKey.second << '\n';
}

char convert64(int value)
{
	//converts the number to its base64 form
	if (value <= 25)
		return 'A' + value;
	if (value <= 51)
		return 'a' + value - 26;
	if (value <= 61)
		return '0' + value - 52;
	if (value == 62)
		return '+';
	return '/';
}

int getValueHex(char digit)
{
	//returns the value of a hex digit
	if ('0' <= digit and digit <= '9')
		return digit - '0';
	else if ('a' <= digit and digit <= 'f')
		return digit - 'a' + 10;
	return digit - 'A' + 10;
}

string hexToBase64(string hexs)
{
	//converts a string into base64 form
	string answer = "";
	while (hexs.length() != 64)
	{
		string temp = "0";
		temp += hexs;
		swap(temp, hexs);
	}
	if (hexs.length() % 3 != 0)
		hexs += '0';
	for (int a = 0; a < hexs.length(); a += 3)
	{
		answer += convert64(getValueHex(hexs[a]) * 4 + getValueHex(hexs[a + 1]) / 4);
		if (a + 2 < hexs.length())
			answer += convert64((getValueHex(hexs[a + 1]) & 3) * 16 + getValueHex(hexs[a + 2]));
	}
	while (answer.length() % 4 != 0)
		answer += '=';
	return answer;
}

string hexToPEM(const string& hexString, const string& pemType = "EC PRIVATE KEY")
{
	//convert string from hex into PEM
	string binaryData = hexToBase64(hexString);
	string pemData = "-----BEGIN " + pemType + "-----\n";
	pemData += binaryData + "\n";
	pemData += "-----END " + pemType + "-----\n";
	return pemData;
}

string turnToStr(uint256_t number)
{
	//turn uint256_t number to string
	stringstream stream;
	stream << hex << number;
	return stream.str();
}

string signMessage(string message)
{
	//return signature for message
	if (message != "")
	{
		strSign = SHA256(message);
		for (int a = 0; a < 32; a++)
			hashedData[a] = getValueHex(strSign[2 * a]) * 16 + getValueHex(strSign[2 * a + 1]);
	}
	//OpenSSL_add_all_algorithms();

	ECDSA_SIG* signature = ECDSA_do_sign(hashedData, sizeof(hashedData), ecKey);
	ECDSA_SIG_free(signature);
	cout << signature << '\n';
	//EVP_cleanup();
}
const char* hashiVeri;
string strVeri;

bool verifySignature(const string message, string signature, pair <uint256_t, uint256_t> publicKeyStr)
{
	//verify that the signature is correct
	if (message != "")
	{
		strVeri = hexToBase64(SHA256(message));
		hashiVeri = strSign.c_str();
	}
	return false;
}