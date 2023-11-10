#include <iostream>
#include "Header.h"
using namespace std;

int main() 
{
    string input = "Hello";
    string hashedResult = HashSHA256(input);

    cout << "Input: " << input << endl;
    cout << "Hashed Result: " << hashedResult << endl;

    return 0;
}