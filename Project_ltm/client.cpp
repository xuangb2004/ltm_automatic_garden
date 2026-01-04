/**
 * Compile: g++ client.cpp -o client
 */
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <map>
#include <iomanip>
#include <fstream>

#define SERVER_PORT 8888

class Network {
public:
    int sock = -1;
    std::string server_ip = "127.0.0.1"; 

    void set_server_ip(std::string ip) {
        server_ip = ip;
    }

    void log_packet(std::string direction, std::string content) {
        std::cout << "\n\033[1;33m[LOG] [" << direction << "] " << content << "\033[0m";
    }

    bool connect_server() {
        if (sock != -1) return true;
        sock = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in addr;
        addr.sin_family = AF_INET; 
        addr.sin_port = htons(SERVER_PORT);
        inet_pton(AF_INET, server_ip.c_str(), &addr.sin_addr);
        
        struct timeval timeout;      
        timeout.tv_sec = 3; timeout.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof timeout);

        return connect(sock, (struct sockaddr *)&addr, sizeof(addr)) >= 0;
    }

    std::string send_cmd(std::string msg) {
        if (sock == -1 && !connect_server()) return "";
        log_packet("CLIENT -> SERVER", msg);
        msg += "\n";
        send(sock, msg.c_str(), msg.length(), 0);
        std::string line = ""; char c;
        while (recv(sock, &c, 1, 0) > 0) { if (c == '\n') break; line += c; }
        if (!line.empty()) log_packet("SERVER -> CLIENT", line);
        std::cout << "\n";
        return line;
    }
    void disconnect() { if (sock != -1) close(sock); sock = -1; }
};

class App {
    Network net;
    std::string current_user = "";

    void show_device_data(std::string raw, std::string type) {
        std::map<std::string, std::string> data;
        std::stringstream ss(raw); std::string segment;
        while(std::getline(ss, segment, ',')) {
            size_t p = segment.find('=');
            if(p!=std::string::npos) data[segment.substr(0, p)] = segment.substr(p+1);
        }
        std::cout << "\n--- THONG SO (" << type << ") ---\n";
        if (type == "SENSOR") {
            std::cout << " - Gio he thong: " << data["CUR_HOUR"] << "h\n";
            std::cout << " - Nhiet do: " << data["temp"] << " C | Do am KK: " << data["humid"] << " %\n";
        }
        else if (type == "PUMP") std::cout << " - Do am Dat: " << data["humid"] << " %\n - Cai dat: Min=" << data["HMIN_SET"] << "%, Max=" << data["HMAX_SET"] << "%\n";
        else if (type == "FERT") {
            std::cout << " - N: " << data["N"] << " (Min: " << data["N_SET"] << ")\n";
            std::cout << " - P: " << data["P"] << " (Min: " << data["P_SET"] << ")\n";
            std::cout << " - K: " << data["K"] << " (Min: " << data["K_SET"] << ")\n";
        }
        else if (type == "LIGHT") {
            std::cout << " - Gio he thong: " << data["CUR_HOUR"] << "h\n";
            std::cout << " - Hen gio tu dong: Bat " << data["L_START"] << "h -> Tat " << data["L_END"] << "h\n";
        }
        std::cout << "-----------------------\n";
    }

    // --- LOGIC DIEU KHIEN 1 THIET BI (DA TACH RIENG) ---
    void process_specific_kit(std::string kit_id) {
        while(true) {
            std::cout << "\n--- DIEU KHIEN THIET BI: " << kit_id << " ---\n";
            std::cout << "1. Cam bien moi truong\n";
            std::cout << "2. He thong Bom nuoc\n";
            std::cout << "3. He thong Phan bon\n";
            std::cout << "4. He thong Den quang hop\n";
            std::cout << "0. Quay lai danh sach thiet bi\n";
            std::cout << "Chon chuc nang: ";
            
            int k; std::cin >> k;
            if (k==0) break;
            if (k>=1 && k<=4) control_sub_device(kit_id, k);
        }
    }

