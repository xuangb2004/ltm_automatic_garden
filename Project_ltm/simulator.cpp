// FILE: simulator.cpp
#include "simulator.h"
#include <iostream>
#include <thread>
#include <chrono>

void GardenSimulator::start() {
    std::cout << "[SIM] Environment Physics Started.\n";
    
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        std::lock_guard<std::mutex> lock(db_mutex);

        global_hour++;
        if (global_hour >= 24) global_hour = 0;
        
        std::cout << "--- [SIM TIME] " << global_hour << "h ---\n";

        for (auto &pair : kits) {
            KitState &k = pair.second;
            
            k.current_hour = global_hour;

            if (k.assigned) {
                // . MOI TRUONG TU NHIEN (Giam dan theo thoi gian)
                k.humidity -= 1.5; if(k.humidity < 0) k.humidity = 0;
                k.N -= 0.5; k.P -= 0.5; k.K -= 0.5;
                
                if (k.pump_on) {
                    k.humidity += 5.0; // Bơm đang chạy thì đất ướt lên
                }

            }
        }
    }
}