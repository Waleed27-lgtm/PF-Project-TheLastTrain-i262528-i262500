#include "simulator.h"
#include "physics.h"
#include "switches.h"
#include "logger.h"
#include "viewer_iface.h"
#include "util.h"
#include "level.h"
#include <cctype>
#include <cstdio>
#include <iostream>
#include <string>
using namespace std;

// Modular simulation loop using primitive variables

static const int PASSENGER_FLOOR = 4;
static const int PASSENGER_ROOM = 1;
static const int CAT_FLOOR = 3; 
static const int CAT_ROOM = 3;
static const int PANEL_FLOOR = 3; 
static const int PANEL_ROOM = 4;
static const int CABINET_FLOOR = 5; 
static const int CABINET_ROOM = 1;

static const char* SAVE_FILE = "out/savegame.txt";
static const char* SUMMARY_FILE = "out/game_summary.txt";

static const char* connName(char type) {
    if (type == CONN_DOOR) {
    return "door";
} 
else if (type == CONN_WINDOW) {
    return "window";
} 
else if (type == CONN_LOCKED_DOOR) {
    return "locked door";
} 
else if (type == CONN_STAIRS) {
    return "stairs";
} 
else if (type == CONN_LIFT1) {
    return "Lift 1";
} 
else if (type == CONN_LIFT2) {
    return "Lift 2";
} 
else {
    return "?";
}
}


// Prints every room you can walk to from here
static void showExits(int floor, int room, const char conn[NUM_FLOORS][ROOMS_PER_FLOOR - 1]) {
    cout << "Exits:";
    for (int i = 1; i <= NUM_FLOORS; i++) {
        for (int j = 1; j <= ROOMS_PER_FLOOR; j++) {
            char connectionType;
            if (isDirectlyConnected(floor, room, i, j, conn, connectionType))
                cout << "  F" << i << "-R" << j << " (" << connName(connectionType) << ")";
        }
    }
    cout << "\n";
}


//if user types help, all these codes will occur
static void printHelp() {
    printHeader("COMMANDS");
    cout << "  move <floor> <room>   walk to a connected room, e.g.  move 2 1\n"
              << "  inspect               search the room (finds clue digits)\n"
              << "  collect <symbol>      pick up an item, e.g.  collect <\n"
              << "  use <symbol>          use an item you carry\n"
              << "  unlock 427            open the cabinet (type the code from the clues)\n"
              << "  goals / map / help    show objectives, building, this list\n"
              << "  save / load / quit\n"
              << "Goal: collect the 3 clues, open the cabinet in F5-R1, take the Train Key\n"
              << "and use it on the platform F5-R2. Rescuing the passenger and the cat gives bonus points.\n";
}


bool handleMoveCommand(int targetFloor, int targetRoom,
                       int& currentFloor, int& currentRoom,
                       int& energy, int& time,
                       const char horizontalConn[NUM_FLOORS][ROOMS_PER_FLOOR - 1],
                       const char inventory[MAX_INVENTORY_CAPACITY], int inventoryCount,
                       int& emergencyTripsLeft, bool lift2Operational) {

    char connectionType = CONN_NONE;
    if (!isDirectlyConnected(currentFloor, currentRoom, targetFloor, targetRoom, horizontalConn, connectionType)) {
        cout << "You cannot go there directly. Look at the exits list.\n";
        return false;
    }
    if (connectionType == CONN_LIFT1 && !canUseLift1(currentFloor, targetFloor)) {
        cout << "Lift 1 does not go there.\n";
        return false;
    }
    if (connectionType == CONN_LIFT2 && !canUseLift2(currentFloor, targetFloor, lift2Operational, emergencyTripsLeft)) {
        cout << "Lift 2 has no power. Install the fuse in the control panel (F3-R4).\n";
        return false;
    }
    if (!canTraverse(targetFloor, targetRoom, connectionType, inventory, inventoryCount, emergencyTripsLeft, lift2Operational)) {
        if (connectionType == CONN_LOCKED_DOOR) 
            cout << "The door is locked. You need the Master Key.\n";
        else                          
            cout << "Lift 2 needs the Lift Pass.\n";
        return false;
    }

    // Lift 2 without power + pass uses up one emergency trip
    if (connectionType == CONN_LIFT2 && !(lift2Operational && hasItem(inventory, inventoryCount, ENT_LIFT_PASS)))
        emergencyTripsLeft--;

    int dEnergy = COST_MOVE_ORDINARY_ENERGY; 
    int dTime = COST_MOVE_ORDINARY_TIME;

    if (connectionType == CONN_WINDOW) {
        dEnergy = COST_MOVE_WINDOW_ENERGY;
        dTime = COST_MOVE_WINDOW_TIME;
    } 
    else if (connectionType == CONN_LIFT1 || connectionType == CONN_LIFT2) {
        dEnergy = COST_MOVE_LIFT_ENERGY;
        dTime = COST_MOVE_LIFT_TIME;
    }
    applyResourceDelta(energy, time, dEnergy, dTime);//subtrsacts the energy and time from the current energy and time

    currentFloor = targetFloor;
    currentRoom = targetRoom;
    cout << "You go through the " << connName(connectionType) << " to F" << targetFloor << "-R" << targetRoom << ".\n";
    return true;
}