    void control_sub_device(std::string kit_id, int type_idx) {
        std::string types[] = {"", "SENSOR", "PUMP", "FERT", "LIGHT"};
        std::string cur_type = types[type_idx];
        std::string base_cmd = "KIT_ID=" + kit_id;

        while(true) {
            std::cout << "\n--- DIEU KHIEN: " << cur_type << " ---\n";
            std::cout << "1. Xem thong so\n";

            if (cur_type == "PUMP") {
                std::cout << "2. Bat Bom (Thu cong)\n";
                std::cout << "3. Tat Bom (Thu cong)\n";
                std::cout << "4. Cai dat Hmin/Hmax Tu dong\n";
            }
            else if (cur_type == "LIGHT") {
                std::cout << "2. Bat Den (Thu cong)\n";
                std::cout << "3. Tat Den (Thu cong)\n";
                std::cout << "4. Hen gio Tu Dong\n";
                std::cout << "5. Bat lai che do Auto\n"; 
            }
            else if (cur_type == "FERT") {
                std::cout << "2. Cai dat NPK (Tu dong)\n";
                std::cout << "3. Bon phan NGAY (Thu cong)\n"; 
            }

            std::cout << "0. Quay lai\nChon: ";
            int k; std::cin >> k; 
            if(k==0) break;

            if(k==1) show_device_data(net.send_cmd("CMD=INFO_DATA_REQ," + base_cmd), cur_type);
            
            else if(cur_type=="PUMP") {
                if(k==2) net.send_cmd("CMD=CONTROL_REQ," + base_cmd + ",device=pump,action=ON");
                else if(k==3) net.send_cmd("CMD=CONTROL_REQ," + base_cmd + ",device=pump,action=OFF");
                else if(k==4) {
                    std::string s_min, s_max;
                    std::cout << "Nhap do am toi thieu (H_MIN %): "; std::cin >> s_min;
                    std::cout << "Nhap do am toi da (H_MAX %): "; std::cin >> s_max;
                    try {
                         if(std::stof(s_min) < 0 || std::stof(s_max) < std::stof(s_min)) 
                            std::cout << ">> Loi: So khong hop le!\n";
                         else 
                            std::cout << net.send_cmd("CMD=SET_PARAM," + base_cmd + ",HMIN=" + s_min + ",HMAX=" + s_max) << "\n";
                    } catch(...) { std::cout << ">> Loi nhap lieu!\n"; }
                }
            }
            
            else if(cur_type=="LIGHT") {
                if(k==2) net.send_cmd("CMD=CONTROL_REQ," + base_cmd + ",device=light,action=ON");
                else if(k==3) net.send_cmd("CMD=CONTROL_REQ," + base_cmd + ",device=light,action=OFF");
                else if(k==4) {
                    std::string s, e; std::cout << "Gio bat (0-23): "; std::cin >> s; std::cout << "Gio tat (0-23): "; std::cin >> e;
                    std::cout << net.send_cmd("CMD=SET_TIMER," + base_cmd + ",START=" + s + ",END=" + e) << "\n";
                }
                else if(k==5) {
                      std::cout << net.send_cmd("CMD=ENABLE_AUTO," + base_cmd) << "\n";
                }
            }
            
            else if(cur_type=="FERT") {
                if(k==2) { 
                    std::string n,p,kk; 
                    std::cout<<"Nhap nguong N (min): ";std::cin>>n;
                    std::cout<<"Nhap nguong P (min): ";std::cin>>p;
                    std::cout<<"Nhap nguong K (min): ";std::cin>>kk;
                    net.send_cmd("CMD=SET_PARAM," + base_cmd + ",NMIN="+n+",PMIN="+p+",KMIN="+kk);
                }
                else if(k==3) {
                    std::cout << net.send_cmd("CMD=CONTROL_REQ," + base_cmd + ",device=fert,action=ON") << "\n";
                    std::cout << ">> Da gui lenh bon phan!\n";
                }
            }
        }
    }

public:
    void setup_connection() {
        std::string ip;
        std::cout << "=== KET NOI SERVER ===\n";
        std::cout << "Nhap IP cua may Server (VD: 192.168.1.X) [Enter de dung mac dinh 127.0.0.1]: ";
        if (std::cin.peek() == '\n') std::cin.ignore();
        else std::cin >> ip;
        
        if (!ip.empty()) net.set_server_ip(ip);
        
        std::cout << "Dang ket noi thu...";
        if (net.connect_server()) {
            std::cout << " THANH CONG!\n";
            net.disconnect();
        } else {
            std::cout << " THAT BAI! Vui long kiem tra IP hoac Tuong lua (Firewall).\n";
            exit(0);
        }
    }

