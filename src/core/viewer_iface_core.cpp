#include "viewer_iface.h"
#include "util.h"
#include <level.h>
#include <iostream>

// Terminal UI and dashboard rendering using primitive parameters

using namespace std;

static bool isClue(char s) {
    return s == ENT_CLUE_4 || s == ENT_CLUE_2 || s == ENT_CLUE_7;
}

void renderDashboard(int playerFloor, int playerRoom, int energy, int time, int score,
                     const char inventory[MAX_INVENTORY_CAPACITY], int inventoryCount,
                     int emergencyTripsLeft) {     //prints main header of game and current status of player
    printHeader("THE LAST TRAIN");
    cout << "Location: Floor " << playerFloor << ", Room " << playerRoom
         << "   Energy: " << energy << "/" << MAX_ENERGY
         << "   Time: " << time << "/" << MAX_TIME
         << "   Score: " << score << "\n";
    cout << "Inventory:";
    if (inventoryCount == 0) 
        cout << " (empty)";
    for (int i = 0; i < inventoryCount; i++)
        cout << "  [" << inventory[i] << "] " << getEntityName(inventory[i]);
    cout << "\n";
    if (emergencyTripsLeft > 0)
        cout << "Emergency Lift 2 trips left: " << emergencyTripsLeft << "\n";
}


// Clue digits stay hidden until the player inspects the room
void renderRoomDetails(int floor, int room,
                       const char roomEntities[NUM_FLOORS][ROOMS_PER_FLOOR][MAX_ENTITIES_PER_ROOM],
                       int entityCount) { //prints the items inside the room
    cout << "\nYou are in F" << floor << "-R" << room << ". You can see:";
    bool seeAnything = false;
    for (int i = 0; i < entityCount; i++) {
        char s = roomEntities[floor - 1][room - 1][i];
        if (isClue(s)) // clues are hidden until player inspects room
            continue;  //so we termiante this iteration
        cout << "  [" << s << "] " << getEntityName(s);
        seeAnything = true;
    }
    if (!seeAnything) 
        cout << " nothing special";
    cout << "\n";
}

//this function prints a small map and tells where player and train exit is as [P] [T]
void renderAsciiBuilding(int playerFloor, int playerRoom) {
    printHeader("BUILDING");
    for (int f = NUM_FLOORS; f >= 1; f--) {//checks th floors
        cout << "F" << f << "  ";
        for (int r = 1; r <= ROOMS_PER_FLOOR; r++) {//checks room
            if (f == playerFloor && r == playerRoom) cout << "[P]";
            else if (f == PLATFORM_FLOOR && r == PLATFORM_ROOM) cout << "[T]";
            else cout << "[ ]";
        }
        cout << "\n";
    }
    cout << "    R1 R2 R3 R4      P = you, T = train platform\n";
}

//this function prints objectives 
void renderObjectivesChecklist(const bool objectives[NUM_OBJECTIVES]) {
    printHeader("OBJECTIVES");
    for (int i = 0; i < OBJ_MISSION_WON; i++)
        cout << (objectives[i] ? "[x] " : "[ ] ") << getObjectiveName(i) << "\n";
}
