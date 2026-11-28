#include "physics.h"
#include "util.h"
#include "physics.h"
#include "config.h"
#include "util.h"
#include "switches.h"
// Movement validation and resource adjustments without structures
bool isDirectlyConnected(int fromF, int fromR, int toF, int toR,
    const char horizontalConn[NUM_FLOORS][ROOMS_PER_FLOOR - 1],char& outConnType) {

    if(fromF < 1 || fromF > NUM_FLOORS || toF < 1 || toF > NUM_FLOORS)
     return false;// it checks that the floor number is in between 1 and 5
    else if(fromR < 1 || fromR > ROOMS_PER_FLOOR || toR < 1 || toR > ROOMS_PER_FLOOR)
     return false;//it checks that room number is in between 1 and 4 per floor.
    
    if(fromF == toF) {  // it checks that user wants to go on a same floor or not. 
        int leftroom;  
        if(fromR - toR != 1 && toR - fromR != 1){ // it basically checks that user wants to go 1 room away or not if not then it means it return false which basically means user give invalid input.
        return false;
        }
         else if(fromR < toR){ //it is checking that the user is in left room or right room 
            leftroom = fromR;
         }
         else
              leftroom = toR;
        char connection = horizontalConn[fromF - 1][leftroom - 1];//basically this line helps us to find the connection type between the rooms for example window ,door or none .
        if (connection == CONN_NONE || connection == CONN_SOLID_WALL){ 
            return false;
        }
        else{
        outConnType = connection;//it means there is some connection between the rooms.
        return true;
        }
    }
     else if(fromR != toR) // it means user wants to go another floor.
     return false; 
     int upperfloor;
     int downfloor;
     if(fromF < toF){//it means user wants to go up.
        upperfloor = toF;
        downfloor = fromF;
     }
     else{// it means user wants to go down.
        upperfloor = fromF;
        downfloor = toF;
     }

    if (fromR == 1 && ((downfloor == 1 && upperfloor == 2) || (downfloor == 2 && upperfloor == 3) || (downfloor == 4 && upperfloor == 5))) {
        outConnType = CONN_STAIRS;// it means user is in room 1 and wants to go up or down using stairs.
        return true;
    }
    else if((fromR == 2|| fromR == 3) &&downfloor== 1 && upperfloor == 2) {
        outConnType = CONN_LIFT1;// it specifies that if user want to room 3 and 2 of floor 1 and floor he can use lift1
        return true;
    }
    bool lift2Floors = (fromF == 2 || fromF == 4 || fromF == 5) && (toF == 2 || toF == 4 || toF == 5);//// Lift 2 serves floors 2, 4 and 5.
    if (fromR == 4 && lift2Floors) {// It is usable only when Room 4 on the floors involved is connected to the lift. 
        outConnType = CONN_LIFT2;
        return true;
    }
    else
    return false;
}
void applyResourceDelta(int& energy, int& time, int deltaEnergy, int deltaTime) {
    energy = clampInt(energy + deltaEnergy, 0, MAX_ENERGY);//it basically check that energy is in range or not between 0 to 100
    time = clampInt(time + deltaTime, 0, MAX_TIME);//it basically check that time is in range or not between 0 to 100
}

void applyScoreDelta(int& score, int deltaScore) { // it add or subtract the score
    score += deltaScore;
    if (score < 0) 
    score = 0;
}

bool hasItem(const char inventory[MAX_INVENTORY_CAPACITY], int inventoryCount, char itemSymbol) {
    for (int i = 0; i < inventoryCount; i++){//inventory count tell that how many slots are actually filled
        if (inventory[i] == itemSymbol)
         return true;//Returns true if itemSymbol is found in the player's inventory, false otherwise.
    }
          return false;
}

bool addItem(char inventory[MAX_INVENTORY_CAPACITY], int& inventoryCount, char itemSymbol) {// we use this function to puts a new item into the player's inventory, but only if there is still space
    if (inventoryCount >= MAX_INVENTORY_CAPACITY){
         return false;// payer dont have enough space to add new items in their inventory
    }
    
    inventory[inventoryCount] = itemSymbol;
    inventoryCount++;
    return true;
}

bool canTraverse(int targetFloor, int targetRoom, char connType,
                 const char inventory[MAX_INVENTORY_CAPACITY], int inventoryCount,
                 int emergencyTripsLeft, bool lift2Operational) {// it tells that player can pass through this connection or not

    if (connType == CONN_LOCKED_DOOR){
    return unlockFloor3Door(inventory, inventoryCount);
    }
    if (connType == CONN_LIFT2) {
        return (lift2Operational && hasItem(inventory, inventoryCount, ENT_LIFT_PASS)) || emergencyTripsLeft > 0;
    }
    return true;
}                  

 bool removeItem(char inventory[MAX_INVENTORY_CAPACITY], int& inventoryCount, char itemSymbol) {
    for (int i = 0; i < inventoryCount; i++) {//this function is used to remove tje items from the inventory.
        if (inventory[i] == itemSymbol) {// it matches which inventory item  we want to remove
            for (int k = i; k < inventoryCount - 1; k++) {
            inventory[k] = inventory[k + 1];//it replaces the index of an array after the removal of inventory item
            }
            inventoryCount--;
            return true;
        }
    }
    return false;
}