    void auth_menu() {
        while (current_user == "") {
            std::cout << "\n=== SMART GARDEN ===\n1. Dang nhap\n2. Dang ky\n0. Thoat\nChon: ";
            int c; std::cin >> c; if(c==0) exit(0);
            std::string u, p; std::cout<<"User: "; std::cin>>u; std::cout<<"Pass: "; std::cin>>p;
            if (c==1) {
                if (net.send_cmd("CMD=LOGIN,USER="+u+",PASS="+p).find("SUCCESS") != std::string::npos) current_user = u;
                else std::cout << ">> Sai thong tin!\n";
            } else { std::cout << ">> " << net.send_cmd("CMD=REGISTER,USER="+u+",PASS="+p) << "\n"; }
        }
    }

    void garden_list_menu() {
        while (true) {
            std::cout << "\n=== USER: " << current_user << " ===\n1. DS Vuon\n2. Tao Vuon\n3. Doi mat khau\n0. Logout\nChon: ";
            int c; std::cin >> c; if (c==0) { current_user=""; net.disconnect(); return; }
            
            if (c==1) {
                std::string resp = net.send_cmd("CMD=GET_LIST,USER="+current_user);
                size_t pos = resp.find("LIST=");
                if (pos!=std::string::npos) {
                    std::string data = resp.substr(pos+5);
                    if(data=="EMPTY" || data.empty()) { std::cout << ">> Chua co vuon.\n"; continue; }
                    std::vector<std::string> g_names;
                    std::stringstream ss(data); std::string n; int idx=1;
                    while(std::getline(ss, n, ';')) { if(!n.empty()) { g_names.push_back(n); std::cout << idx++ << ". " << n << "\n"; } }
                    std::cout << "Chon vuon (0 huy): "; int k; std::cin >> k;
                    if(k>0 && k<=g_names.size()) process_garden(g_names[k-1]);
                }
            } 
            else if (c==2) { 
                std::string n; std::cout<<"Ten vuon: "; std::cin>>n; 
                std::cout << net.send_cmd("CMD=CREATE_GARDEN,USER="+current_user+",NAME="+n) << "\n"; 
            }
            else if (c==3) { 
                std::string old_pass, new_pass, confirm_pass;
                std::cout << "\n--- DOI MAT KHAU ---\n";
                std::cout << "Nhap mat khau cu: "; std::cin >> old_pass;
                std::cout << "Nhap mat khau moi: "; std::cin >> new_pass;
                std::cout << "Xac nhan mat khau moi: "; std::cin >> confirm_pass;

                if (new_pass != confirm_pass) {
                    std::cout << "\n\033[1;31m>> LOI: Mat khau xac nhan khong khop! Vui long thu lai.\033[0m\n";
                } 
                else if (new_pass.empty()) {
                    std::cout << "\n\033[1;31m>> LOI: Mat khau khong duoc de trong!\033[0m\n";
                }
                else {
                    std::string resp = net.send_cmd("CMD=CHANGE_PASS,USER="+current_user+",OLD_PASS="+old_pass+",NEW_PASS="+new_pass);
                    if(resp.find("SUCCESS") != std::string::npos) std::cout << "\n\033[1;32m>> THANH CONG: Da doi mat khau! Vui long ghi nho.\033[0m\n";
                    else std::cout << "\n\033[1;31m>> THAT BAI: Mat khau cu khong dung!\033[0m\n";
                }
            }
        }
    }

