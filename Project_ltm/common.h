#ifndef COMMON_H
#define COMMON_H

#include <string>
#include <map>
#include <vector>

// Trạng thái của một Bộ Kit
struct KitState {
    std::string id;
    bool assigned = false; 
    
    // Thoi gian ao (0-23 gio)
    int current_hour = 8; 

    // Moi truong
    float humidity = 60.0;
    float temp = 28.5;
    float N = 45.0, P = 40.0, K = 35.0;
    
    // Thiet bi chap hanh
    bool pump_on = false;
    bool light_on = false;
    
    // Cau hinh Tu dong
    float H_MIN = 40.0, H_MAX = 80.0;
    float N_MIN = 30.0, P_MIN = 30.0, K_MIN = 30.0;

    // Cau hinh hen gio Den (0-23)
    int light_start = 18; // Mac dinh bat luc 18h
    int light_end = 22;   // Mac dinh tat luc 22h
    bool auto_light_mode = true; // Che do tu dong cho den
};

struct Garden { 
    std::string owner; 
    std::string name; 
    std::vector<std::string> kit_ids;
};

struct User { std::string username; std::string password; };

#endif