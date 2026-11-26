#include "logger.h"
#include "util.h"
#include <filesystem>
#include <fstream>
#include "level.h"
using namespace std;
// Modular simulation loop using primitive variables
// Creates the "out" folder if it does not exist yet.
// ofstream can create files but not folders, so without this
// the save file and summary report could not be written to "out/".
static void makeOutFolder() {
    error_code ec;   // errors go here instead of throwing
    filesystem::create_directories("out", ec);
}
bool saveGameStateToFile(const char* filename,
                         int playerFloor, int playerRoom,
                         int energy, int time, int score,
                         const char inventory[MAX_INVENTORY_CAPACITY], int inventoryCount,
                         const bool objectives[NUM_OBJECTIVES], int emergencyTripsLeft,
                         const char roomEntities[NUM_FLOORS][ROOMS_PER_FLOOR][MAX_ENTITIES_PER_ROOM],
                         const int entityCounts[NUM_FLOORS][ROOMS_PER_FLOOR]) {//this function save the surrent game in a file
    makeOutFolder();// it helps us to save file into a new folder out
    ofstream out(filename);//using to write in a file
    if (!out) // if the file  cannnot open in out folder it returns false 
    return false;

    out << playerFloor << ' ' << playerRoom << ' ' << energy << ' ' << time << ' '
        << score << ' ' << emergencyTripsLeft <<endl;
    out << inventoryCount;
    for (int i = 0; i < inventoryCount; i++) 
    out << ' ' << inventory[i];
    out << endl;
    for (int i = 0; i < NUM_OBJECTIVES; i++) 
    out << (objectives[i] ? 1 : 0) << ' ';
    out << endl;
    for (int i = 0; i < NUM_FLOORS; i++) {
        for (int j = 0; j < ROOMS_PER_FLOOR; j++) {
            out << entityCounts[i][j];
            for (int k= 0; k < entityCounts[i][j]; k++) 
            out << ' ' << roomEntities[i][j][k];
            out << endl;
        }
    }
    return (bool)out;//out is an ofstream, not a bool. The (bool) is a cast (a type conversion) that turns the stream into true or false:
                     //true: every write so far succeeded
                     //false: something went wrong
}

bool loadGameStateFromFile(const char* filename,
                           int& playerFloor, int& playerRoom,
                           int& energy, int& time, int& score,
                           char inventory[MAX_INVENTORY_CAPACITY], int& inventoryCount,
                           bool objectives[NUM_OBJECTIVES], int& emergencyTripsLeft,
                           char roomEntities[NUM_FLOORS][ROOMS_PER_FLOOR][MAX_ENTITIES_PER_ROOM],
                           int entityCounts[NUM_FLOORS][ROOMS_PER_FLOOR]) {
    ifstream in(filename); // read from the file
    if (!in) return false;

    int f, r, en, ti, sc, trips, invCount;//en=energy,ti=time,sc=score
    char inv[MAX_INVENTORY_CAPACITY];
    bool obj[NUM_OBJECTIVES];
    char ents[NUM_FLOORS][ROOMS_PER_FLOOR][MAX_ENTITIES_PER_ROOM];
    int counts[NUM_FLOORS][ROOMS_PER_FLOOR];

    if (!(in >> f >> r >> en >> ti >> sc >> trips >> invCount))
     return false;
    else if (f < 1 || f > NUM_FLOORS || r < 1 || r > ROOMS_PER_FLOOR)
     return false;
    else if (invCount < 0 || invCount > MAX_INVENTORY_CAPACITY) 
    return false;
    for (int i = 0; i < invCount; i++)
        if (!(in >> inv[i])) 
        return false;
    for (int i = 0; i < NUM_OBJECTIVES; i++) {
        int flag;
        if (!(in >> flag)) return false;
        obj[i] = (flag != 0);
    }
    for (int a = 0; a < NUM_FLOORS; a++) {
        for (int b = 0; b < ROOMS_PER_FLOOR; b++) {
            if (!(in >> counts[a][b]) || counts[a][b] < 0 || counts[a][b] > MAX_ENTITIES_PER_ROOM) return false;
            for (int c = 0; c < counts[a][b]; c++)
                if (!(in >> ents[a][b][c])) return false;
        }
    }

    playerFloor = f; playerRoom = r;
    energy = clampInt(en, 0, MAX_ENERGY);
    time = clampInt(ti, 0, MAX_TIME);
    score = sc; emergencyTripsLeft = trips; inventoryCount = invCount;
    for (int i = 0; i < invCount; i++) 
    inventory[i] = inv[i];
    for (int i = 0; i < NUM_OBJECTIVES; i++)
     objectives[i] = obj[i];
     for (int a = 0; a < NUM_FLOORS; a++) {
        for (int b = 0; b < ROOMS_PER_FLOOR; b++) {
            entityCounts[a][b] = counts[a][b];
            for (int c = 0; c < counts[a][b]; c++) 
            roomEntities[a][b][c] = ents[a][b][c];
        }
    }
    return true;
}

void writeGameSummaryReport(const char* filename,
                            int score, int remainingEnergy, int remainingTime,
                            const bool objectives[NUM_OBJECTIVES], bool victory) {
    makeOutFolder();
    ofstream out(filename);
    if (!out) return;

    out << "THE LAST TRAIN - GAME SUMMARY\n";
    out << "Result: ";
if (victory) {
    out << "VICTORY";
} else {
    out << "MISSION FAILED";
}
out << endl;
    out << "Score: " << score << endl;
    out << "Energy left: " << remainingEnergy << endl;
    out << "Time left: " << remainingTime <<endl<<endl;
    out << "Objectives:"<<endl;
    for (int i = 0; i < OBJ_MISSION_WON; i++)
        out << (objectives[i] ? "  [x] " : "  [ ] ") << getObjectiveName(i) << "\n";
}