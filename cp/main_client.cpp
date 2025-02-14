#include <iostream>
#include <unistd.h>
#include <string>
#include <sstream>
#include <signal.h>
#include <zmq.hpp>
#include <thread>
#include <chrono>
#include <pthread.h>

using namespace std::chrono_literals;
const int DEFAULT_PORT  = 5050;

// commandMutex (для защиты при проверке приглашения)
pthread_mutex_t commandMutex;
zmq::context_t zmqContext(2);
zmq::socket_t playerSocket(zmqContext, ZMQ_REP);

int playerId;
std::string userCommand;

// Отправка сообщения по сокету
bool sendZmqMessage(zmq::socket_t &socket, const std::string &msg) {
    zmq::message_t message(msg.size());
    memcpy(message.data(), msg.c_str(), msg.size());
    return socket.send(message);
}

// Приём сообщения по сокету
std::string receiveZmqMessage(zmq::socket_t &socket) {
    zmq::message_t message;
    bool ok = false;
    try {
        ok = socket.recv(&message);
    }
    catch (...) {
        ok = false;
    }
    std::string received(std::string(static_cast<char*>(message.data()), message.size()));
    if (received.empty() || !ok) {
        return "Ошибка получения сообщения!";
    }
    return received;
}

// Формируем строку вида "tcp://127.0.0.1:<port>"
std::string getPortName(int port) {
    return "tcp://127.0.0.1:" + std::to_string(port);
}

// Параметры для потока (пока структура пустая)
typedef struct {
} CheckInviteParams;

// Функция, которая в отдельном потоке проверяет, пришло ли приглашение
void* checkInvite(void *param) {
    std::string inviteTempString;
    pthread_mutex_lock(&commandMutex);
    std::string inviteMsg = receiveZmqMessage(playerSocket);
    std::stringstream inviteStream(inviteMsg);
    std::getline(inviteStream, inviteTempString, ':');

    if (inviteTempString == "invite") {
        std::this_thread::sleep_for(100ms);
        std::getline(inviteStream, inviteTempString, ':');
        std::cout << "Игрок с ником " << inviteTempString << " приглашает вас в игру!" << std::endl;
        std::cout << "Вы согласны? (y/n)" << std::endl;
        std::cin >> userCommand;
        std::cout << "Ваш ответ: " << userCommand << "\n";

        if (!userCommand.empty() && userCommand[0] == 'y') {
            std::cout << "Вы приняли запрос!" << std::endl;
            sendZmqMessage(playerSocket, "accept");
            pthread_mutex_unlock(&commandMutex);
            pthread_exit(nullptr);
        } else {
            std::cout << "Вы отклонили запрос!" << std::endl;
            pthread_mutex_unlock(&commandMutex);
            sendZmqMessage(playerSocket, "reject");
        }
    }
    pthread_exit(nullptr);
}

