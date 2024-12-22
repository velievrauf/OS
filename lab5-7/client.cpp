#include "node.h"
#include "net_func.h"
#include "set"
#include <signal.h>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <map>

static std::atomic<bool> heartbeat_active(false);
static std::atomic<int> heartbeat_interval_ms(0);
static std::thread heartbeat_thread;
static bool heartbeat_thread_started = false;
static std::mutex nodes_mutex;
static std::map<int, std::chrono::time_point<std::chrono::steady_clock>> last_success_ping;

void heartbeat_thread_func(Node* me, std::set<int>* all_nodes) {
    // Поток отправляет пинги всем известным узлам каждые heartbeat_interval_ms миллисекунд
    // и проверяет, есть ли узлы, не ответившие уже > 4 раза дольше интервала.
    while (heartbeat_active.load()) {
        int interval = heartbeat_interval_ms.load();
        if (interval <= 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        
        {
            std::lock_guard<std::mutex> lock(nodes_mutex);
            for (auto node_id : *all_nodes) {
                if (node_id == -1) {
                    // Корневой узел - скип
                    continue;
                }
                // Пингуем узел
                std::string ans;
                if (me->children.find(node_id) != me->children.end()) {
                    ans = me->Ping_child(node_id);
                } else {
                    std::string str = "ping " + std::to_string(node_id);
                    ans = me->Send(str, node_id);
                    if (ans == "Error: not find") {
                        ans = "Ok: 0";
                    }
                }
                
                if (ans == "Ok: 1") {
                    // Узел доступен, обновляем время последнего успешного ответа
                    last_success_ping[node_id] = std::chrono::steady_clock::now();
                } else {
                    // Узел недоступен или не найден. Если его нет в last_success_ping, занесем текущее время,
                    // чтобы отсчитать таймер недоступности.
                    if (last_success_ping.find(node_id) == last_success_ping.end()) {
                        last_success_ping[node_id] = std::chrono::steady_clock::now();
                    } else {
                        // Проверим, сколько времени прошло с последнего успешного пинга
                        auto now = std::chrono::steady_clock::now();
                        auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_success_ping[node_id]).count();
                        // Если прошло более чем 4 * interval, сообщаем о недоступности
                        if (diff > 4 * interval) {
                            std::cout << "Heartbit: node " << node_id << " is unavailable now" << std::endl;
                            // Чтобы сообщение не повторялось бесконечно, обновим время так, чтобы снова ждать
                            last_success_ping[node_id] = std::chrono::steady_clock::now();
                        }
                    }
                }
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(heartbeat_interval_ms.load()));
    }
}

int main() {
    std::set<int> all_nodes;
    std::string prog_path = "./worker";
    Node me(-1);
    all_nodes.insert(-1);
    last_success_ping[-1] = std::chrono::steady_clock::now();
    std::string command;
    while (std::cin >> command) {
        if (command == "create") {
            int id_child, id_parent;
            std::cin >> id_child >> id_parent;
            if (all_nodes.find(id_child) != all_nodes.end()) {
                std::cout << "Error: Already exists" << std::endl;
            } else if (all_nodes.find(id_parent) == all_nodes.end()) {
                std::cout << "Error: Parent not found" << std::endl;
            } else if (id_parent == me.id) {
                std::string ans = me.Create_child(id_child, prog_path);
                std::cout << ans << std::endl;
                all_nodes.insert(id_child);
            } else {
                std::string str = "create " + std::to_string(id_child);
                std::string ans = me.Send(str, id_parent);
                std::cout << ans << std::endl;
                all_nodes.insert(id_child);
            }
        } else if (command == "ping") {
            int id_child;
            std::cin >> id_child;
            if (all_nodes.find(id_child) == all_nodes.end()) {
                std::cout << "Error: Not found" << std::endl;
            } else if (me.children.find(id_child) != me.children.end()) {
                std::string ans = me.Ping_child(id_child);
                std::cout << ans << std::endl;
            } else {
                std::string str = "ping " + std::to_string(id_child);
                std::string ans = me.Send(str, id_child);
                if (ans == "Error: not find") {
                    ans = "Ok: 0";
                }
                std::cout << ans << std::endl;
            }
        } else if (command == "exec") {
            int id;
            std::string cmd;
            std::cin >> id >> cmd;
            std::string msg = "exec " + cmd;
            if (all_nodes.find(id) == all_nodes.end()) {
                std::cout << "Error: Not found" << std::endl;
            } else {
                std::string ans = me.Send(msg, id);
                std::cout << ans << std::endl;
            }
        } else if (command == "remove") {
            int id;
            std::cin >> id;
            std::string msg = "remove";
            if (all_nodes.find(id) == all_nodes.end()) {
                std::cout << "Error: Not found" << std::endl;
            } else {
                std::string ans = me.Send(msg, id);
                if (ans != "Error: not find") {
                    std::istringstream ids(ans);
                    int tmp;
                    while (ids >> tmp) {
                        all_nodes.erase(tmp);
                    }
                    ans = "Ok";
                    if (me.children.find(id) != me.children.end()) {
                        my_net::unbind(me.children[id], me.children_port[id]);
                        me.children[id]->close();
                        me.children.erase(id);
                        me.children_port.erase(id);
                    }
                }
                std::cout << ans << std::endl;
            }
            }else if (command == "heartbit") {
                int time_ms;
                std::cin >> time_ms;
                if (time_ms <= 0) {
                    std::cout << "Error: invalid time" << std::endl;
                    continue;
                }
                heartbeat_interval_ms.store(time_ms);
                if (!heartbeat_thread_started) {
                    heartbeat_active.store(true);
                    heartbeat_thread = std::thread(heartbeat_thread_func, &me, &all_nodes);
                    heartbeat_thread.detach();
                    heartbeat_thread_started = true;
                }
                std::cout << "Ok" << std::endl;
            } 
        }

        heartbeat_active.store(false);
        me.Remove();
        return 0;
        }
