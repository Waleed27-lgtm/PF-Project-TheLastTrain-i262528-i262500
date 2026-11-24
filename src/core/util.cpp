#include "util.h"
#include <iostream>
#include <cstring>
#include <cctype>

using namespace std;


int clampInt(int val, int minVal, int maxVal) {//  // we are using this function to keep the value in between 0 and 100
    if (val < minVal) {
        return minVal;
    }
    if (val > maxVal) {
        return maxVal;
    }
    return val;
}

void trimWhitespace(char* str) {//we are using this function to remove the starting and ending spaces.
    if (str==nullptr) {
         return;
    }
    int start = 0;
    while (str[start] && isspace(str[start])) // basically isspace function check the spaces 
    start++; 
   
    int length= (int)strlen(&str[start]);
    memmove(str, str+start , length+1);//basically, this function is used to move the string after the removing of spaces.
    while(length > 0 && isspace(str[length - 1])){//this loop removes the remaing spaces after the string..
     --length;
     str[length] = '\0';
    }
}

bool stringEquals(const char* a, const char* b) {
   
    if(a == nullptr || b == nullptr){
         return 0;
    }

    int start = 0;
    while(a[start] != '\0' && b[start] != '\0') {
        if (tolower(a[start]) != tolower(b[start]))//this line makes the strinf case-insesitive.
            return 0;
        start++;
    }
    return a[start] == b[start];
} 
void printheader(const char* title)
{
    cout << "####"<<endl;
    if(title != nullptr)
       cout<<" "<< title << endl;

    cout << "####" << endl;
}

void clearscreen()
{
    cout << endl<<endl;
}
// Primitive helper utilities, clamping, and screen control
