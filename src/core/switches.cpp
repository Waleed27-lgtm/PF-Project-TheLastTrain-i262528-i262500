#include "switches.h"
#include "util.h"
#include "switches.h"
#include "util.h"
#include "physics.h"


// Door lock mechanics, lifts, and cabinet verification without structures
bool unlockFloor3Door(const char inventory[MAX_INVENTORY_CAPACITY], int inventoryCount) {
    return hasItem(inventory, inventoryCount, ENT_MASTER_KEY);
}

bool installFuseControlPanel(char inventory[MAX_INVENTORY_CAPACITY], int& inventoryCount, bool objectives[NUM_OBJECTIVES], 
      int& score) {
    if (!hasItem(inventory, inventoryCount, ENT_FUSE))// it returns false if  player dont have a fuse in its inventory 
     return false;

    removeItem(inventory, inventoryCount, ENT_FUSE);//it will remove the fuse from the inventory because we know tht we have a fuse and we used it so now we dont need it.

    objectives[OBJ_FUSE_INSTALLED] = true;
    objectives[OBJ_LIFT2_ACTIVE] = true;//now one of the condition to use a lift is fulfilled
    applyScoreDelta(score, SCORE_INSTALL_FUSE);
    return true;
}
bool canUseLift1(int fromFloor, int toFloor) {

     if(fromFloor >= 1 && fromFloor <= 2 && toFloor >= 1 && toFloor <= 2)
     return true;
     else
     return false;
}

// Lift 2 serves floors 2, 4 and 5. It needs power (fuse installed) or an emergency trip.
bool canUseLift2(int fromFloor, int toFloor, bool lift2Operational, int emergencyTripsLeft) {
    bool startfloor = (fromFloor == 2 || fromFloor == 4 || fromFloor == 5);
    bool destinationfloor = (toFloor == 2 || toFloor == 4 || toFloor == 5);
     if(startfloor && destinationfloor && fromFloor != toFloor && (lift2Operational || emergencyTripsLeft > 0)){
        return true;
     }
     else return false;
}
bool verifyCabinetCode(const char* enteredCode, const bool objectives[NUM_OBJECTIVES]) {
    if(enteredCode==nullptr){
        return false;
    }
    if (!(objectives[OBJ_CLUE_4] || objectives[OBJ_CLUE_2] || objectives[OBJ_CLUE_7]))
     return false;
    if( stringEquals(enteredCode, "427"))
    return true;
    return false;
}