    // --- CAP NHAT: QUAN LY NHIEU KIT TRONG VUON ---
    void process_garden(std::string g_name) {
        while(true) {
            std::string resp = net.send_cmd("CMD=GET_GARDEN_DETAIL,USER="+current_user+",NAME="+g_name);
            std::vector<std::string> my_kits;
            
            size_t pos = resp.find("KIT_LIST=");
            if(pos != std::string::npos) {
                std::string list_str = resp.substr(pos + 9);
                while (!list_str.empty() && (list_str.back() == '\r' || list_str.back() == '\n')) list_str.pop_back();
                
                if(list_str != "NONE" && !list_str.empty()) {
                    std::stringstream ss(list_str); std::string k;
                    while(std::getline(ss, k, ';')) my_kits.push_back(k);
                }
            }

            std::cout << "\n--- QUAN LY VUON: " << g_name << " ---\n";
            std::cout << "Danh sach thiet bi:\n";
            if (my_kits.empty()) std::cout << "  (Chua co thiet bi nao)\n";
            else {
                for(int i=0; i<my_kits.size(); i++) {
                    std::cout << "  " << (i+1) << ". [" << my_kits[i] << "]\n";
                }
            }
            std::cout << "--------------------\n";
            std::cout << "A. Them thiet bi moi\n";
            if (!my_kits.empty()) std::cout << "R. Go bo thiet bi\n";
            std::cout << "0. Quay lai\n";
            std::cout << "Chon (So thu tu thiet bi hoac A/R/0): ";
            
            std::string choice; std::cin >> choice;
            
            if (choice == "0") break;
            else if (choice == "A" || choice == "a") {
                add_kit_menu(g_name);
            }
            else if ((choice == "R" || choice == "r") && !my_kits.empty()) {
                std::cout << "Nhap ten Kit muon xoa (VD: KIT_01): ";
                std::string del_id; std::cin >> del_id;
                std::cout << net.send_cmd("CMD=REMOVE_KIT,USER="+current_user+",GARDEN="+g_name+",KIT_ID="+del_id) << "\n";
            }
            else {
                try {
                    int idx = std::stoi(choice);
                    if (idx >= 1 && idx <= my_kits.size()) {
                        process_specific_kit(my_kits[idx-1]);
                    }
                } catch(...) {}
            }
        }
    }

    void add_kit_menu(std::string g_name) {
        std::string resp = net.send_cmd("CMD=GET_FREE_KITS");
        size_t pos = resp.find("LIST=");
        if(pos!=std::string::npos) {
            std::string data = resp.substr(pos+5);
            while (!data.empty() && (data.back() == '\r' || data.back() == '\n')) data.pop_back();

            if(data=="EMPTY") { std::cout << ">> Het thiet bi ranh trong kho!\n"; return; }
            
            std::vector<std::string> kits; std::stringstream ss(data); std::string k; int idx=1;
            std::cout << "\n--- KHO THIET BI RANH ---\n";
            while(std::getline(ss, k, ';')) { if(!k.empty()) { kits.push_back(k); std::cout << idx++ << ". " << k << "\n"; } }
            
            std::cout << "Chon thiet bi muon them (0 huy): "; int c; std::cin >> c;
            if(c>0 && c<=kits.size()) 
                std::cout << ">> " << net.send_cmd("CMD=ASSIGN_KIT,USER="+current_user+",GARDEN="+g_name+",KIT_ID="+kits[c-1]) << "\n";
        }
    }

    void run() { 
        setup_connection(); 
        while(true) { auth_menu(); garden_list_menu(); } 
    }
};

int main() { App app; app.run(); return 0; }