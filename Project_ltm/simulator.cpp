// FILE: simulator.cpp
#include "simulator.h"
#include <iostream>
#include <thread>
#include <chrono>

void GardenSimulator::start() {
    std::cout << "[SIM] Environment Physics Started.\n";
    
    while (running) {
        // Toc do: 1 giay = 1 gio
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        std::lock_guard<std::mutex> lock(db_mutex);

        // 1. TROI GIO 
        global_hour++;
        if (global_hour >= 24) global_hour = 0;
        
         std::cout << "--- [SIM TIME] " << global_hour << "h ---\n";

        for (auto &pair : kits) {
            KitState &k = pair.second;
            k.current_hour = global_hour;

            if (k.assigned) {
                // 2. LOGIC DO AM & DINH DUONG (Giam dan theo thoi gian)
                k.humidity -= 1.5; if(k.humidity < 0) k.humidity = 0;
                k.N -= 0.5; k.P -= 0.5; k.K -= 0.5;

                // 3. LOGIC NHIET DO (MOI CAP NHAT)
                // Mo phong: Sang am dan -> Trua nong dinh diem -> Chieu mat -> Dem lanh
                if (global_hour >= 6 && global_hour <= 13) {
                    k.temp += 1.5; // Sang den trua: Tang nhiet do
                } 
                else if (global_hour > 13 && global_hour <= 18) {
                    k.temp -= 1.0; // Chieu: Giam nhiet do nhanh
                } 
                else {
                    k.temp -= 0.5; // Dem: Giam tu tu
                }

                // Gioi han nhiet do (Khong cho nong qua 40 hoac lanh qua 15)
                if (k.temp > 38.0) k.temp = 38.0;
                if (k.temp < 20.0) k.temp = 20.0;

                // 4. PHAN HOI VAT LY TU THIET BI
                // Neu bom bat -> Do am tang
                if (k.pump_on) {
                    k.humidity += 5.0; 
                    k.temp -= 0.2; // Bom nuoc lam mat dat mot chut
                }
                
                // Neu den bat -> Nhiet do tang nhe (den toa nhiet)
                if (k.light_on) {
                    k.temp += 0.5;
                }
            }
        }
    }
}