int main(int argc, char** argv) {
    // Сокет для связи с сервером
    zmq::context_t contextLocal(2);
    zmq::socket_t serverSocket(contextLocal, ZMQ_REQ);

    // Подключаемся к основному серверу, слушающему порт 5555
    serverSocket.connect(getPortName(5555));

    // Инициализируем мьютекс
    pthread_mutex_init(&commandMutex, NULL);

    // Параметры для потока
    CheckInviteParams checkInviteParams;
    pthread_t inviteThread;

    int processId = getpid();
    std::string serverResponse;
    std::string tempString;
    int iteration = 1;

    while(true) {
        // login
        if (iteration == 1) {
            iteration++;

            std::string userLogin;
            std::cout << "Введите ваш логин: ";
            std::cin >> userLogin;

            // Формируем запрос
            std::string loginMessage = "login:" + std::to_string(processId) + ":" + userLogin;
            sendZmqMessage(serverSocket, loginMessage);
            serverResponse = receiveZmqMessage(serverSocket);

            std::stringstream ss(serverResponse);
            std::getline(ss, tempString, ':');

            // Обрабатываем ответ
            if (tempString == "Ok") {
                // Считываем, какой номер порта/идентификатор у нашего игрока
                std::getline(ss, tempString, ':');
                playerId = std::stoi(tempString);

                // Подключаемся к сокету, который сервер выделил нашему клиенту
                playerSocket.connect(getPortName(5555 + playerId));

                std::cout << "Вы успешно авторизовались!" << std::endl;
                std::cout << "Вы хотите пригласить друга? (y/n)" << std::endl;
                std::cin >> tempString;

                if (!tempString.empty() && tempString[0] == 'n') {
                    std::cout << "Ждем приглашения от друга..." << std::endl;
                    pthread_create(&inviteThread, NULL, checkInvite, &checkInviteParams);
                    std::this_thread::sleep_for(1000ms);
                    break;
                } else {
                    std::cout << "Чтобы пригласить друга, напишите: invite (friend_login)" << std::endl;
                }

            } else if (tempString == "Error") {
                // Смотрим конкретную ошибку
                std::getline(ss, tempString, ':');
                if (tempString == "NameAlreadyExist") {
                    std::cout << "ERROR: Это имя уже занято! Попробуйте другое." << std::endl;
                    iteration--;
                }
            }
        } else {
            // Ждём от пользователя команды
            std::cin >> userCommand;

            if (userCommand == "invite") {
                std::string friendLogin;
                std::cin >> friendLogin;
                std::cout << "Вы пригласили игрока с ником " << friendLogin << std::endl;
                std::cout << "Ждем ответ..." << std::endl;

                // Отправляем приглашение на сервер
                std::string inviteCmd = "invite:" + std::to_string(playerId) + ":" + friendLogin;
                sendZmqMessage(serverSocket, inviteCmd);

                serverResponse = receiveZmqMessage(serverSocket);
                std::stringstream ss(serverResponse);
                std::getline(ss, tempString, ':');

                if (tempString == "accept") {
                    std::cout << "Запрос принят!" << std::endl;
                    break;
                }
                else if (tempString == "reject") {
                    std::cout << "Запрос отклонен! С вами не хотят играть(" << std::endl;
                }
                else if (tempString == "Error") {
                    std::getline(ss, tempString, ':');
                    if (tempString == "SelfInvite") {
                        std::cout << "ERROR: Вы отправили запрос самому себе. Попробуйте снова." << std::endl;
                    }
                    else if (tempString == "LoginNotExist") {
                        std::cout << "ERROR: Игрока с таким ником не существует. Попробуйте снова." << std::endl;
                    }
                    else if (tempString == "AlreadyInviting") {
                        std::cout << "ERROR: Другой игрок уже хочет вас пригласить. Дадим ему это сделать." << std::endl;
                        pthread_create(&inviteThread, NULL, checkInvite, &checkInviteParams);
                        break;
                    }
                }
            } else {
                std::cout << "Вы ввели несуществующую команду. Попробуйте снова." << std::endl;
            }
        }
    }

    // Ожидаем "ping" от сервера, чтобы войти в игру
    pthread_mutex_lock(&commandMutex);
    serverResponse = receiveZmqMessage(playerSocket);
    std::string playerAnswer;
    if (serverResponse == "ping") {
        std::cout << "Вы готовы к игре? (y/n)" << std::endl;
        std::cin >> playerAnswer;
        if (!playerAnswer.empty() && playerAnswer[0] == 'y') {
            sendZmqMessage(playerSocket, "pong");
            std::cout << "Вы согласились. Дождитесь других игроков и мы начнем!" << std::endl;
        } else {
            sendZmqMessage(playerSocket, "no_pong");
            std::cout << "Вы отказались. До свидания!" << std::endl;
            return 0;
        }
    } else {
        std::cout << "Пришло неизвестное сообщение вместо 'ping'!" << std::endl;
    }

    if (playerId == 1) {
        std::cout << "Начинаем игру" << std::endl;
    } else {
        std::cout << "Начинаем игру. Подождите, пока другой пользователь расставит корабли" << std::endl;
    }

    std::cout << "Чтобы расставить ваши корабли (формат: x, y и ориентация (H или V) через пробелы). Подождите приглашения к размещению." << std::endl;

    while(true) {
        std::string incomingMessage = receiveZmqMessage(playerSocket);
        std::stringstream strs(incomingMessage);
        strs >> tempString;

        if (tempString == "Разместите") {
            std::cout << incomingMessage << std::endl;
            char orientation;
            int x, y;
            std::cin >> y >> x >> orientation;
            std::string sendMsg = "coords:" + std::to_string(x) + ":" + std::to_string(y) + ":" + orientation;
            sendZmqMessage(playerSocket, sendMsg);
        }
        else if (tempString == "board") {
            // Выводим доску после слова "board"
            std::cout << incomingMessage.substr(5, incomingMessage.size()) << std::endl;
            sendZmqMessage(playerSocket, "ok");
        }
        else if (tempString == "Error") {
            std::cout << incomingMessage << std::endl;
            sendZmqMessage(playerSocket, "ok");
        }
        else if (tempString == "your_turn") {
            sendZmqMessage(playerSocket, "ok");
            std::cout << "Ваш ход:" << std::endl;
            incomingMessage = receiveZmqMessage(playerSocket);
            if (incomingMessage == "shoot") {
                int x, y;
                std::cout << "Введите координаты выстрела (формат: x y):" << std::endl;
                std::cin >> y >> x;
                std::string shootMsg = "coords:" + std::to_string(x) + ":" + std::to_string(y);
                sendZmqMessage(playerSocket, shootMsg);

                incomingMessage = receiveZmqMessage(playerSocket);
                if (incomingMessage == "shooted") {
                    std::cout << "Попадание!" << std::endl;
                    sendZmqMessage(playerSocket, "ok");
                } else if (incomingMessage == "miss") {
                    std::cout << "Промах!" << std::endl;
                    sendZmqMessage(playerSocket, "ok");
                }
            }
        }
        else if (tempString == "not_your_turn") {
            std::cout << "Ход соперника: " << std::endl;
            sendZmqMessage(playerSocket, "ok");
            incomingMessage = receiveZmqMessage(playerSocket);
            if (incomingMessage == "shooted") {
                std::cout << "Вас подстрелили!" << std::endl;
                sendZmqMessage(playerSocket, "ok");
            } else if (incomingMessage == "miss") {
                std::cout << "Противник промахнулся" << std::endl;
                sendZmqMessage(playerSocket, "ok");
            }
        }
        else if (tempString == "win") {
            std::cout << "Вы выиграли!" << std::endl;
            sendZmqMessage(playerSocket, "ok");
            return 0;
        }
        else if (tempString == "lose") {
            std::cout << "Вы проиграли!" << std::endl;
            sendZmqMessage(playerSocket, "ok");
            return 0;
        }
    }
}