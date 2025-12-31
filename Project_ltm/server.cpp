/**
 * Compile: g++ server.cpp simulator.cpp -o server -pthread
 * Run: ./server
 */
#include "common.h"
#include "simulator.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <algorithm>
#include <map>

#define PORT 8888
#define BUFFER_SIZE 2048

class CentralServer {
private:
    std::vector<User> users;
    std::vector<Garden> gardens;
    std::map<std::string, KitState> kits; 
    
    std::mutex db_mutex;  // Khoa bao ve du lieu
    std::mutex log_mutex; // [MOI] Khoa bao ve man hinh log
    
    bool running = true;
    GardenSimulator* sim;

public:
    CentralServer() {
        load_database();
        if (kits.empty()) {
            for(int i=1; i<=5; i++) {
                std::string id = "KIT_0" + std::to_string(i);
                kits[id] = {id}; 
            }
            std::cout << "[INIT] Created 5 virtual Kits.\n";
            save_all();
        }
        sim = new GardenSimulator(kits, db_mutex);
    }

    // ---  HAM LOG CHO SERVER ---
    void server_log(std::string direction, std::string content, int sock_id) {
        std::lock_guard<std::mutex> lock(log_mutex);
        
        // Nhan (RECV),  Gui (SEND)
        if(direction == "RECV") std::cout << "\033[1;33m"; 
        else if(direction == "SEND") std::cout << "\033[1;36m";
        else std::cout << "\033[1;37m"; // Mau trang cho Info

        std::cout << "[SOCK " << sock_id << "] [" << direction << "] " << content << "\033[0m\n";
    }

