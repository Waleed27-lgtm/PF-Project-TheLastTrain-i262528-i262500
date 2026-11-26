#include "level.h"
#include "config.h"
#include "util.h"
#include "physics.h"
#include <cctype>
#include <cstring>
#include <fstream>
#include <iostream>
#include <cstdlib>
using namespace std;
// Modular implementation using primitive arrays for map parsing and verification



static int readLabelRow(const char* line, int cols[ROOMS_PER_FLOOR]) {//basically finds the location of each room (where each room sits)
    int length = (int)strlen(line), found = 0, floor = 0;
    for (int i = 0; i + 4 < length && found < ROOMS_PER_FLOOR; i++) {      //starts raading eg f1-r1 from left to rigjt
        if (line[i] == 'F' && line[i + 2] == '-' && line[i + 3] == 'R' &&               //cheks if it matches exact format like eg F1-R1
            isdigit(line[i + 1]) && line[i + 4] - '0' == found + 1) {
            floor = line[i + 1] - '0';       //-0 will convert the char to int '1' - '0' = 1
            cols[found++] = i;  //puts colunm numbers of that floor in the elements of aray
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



//each room has 11 columns if we count from left to right
static void readItems(const char* line, const int cols[ROOMS_PER_FLOOR], char items[ROOMS_PER_FLOOR][MAX_ENTITIES_PER_ROOM],
                      int counts[ROOMS_PER_FLOOR]) {// basically checks what items are inside the room
                                                    // 1st will read line of items, saved columns, 
                                                    // stores items in 2d array, last one stores how many items are in each room
    int length = (int)strlen(line);
    for (int k = 0; k < ROOMS_PER_FLOOR; k++) { //runs 4 times for each room
        for (int i = cols[k] - 1; i < cols[k] + 10 && i < length; i++) {//condition will check all the columns inside the room
            if (line[i] == ' ' || line[i] == '|') 
                continue;
            int quantity;
            if (line[i] == ENT_ENERGY_PACK) {  // the map shows ')' once but there are two energy packs
                quantity = 2;
            }
            else{
                quantity = 1;
            }
            for (int c = 0; c < quantity && counts[k] < MAX_ENTITIES_PER_ROOM; c++) //
                items[k][counts[k]++] = line[i];
        }
    }
}
bool loadBuildingMap(const char* filename,
                     char horizontalConn[NUM_FLOORS][ROOMS_PER_FLOOR - 1],
                     char roomEntities[NUM_FLOORS][ROOMS_PER_FLOOR][MAX_ENTITIES_PER_ROOM],
                     int entityCounts[NUM_FLOORS][ROOMS_PER_FLOOR]) {
    for (int i = 0; i < NUM_FLOORS; i++) {// this loop basically controls the floors
        for (int j = 0; j < ROOMS_PER_FLOOR - 1; j++) //this loop checks every room of that floor
        horizontalConn[i][j] = CONN_NONE;// we basically initialize all index of arrays to avoid the garbage value 
        for (int k = 0; k < ROOMS_PER_FLOOR; k++) {// it checks the all rooms for the entity
            entityCounts[i][k] = 0;//we basically initialize entity count to 0 to avoid the garbage value
            for (int a = 0; a < MAX_ENTITIES_PER_ROOM; a++) 
            roomEntities[i][k][a] = ENT_NONE;//we basically initialize all index of arrays to none to avoid the garbage value 
        }
    }
    ifstream file(filename);// we declare that we want to read a file
    if (!file) // it returns falls if file is empty
     return false;

    char current_line[MAX_LINE_LEN];
    int room_columns[ROOMS_PER_FLOOR] = { 0 };//it initialized all index 0;
    int next_item_floor = 0; // floor whose item row comes next (0 = none)
    int completed_floors = 0;

    while (file.getline(current_line, MAX_LINE_LEN)) {// this funtion is reading the character of every line
        current_line[strcspn(current_line, "\r\n")] = '\0'; // It scans current_line from the start and returns how many characters come before the first \r or \n. If neither is found, it returns the length of the string.
        if (next_item_floor != 0) {
            readItems(current_line, room_columns, roomEntities[next_item_floor - 1], entityCounts[next_item_floor - 1]);//reads the item in a floor
            next_item_floor = 0;//bcz next line is not a floorafter every floor
            completed_floors++;
        } else {
            next_item_floor = readLabelRow(current_line, room_columns);//not an item row, so check whether this line starts a new floor
            if (next_item_floor != 0)  // if it is an floor
            readConnections(current_line, room_columns, horizontalConn[next_item_floor - 1]);// it is checking that there is a connection between the rooms and type of commection
        }
    }
    return completed_floors == NUM_FLOORS;
}
bool loadInitialState(const char* filename,
                      int& playerFloor, int& playerRoom,
                      int& energy, int& time, int& score,
                      char inventory[MAX_INVENTORY_CAPACITY], int& inventoryCount,
                      bool objectives[NUM_OBJECTIVES], int& emergencyTripsLeft) {//this  function is used to fetch the starting value of game from a file
    ifstream file(filename);
    if (!file) return false;// we declare that we want to read a file

    playerFloor = 0;           
    playerRoom = 0;
    energy = INITIAL_ENERGY;
    time = INITIAL_TIME;
    score = INITIAL_SCORE;
    inventoryCount = 0;
    emergencyTripsLeft = 0;
    for (int i = 0; i < MAX_INVENTORY_CAPACITY; i++)
     inventory[i] = ENT_NONE;
    for (int i = 0; i < NUM_OBJECTIVES; i++) 
    objectives[i] = false;

    char line[MAX_LINE_LEN];
    while (file.getline(line, MAX_LINE_LEN)) { //strchr scans each line and find the colon
        char* colon = strchr(line, ':');//it is finfing the first colon from the initial.txt file
        if (colon == nullptr) 
        continue;
        *colon = '\0';//it seprates the text before the colon
        char* value = colon + 1;//it  points the first charachter after the colon
        trimWhitespace(line); // remove extra spaces from the name, e.g. "INITIAL_ENERGY " becomes "INITIAL_ENERGY"
        trimWhitespace(value);// remove extra spaces from the value, e.g. " 100" becomes "100"

        if (stringEquals(line, "INITIAL_ENERGY"))//This line checks whether the name on the current line is INITIAL_ENERGY
        energy = clampInt(atoi(value), 0, MAX_ENERGY);//atoi change the string into integer
        else if (stringEquals(line, "INITIAL_TIME")) //This line checks whether the name on the current line is INITIAL_TIME 
        time = clampInt(atoi(value), 0, MAX_TIME);//atoi change the string into integer
        else if (stringEquals(line, "INITIAL_SCORE")) //same as the above line
         score = atoi(value);//same as the above line
        else if (stringEquals(line, "START_ROOM")) {
    // expected format e.g. "F3-R2"
            if (strlen(value) >= 5 &&
            value[0] == 'F' && isdigit(value[1]) &&
             value[2] == '-' && value[3] == 'R' && isdigit(value[4])) {
             playerFloor = value[1] - '0';  
             playerRoom  = value[4] - '0';   
             }
}
    }
    return true;
}

bool validateStartingRoom(int startFloor, int startRoom,
                          const char horizontalConn[NUM_FLOORS][ROOMS_PER_FLOOR - 1]) {//check that a player starting in a given room can actually walk to the Train Boarding Platform (F5-R2). If not, the map is unwinnable and the function returns false.
    if (startFloor < 1 || startFloor > NUM_FLOORS || startRoom < 1 || startRoom > ROOMS_PER_FLOOR)
        return false;

    const int platformFloor = 5;   // F5-R2 = Train Boarding Platform
    const int platformRoom  = 2;
    const int totalRooms = NUM_FLOORS * ROOMS_PER_FLOOR;

    int roomQueue[totalRooms], head = 0, tail = 0;// queue of rooms waiting to be explored; head = front, tail = back
    bool seen[totalRooms] = { false };// seen[i] is true once room i has been reached (all start false)
     int start = (startFloor - 1) * ROOMS_PER_FLOOR + (startRoom - 1);   // convert floor/room into one index 0-19
    seen[start] = true;                    // the starting room counts as reached
    roomQueue[tail++] = start;             // put the starting room in the queue, then move tail forward

    while (head < tail) {                  // keep going while the queue still has rooms to explore
        int current = roomQueue[head++];       // take the front room as the current room, then move head forward
        for (int t = 0; t < totalRooms; t++) {   // try every room t as a possible neighbor of cur
            char type;                     // receives the connection type (door, lift, etc.) from the function below
            if (!seen[t] &&                // only consider rooms we haven't reached yet
                isDirectlyConnected(current / ROOMS_PER_FLOOR + 1, current % ROOMS_PER_FLOOR + 1,   // cur's floor and room number
                                    t / ROOMS_PER_FLOOR + 1, t % ROOMS_PER_FLOOR + 1,       // t's floor and room number
                                    horizontalConn, type)) {   // is there a direct connection between cur and t?
                seen[t] = true;            // mark t as reached so it is never added twice
                roomQueue[tail++] = t;     // add t to the back of the queue so its neighbors get explored later
            }
        }
    }
    // After the search, seen[] is true for every reachable room.
    // Return whether the platform (index (5-1)*4 + (2-1) = 17) was reached.
    return seen[(platformFloor - 1) * ROOMS_PER_FLOOR + (platformRoom - 1)];
}
const char* getEntityName(char s) {
    switch (s) {
        case ENT_MASTER_KEY:
         return "Master Key";
        case ENT_ENERGY_PACK:        
        return "Energy Pack";
        case ENT_EMERGENCY_TOOLKIT:  
        return "Emergency Toolkit";
        case ENT_CLUE_4:             
        return "Clue Digit 4";
        case ENT_FUSE:               
        return "Fuse";
        case ENT_CAT:                
        return "Stranded Cat";
        case ENT_CONTROL_PANEL:      
        return "Control Panel";
        case ENT_LIFT_PASS:          
        return "Lift Pass";
        case ENT_PASSENGER:          
        return "Stranded Passenger";
        case ENT_MEDICAL_KIT:        
        return "Medical Kit";
        case ENT_EMERGENCY_CARD:     
        return "Emergency Access Card";
        case ENT_CLUE_2:             
        return "Clue Digit 2";
        case ENT_CLUE_7:             
        return "Clue Digit 7";
        case ENT_CABINET_TRAIN_KEY:  
        return "Cabinet / Train Key";
        case ENT_CAT_CARRIER:        
        return "Cat Carrier";
        case ENT_OBSERVATION_TOKEN:  
        return "Observation Token";
        default:                     
        return "Unknown";
    }
}

const char* getObjectiveName(int i) {
    static const char* names[NUM_OBJECTIVES] = {
        "Master Key collected", "Clue digit 4 found", "Clue digit 2 found", "Clue digit 7 found",
        "Cabinet unlocked (427)", "Train Key collected", "Fuse collected", "Fuse installed",
        "Lift Pass collected", "Lift 2 powered", "Passenger rescued", "Cat Carrier collected",
        "Cat secured", "Train prepared", "Mission won", "Mission failed" };
        if(i >= 0 && i< NUM_OBJECTIVES){
            return names[i];
        }
        else
        return " unknown";
    
}