bool handleInspectCommand(int currentFloor, int currentRoom,//this function is used to inspect the room and find the clue digits
                          int& energy, int& time,
                          const char roomEntities[NUM_FLOORS][ROOMS_PER_FLOOR][MAX_ENTITIES_PER_ROOM],
                          int entityCount,
                          bool objectives[NUM_OBJECTIVES], int& score) {
    applyResourceDelta(energy, time, COST_INSPECT_ENERGY, COST_INSPECT_TIME);
    cout << "You search the room carefully...\n";

    bool found = false;
    for (int i = 0; i < entityCount; i++) {
        char entitySymbol = roomEntities[currentFloor - 1][currentRoom - 1][i];
        int clue = -1;
        if (entitySymbol == ENT_CLUE_4) clue = OBJ_CLUE_4;
        else if (entitySymbol == ENT_CLUE_2) clue = OBJ_CLUE_2;
        else if (entitySymbol == ENT_CLUE_7) clue = OBJ_CLUE_7;
        if (clue < 0) 
            continue;

        cout << "  You found a clue: the digit " << entitySymbol << "!\n";
        found = true;
        //this will check if the clue was already found
        if (!objectives[clue]) {
            objectives[clue] = true;
            // adds bonus score for collecting a clue
            applyScoreDelta(score, SCORE_CLUE_FOUND);
        }
    }
    if (!found) cout << "  Nothing new here.\n";
        return true;
}


bool handleCollectCommand(char itemSymbol, int currentFloor, int currentRoom,//pickes up items
                          int& energy, int& time, int& score,               //then removes from the room
                          char roomEntities[NUM_FLOORS][ROOMS_PER_FLOOR][MAX_ENTITIES_PER_ROOM],
                          int& entityCount,
                          char inventory[MAX_INVENTORY_CAPACITY], int& inventoryCount,
                          bool objectives[NUM_OBJECTIVES]) {
    char* room = roomEntities[currentFloor - 1][currentRoom - 1];
    int index = -1;
    for (int i = 0; i < entityCount; i++)
        if (room[i] == itemSymbol) 
            index = i;// the item in the room is stored in this index

    //if there is no item in the room or you try to collect a clue, bcz codes are meant to be inspected
    if (index < 0 || itemSymbol == ENT_CLUE_4 || itemSymbol == ENT_CLUE_2 || itemSymbol == ENT_CLUE_7) {
        cout << "There is no such item you can pick up here.\n";
        return false;
    }
    if (itemSymbol == ENT_CAT || itemSymbol == ENT_PASSENGER || itemSymbol == ENT_CONTROL_PANEL) {
        cout << "You cannot carry that.\n";
        return false;
    }
    //you cant pick up the train key untill you use unlock 247
    if (itemSymbol == ENT_CABINET_TRAIN_KEY && !objectives[OBJ_CABINET_UNLOCKED]) {
        cout << "The cabinet is locked. Use  unlock <code>.\n";
        return false;
    }
    if (!addItem(inventory, inventoryCount, itemSymbol)) {
        cout << "Your inventory is full.\n";
        return false;
    }

    for (int i = index; i < entityCount - 1; i++) 
        room[i] = room[i + 1];            // remove from the room
        room[entityCount - 1] = ENT_NONE;//erase the slot bcz items were collected
        entityCount--;

        //this funcrion deducts the costs
    applyResourceDelta(energy, time, COST_ITEM_PICKUP_ENERGY, COST_ITEM_PICKUP_TIME);
    cout << "You picked up: " << getEntityName(itemSymbol) << ".\n";

    //checking off the goals 
    if (itemSymbol == ENT_MASTER_KEY) {
        objectives[OBJ_MASTER_KEY] = true;
        applyScoreDelta(score, SCORE_KEY_COLLECT);
    }
     else if (itemSymbol == ENT_FUSE) {
        objectives[OBJ_FUSE_COLLECTED] = true;
    }
     else if (itemSymbol == ENT_LIFT_PASS) {
        objectives[OBJ_LIFT_PASS] = true;
        applyScoreDelta(score, SCORE_LIFT_PASS);
    }
     else if (itemSymbol == ENT_CAT_CARRIER) {
        objectives[OBJ_CAT_CARRIER] = true;
    }
     else if (itemSymbol == ENT_CABINET_TRAIN_KEY) {
        objectives[OBJ_TRAIN_KEY] = true;
        applyScoreDelta(score, SCORE_TRAIN_KEY);
    }
    return true;
}


