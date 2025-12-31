#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "common.h"
#include <map>
#include <mutex>

class GardenSimulator {
private:
    std::map<std::string, KitState>& kits; // Tham chieu den kho thiet bi cua Server
    std::mutex& db_mutex; // Tham chieu den khoa bao ve cua Server
    bool running = true;
    int global_hour = -1;
public:
    GardenSimulator(std::map<std::string, KitState>& k, std::mutex& m) 
        : kits(k), db_mutex(m) {}

    void start();
    void stop() { running = false; }
};

#endif