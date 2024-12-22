#include "node.h"
#include "net_func.h"
#include <fstream>
#include <vector>
#include <signal.h>
#include <chrono>

int my_id = 0;

static bool timer_running = false;
static std::chrono::time_point<std::chrono::steady_clock> start_time;
static std::chrono::duration<double> elapsed(0.0);

void Log(std::string str) {
    std::string f = std::to_string(my_id) + ".txt";
    std::ofstream fout(f, std::ios_base::app);
    fout << str;
    fout.close();
}

int main(int argc, char **argv) {
    if (argc != 3) {
        return -1;
    }

    Node me(atoi(argv[1]), atoi(argv[2]));
    my_id = me.id;
    std::string prog_path = "./worker";
    while (1) {
        std::string message;
        std::string command = " ";
        message = my_net::reseave(&(me.parent));
        std::istringstream request(message);
        request >> command;

        if (command == "create") {
            int id_child;
            request >> id_child;
            std::string ans = me.Create_child(id_child, prog_path);
            my_net::send_message(&me.parent, ans);
        } else if (command == "pid") {
            std::string ans = me.Pid();
            my_net::send_message(&me.parent, ans);
        } else if (command == "ping") {
            int id_child;
            request >> id_child;
            std::string ans = me.Ping_child(id_child);
            my_net::send_message(&me.parent, ans);
        } else if (command == "send") {
            int id;
            request >> id;
            std::string str;
            getline(request, str);
            str.erase(0, 1);
            std::string ans;
            ans = me.Send(str, id);
            my_net::send_message(&me.parent, ans);
        } else if (command == "exec") {
            std::string cmd;
            request >> cmd;
            std::string ans;
            if (cmd == "start") {
                if (!timer_running) {
                    timer_running = true;
                    elapsed = std::chrono::duration<double>(0);
                    start_time = std::chrono::steady_clock::now();
                    ans = "Ok:" + std::to_string(me.id) + ":timer started";
                } else {
                    ans = "Ok:" + std::to_string(me.id) + ":timer already running";
                }
            } else if (cmd == "time") {
                if (timer_running) {
                    auto now = std::chrono::steady_clock::now();
                    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
                    ans = "Ok:" + std::to_string(me.id) + ":elapsed " + std::to_string(diff) + " ms";
                } else {
                    ans = "Ok:" + std::to_string(me.id) + ":timer not running";
                }
            } else if (cmd == "stop") {
                if (timer_running) {
                    auto now = std::chrono::steady_clock::now();
                    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
                    timer_running = false;
                    ans = "Ok:" + std::to_string(me.id) + ":timer stopped at " + std::to_string(diff) + " ms";
                } else {
                    ans = "Ok:" + std::to_string(me.id) + ":timer not running";
                }
            } else {
                ans = "Error: invalid exec command";
            }
            my_net::send_message(&me.parent, ans);
        } else if (command == "remove") {
    std::cout << "[WORKER] Removing node " << my_id << "..." << std::endl;
    std::string ans = me.Remove(); 
    ans = std::to_string(me.id) + " " + ans;
    my_net::send_message(&me.parent, ans);
    my_net::disconnect(&me.parent, me.parent_port);
    me.parent.close();
    std::cout << "[WORKER] Node " << my_id << " removed successfully." << std::endl;
    break;
}
    }
    sleep(1);
    return 0;
}