bool handleUseCommand(char itemSymbol, int currentFloor, int currentRoom,
                      int& energy, int& time, int& score,
                      char inventory[MAX_INVENTORY_CAPACITY], int& inventoryCount,
                      bool objectives[NUM_OBJECTIVES]) {
    if (!hasItem(inventory, inventoryCount, itemSymbol)) {
        cout << "You do not have that item.\n";
        return false;
    }

    bool used = false;       // did the item do something?
    bool removeIt = true;    // is it used up afterwards?
    switch (itemSymbol) {
        case ENT_ENERGY_PACK:
            applyResourceDelta(energy, time, BONUS_ENERGY_PACK, 0);
            cout << "You feel stronger: +" << BONUS_ENERGY_PACK << " energy.\n";
            used = true;
            break;
        case ENT_EMERGENCY_TOOLKIT:
            applyResourceDelta(energy, time, BONUS_EMERGENCY_TOOLKIT, 0);
            cout << "Quick repairs boost you: +" << BONUS_EMERGENCY_TOOLKIT << " energy.\n";
            used = true;
            break;
        case ENT_OBSERVATION_TOKEN:
            applyResourceDelta(energy, time, 0, BONUS_OBSERVATION_TOKEN);
            cout << "The token buys you more time: +" << BONUS_OBSERVATION_TOKEN << " time.\n";
            used = true;
            break;
        case ENT_MEDICAL_KIT:
            if (currentFloor == PASSENGER_FLOOR && currentRoom == PASSENGER_ROOM && !objectives[OBJ_PASSENGER_RESCUED]) {
                objectives[OBJ_PASSENGER_RESCUED] = true;
                applyScoreDelta(score, SCORE_RESCUE_PASSENGER);
                cout << "You treat the passenger. Passenger rescued!\n";
                used = true;
            } 
            else {
                cout << "Nobody here needs the medical kit.\n";
            }
            break;
        case ENT_FUSE:
            removeIt = false;                           // installFuseControlPanel removes it itself
            if (currentFloor == PANEL_FLOOR && currentRoom == PANEL_ROOM) {
                used = installFuseControlPanel(inventory, inventoryCount, objectives, score);
                if (used) cout << "Fuse installed. Lift 2 now has power!\n";
            } 
            else {
                cout << "The fuse belongs in the control panel (F3-R4).\n";
            }
            break;
        case ENT_CAT_CARRIER:
            if (currentFloor == CAT_FLOOR && currentRoom == CAT_ROOM && !objectives[OBJ_CAT_SECURED]) {
                objectives[OBJ_CAT_SECURED] = true;
                applyScoreDelta(score, SCORE_SECURE_CAT);
                cout << "The cat hops into the carrier. Cat secured!\n";
                used = true;
            } 
            else {
                cout << "There is no cat to catch here.\n";
            }
            break;
        case ENT_CABINET_TRAIN_KEY:
            if (currentFloor == PLATFORM_FLOOR && currentRoom == PLATFORM_ROOM) {
                objectives[OBJ_TRAIN_PREPARED] = true;
                applyScoreDelta(score, SCORE_PREPARE_TRAIN);
                cout << "The train key turns. The train is ready to leave!\n";
                used = true;
            } 
            else {
                cout << "Use the Train Key on the platform (F5-R2).\n";
            }
            break;
        default:
            cout << "That item cannot be used.\n";
    }

    if (!used) //if item was not used due to any reason
        return false;
    if (removeIt)
        removeItem(inventory, inventoryCount, itemSymbol);  //removes item used from resource pack
    applyResourceDelta(energy, time, COST_ITEM_USE_ENERGY, COST_ITEM_USE_TIME);
    return true;
}


bool handleUnlockCabinetCommand(const char* code, int currentFloor, int currentRoom,  //checks many things about the clue digits and opening the cabinet
                                bool objectives[NUM_OBJECTIVES], int& score,
                                char roomEntities[NUM_FLOORS][ROOMS_PER_FLOOR][MAX_ENTITIES_PER_ROOM],
                                int& entityCount) {
    bool cabinetHere = false;
    if (currentFloor == CABINET_FLOOR && currentRoom == CABINET_ROOM)
        for (int i = 0; i < entityCount; i++)   //scans for cabinet key
            if (roomEntities[currentFloor - 1][currentRoom - 1][i] == ENT_CABINET_TRAIN_KEY) 
            {
                cabinetHere = true;
            }

    if (objectives[OBJ_CABINET_UNLOCKED]) {
        cout << "The cabinet is already open.\n";
        return false;
    }
    if (!cabinetHere) {
        cout << "There is no cabinet in this room.\n";
        return false;
    }
    if (!verifyCabinetCode(code, objectives)) {
        cout << "Wrong code, or you have not found all three clue digits yet.\n";
        return false;
    }
    objectives[OBJ_CABINET_UNLOCKED] = true;
    applyScoreDelta(score, SCORE_CABINET_UNLOCK);
    cout << "Click! The cabinet opens. You can take the Train Key.\n";
    return true;
}