    std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (std::string::npos == first) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }

    void load_database() {
        std::string line;
        // Load Users
        std::ifstream f_users("users.txt");
        if(f_users.is_open()) {
            while (std::getline(f_users, line)) {
                if (trim(line).empty()) continue;
                std::stringstream ss(line); std::string u, p;
                std::getline(ss, u, '|'); std::getline(ss, p, '|');
                users.push_back({trim(u), trim(p)});
            }
            f_users.close();
        }
        // Load Gardens
        std::ifstream f_gardens("gardens.txt");
        if(f_gardens.is_open()) {
            while (std::getline(f_gardens, line)) {
                if (trim(line).empty()) continue;
                std::stringstream ss(line); std::string o, n, k;
                std::getline(ss, o, '|'); std::getline(ss, n, '|'); std::getline(ss, k, '|');
                k = trim(k);
                gardens.push_back({trim(o), trim(n), k});
            }
            f_gardens.close();
        }
        // Load Kits
        std::ifstream f_kits("kits.txt");
        if(f_kits.is_open()) {
            while (std::getline(f_kits, line)) {
                if (trim(line).empty()) continue;
                std::stringstream ss(line); 
                std::string id, assigned_str, hmin, hmax, nmin, pmin, kmin, l_start, l_end, auto_l;
                std::getline(ss, id, '|'); std::getline(ss, assigned_str, '|');
                std::getline(ss, hmin, '|'); std::getline(ss, hmax, '|');
                std::getline(ss, nmin, '|'); std::getline(ss, pmin, '|'); std::getline(ss, kmin, '|');
                std::getline(ss, l_start, '|'); std::getline(ss, l_end, '|'); std::getline(ss, auto_l, '|');

                KitState k; k.id = trim(id); k.assigned = (trim(assigned_str) == "1");
                try {
                    k.H_MIN = std::stof(hmin); k.H_MAX = std::stof(hmax);
                    k.N_MIN = std::stof(nmin); k.P_MIN = std::stof(pmin); k.K_MIN = std::stof(kmin);
                    k.light_start = std::stoi(l_start); k.light_end = std::stoi(l_end);
                    k.auto_light_mode = (trim(auto_l) == "1");
                } catch(...) { }
                kits[k.id] = k;
            }
            f_kits.close();
        }
    }

    void save_all() {
        std::ofstream f_g("gardens.txt");
        for (const auto &g : gardens) f_g << g.owner << "|" << g.name << "|" << g.kit_id << "\n";
        f_g.close();
        std::ofstream f_u("users.txt");
        for (const auto &u : users) f_u << u.username << "|" << u.password << "\n";
        f_u.close();
        std::ofstream f_k("kits.txt");
        for (const auto &pair : kits) {
            const KitState &k = pair.second;
            f_k << k.id << "|" << (k.assigned ? "1" : "0") << "|"
                << k.H_MIN << "|" << k.H_MAX << "|" << k.N_MIN << "|" << k.P_MIN << "|" << k.K_MIN << "|"
                << k.light_start << "|" << k.light_end << "|" << (k.auto_light_mode ? "1" : "0") << "\n";
        }
        f_k.close();
    }

    void automation_control_loop() {
        std::cout << "[SERVER] Automation Logic Started.\n";
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1500));
            std::lock_guard<std::mutex> lock(db_mutex);

            for (auto &pair : kits) {
                KitState &k = pair.second;
                if (k.assigned) {
                    if (k.humidity < k.H_MIN && !k.pump_on) {
                        k.pump_on = true; std::cout << "[AUTO " << k.id << "] Do am thap. BAT BOM.\n";
                    } else if (k.humidity > k.H_MAX && k.pump_on) {
                        k.pump_on = false; std::cout << "[AUTO " << k.id << "] Do am du. TAT BOM.\n";
                    }
                    if (k.N < k.N_MIN || k.P < k.P_MIN || k.K < k.K_MIN) {
                        k.N += 15; k.P += 15; k.K += 15; std::cout << "[AUTO " << k.id << "] Bon phan.\n";
                    }
                    if (k.auto_light_mode) {
                        bool should = false;
                        if (k.light_start < k.light_end) { if(k.current_hour >= k.light_start && k.current_hour < k.light_end) should=true; }
                        else { if(k.current_hour >= k.light_start || k.current_hour < k.light_end) should=true; }
                        if(should && !k.light_on) { k.light_on=true; std::cout << "[TIMER " << k.id << "] Bat Den.\n"; }
                        else if(!should && k.light_on) { k.light_on=false; std::cout << "[TIMER " << k.id << "] Tat Den.\n"; }
                    }
                }
            }
        }
    }

    std::string handle_request(std::string msg) {
        msg = trim(msg);
        std::map<std::string, std::string> data;
        std::stringstream ss(msg); std::string segment;
        while(std::getline(ss, segment, ',')) {
            size_t pos = segment.find('=');
            if(pos != std::string::npos) data[trim(segment.substr(0, pos))] = trim(segment.substr(pos+1));
        }
        std::string cmd = data.count("CMD") ? data["CMD"] : (data.count("MSG_TYPE") ? data["MSG_TYPE"] : "");
        std::lock_guard<std::mutex> lock(db_mutex);

        // XU LY CAC LENH   
        if (cmd == "LOGIN") {
            for (auto &u : users) if (u.username == data["USER"] && u.password == data["PASS"]) return "STATUS=SUCCESS";
            return "STATUS=FAIL";
        }
        else if (cmd == "REGISTER") {
            for (auto &u : users) if (u.username == data["USER"]) return "STATUS=FAIL,MSG=UserExists";
            users.push_back({data["USER"], data["PASS"]}); save_all(); return "STATUS=SUCCESS";
        }
        else if (cmd == "CHANGE_PASS") {
            for (auto &u : users) {
                if (u.username == data["USER"] && u.password == data["OLD_PASS"]) { u.password = data["NEW_PASS"]; save_all(); return "STATUS=SUCCESS"; }
            }
            return "STATUS=FAIL";
        }
        else if (cmd == "CREATE_GARDEN") {
            for(auto &g : gardens) if(g.owner == data["USER"] && g.name == data["NAME"]) return "STATUS=FAIL,MSG=NameExists";
            gardens.push_back({data["USER"], data["NAME"]}); save_all(); return "STATUS=SUCCESS";
        }
        else if (cmd == "GET_LIST") {
            std::string list_str = "STATUS=SUCCESS,LIST="; bool first = true;
            for (auto &g : gardens) { if (g.owner == data["USER"]) { if (!first) list_str += ";"; list_str += g.name; first = false; } }
            if (first) list_str += "EMPTY"; return list_str;
        }
        else if (cmd == "GET_FREE_KITS") {
            std::string list_str = "STATUS=SUCCESS,LIST="; bool first = true;
            for (auto &pair : kits) { if (!pair.second.assigned) { if (!first) list_str += ";"; list_str += pair.first; first = false; } }
            if (first) list_str += "EMPTY"; return list_str;
        }
        else if (cmd == "ASSIGN_KIT") {
            std::string target_kit = data["KIT_ID"];
            if (kits[target_kit].assigned) return "STATUS=FAIL,MSG=KitBusy";
            for(auto &g : gardens) {
                if(g.owner == data["USER"] && g.name == data["GARDEN"]) { g.kit_id = target_kit; kits[target_kit].assigned = true; save_all(); return "STATUS=SUCCESS"; }
            }
            return "STATUS=FAIL";
        }
        else if (cmd == "REMOVE_KIT") {
            for(auto &g : gardens) {
                if(g.owner == data["USER"] && g.name == data["GARDEN"]) {
                    std::string k_id = g.kit_id; g.kit_id = "NONE";
                    if(kits.count(k_id)) kits[k_id].assigned = false; save_all(); return "STATUS=SUCCESS";
                }
            }
            return "STATUS=FAIL";
        }
        else if (cmd == "GET_GARDEN_DETAIL") {
            for(auto &g : gardens) if(g.owner == data["USER"] && g.name == data["NAME"]) return "STATUS=SUCCESS,KIT_ID=" + g.kit_id;
            return "STATUS=FAIL";
        }
        else if (cmd == "INFO_DATA_REQ") {
            std::string k_id = data["KIT_ID"];
            if (kits.count(k_id)) {
                KitState &k = kits[k_id];
                return "MSG_TYPE=INFO_DATA_RES,temp=" + std::to_string(k.temp) + ",humid=" + std::to_string(k.humidity) + 
                       ",N=" + std::to_string(k.N) + ",P=" + std::to_string(k.P) + ",K=" + std::to_string(k.K) +
                       ",N_SET=" + std::to_string(k.N_MIN) + ",P_SET=" + std::to_string(k.P_MIN) + ",K_SET=" + std::to_string(k.K_MIN) +
                       ",HMIN_SET=" + std::to_string(k.H_MIN) + ",HMAX_SET=" + std::to_string(k.H_MAX) +
                       ",CUR_HOUR=" + std::to_string(k.current_hour) + ",L_START=" + std::to_string(k.light_start) + ",L_END=" + std::to_string(k.light_end);
            }
            return "STATUS=FAIL";
        }
        else if (cmd == "CONTROL_REQ") {
            std::string k_id = data["KIT_ID"];
            if (kits.count(k_id)) {
                KitState &k = kits[k_id];
                std::string dev = data["device"]; std::string act = data["action"];
                if (dev == "pump") k.pump_on = (act == "ON");
                if (dev == "light") { k.light_on = (act == "ON"); k.auto_light_mode = false; }
                save_all(); return "MSG_TYPE=CONTROL_RES,STATUS=SUCCESS";
            }
            return "STATUS=FAIL";
        }
        else if (cmd == "SET_TIMER") {
            std::string k_id = data["KIT_ID"];
            if (kits.count(k_id)) {
                kits[k_id].light_start = std::stoi(data["START"]); kits[k_id].light_end = std::stoi(data["END"]);
                kits[k_id].auto_light_mode = true; save_all(); return "MSG_TYPE=RES,STATUS=SUCCESS";
            }
            return "STATUS=FAIL";
        }
        else if (cmd == "ENABLE_AUTO") {
            std::string k_id = data["KIT_ID"];
            if (kits.count(k_id)) { kits[k_id].auto_light_mode = true; save_all(); return "MSG_TYPE=RES,STATUS=SUCCESS"; }
            return "STATUS=FAIL";
        }
        else if (cmd == "SET_PARAM") {
            std::string k_id = data["KIT_ID"];
            if (kits.count(k_id)) {
                KitState &k = kits[k_id];
                if(data.count("HMIN")) k.H_MIN = std::stof(data["HMIN"]);
                if(data.count("HMAX")) k.H_MAX = std::stof(data["HMAX"]);
                if(data.count("NMIN")) k.N_MIN = std::stof(data["NMIN"]);
                if(data.count("PMIN")) k.P_MIN = std::stof(data["PMIN"]);
                if(data.count("KMIN")) k.K_MIN = std::stof(data["KMIN"]);
                save_all(); return "MSG_TYPE=SET_PARAM_RES,STATUS=SUCCESS";
            }
            return "STATUS=FAIL";
        }
        return "STATUS=ERROR";
    }

    void client_handler(int sock) {
        char buffer[BUFFER_SIZE];
        while(true) {
            memset(buffer, 0, BUFFER_SIZE);
            int n = recv(sock, buffer, BUFFER_SIZE, 0);
            if (n <= 0) break;
            
            // Chuan hoa chuoi nhan duoc (xoa ky tu xuong dong thua)
            std::string raw_msg(buffer);
            while(!raw_msg.empty() && (raw_msg.back() == '\r' || raw_msg.back() == '\n')) raw_msg.pop_back();

            // [LOG] Log ban tin nhan
            server_log("RECV", raw_msg, sock);

            // Xu ly
            std::string response = handle_request(raw_msg);
            
            // [LOG] Log ban tin gui
            server_log("SEND", response, sock);

            response += "\n";
            send(sock, response.c_str(), response.length(), 0);
        }
        close(sock);
        server_log("INFO", "Client Disconnected", sock);
    }

    void start() {
        int server_fd, new_socket; struct sockaddr_in address; int opt = 1; int addrlen = sizeof(address);
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        address.sin_family = AF_INET; address.sin_addr.s_addr = INADDR_ANY; address.sin_port = htons(PORT);
        bind(server_fd, (struct sockaddr *)&address, sizeof(address));
        listen(server_fd, 5);
        
        std::cout << ">>> CENTRAL SERVER RUNNING ON PORT " << PORT << " <<<\n";
        
        std::thread(&GardenSimulator::start, sim).detach();
        std::thread(&CentralServer::automation_control_loop, this).detach();

        while (running) {
            new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
            // [LOG] Log khi co ket noi moi
            server_log("INFO", "New Client Connected", new_socket);
            std::thread(&CentralServer::client_handler, this, new_socket).detach();
        }
    }
};

int main() { CentralServer s; s.start(); return 0; }