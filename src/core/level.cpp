#include "level.h"
#include "util.h"
#include "physics.h"
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
// Modular implementation using primitive arrays for map parsing and verification



static int readLabelRow(const char* line, int cols[ROOMS_PER_FLOOR]) {//basically finds the location of each room (where each room sits)
    int len = (int)strlen(line), found = 0, floor = 0;
    for (int p = 0; p + 4 < len && found < ROOMS_PER_FLOOR; p++) {      //starts raading eg f1-r1 from left to rigjt
        if (line[p] == 'F' && line[p + 2] == '-' && line[p + 3] == 'R' &&               //cheks if it matches exact format like eg F1-R1
            isdigit((unsigned char)line[p + 1]) && line[p + 4] - '0' == found + 1) {
            floor = line[p + 1] - '0';       //-0 will convert the char to int '1' - '0' = 1
            cols[found++] = p;  //puts colunm numbers of that floor in the elements of aray
        }
    }
    if (found != ROOMS_PER_FLOOR || floor < 1 || floor > NUM_FLOORS) //if condition not satisfied then
    return 0;
    return floor;    //returns floor and also validates the floor
}



static void readConnections(const char* line, const int cols[ROOMS_PER_FLOOR], char conn[ROOMS_PER_FLOOR - 1]) {// basically check the connections between each rooms 
                                                                                                                // 1st 2 things read same thing as above func
                                                                                                                // 3rd telling about number of connection on one row (3)
    for (int k = 0; k < ROOMS_PER_FLOOR - 1; k++) {//runs 3 times to check what is in the 3 gaps in between rooms
        char tok[8];                               // 7 characters + 1 null terminator
        int t = 0;

        for (int i = cols[k] + 5; i < cols[k + 1] && t < 7; i++)//each next thing comes after a 5 gap, stops right before next room && stores max 7  elements so no overflow
            if (line[i] != ' ') 
                tok[t++] = line[i]; //only stores max of 7 non-space characters in array 
        tok[t] = '\0';              //the last element as null character

        conn[k] = CONN_SOLID_WALL;  // anything unknown or space counts as a wall
        if (stringEquals(tok, "L1")) 
            conn[k] = CONN_LIFT1;   //if L1 (special char) is found, then store that element of array as L1

        else if (t == 1 && strchr("DWL#", tok[0])) 
            conn[k] = tok[0];       //if any other than L1 store it in array starting from 1st space between rooms (tok[0])
    }
}



