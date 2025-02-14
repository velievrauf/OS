#include <iostream>
#include <unistd.h>
#include <string>
#include <vector>
#include <sstream>
#include <signal.h>
#include <zmq.hpp>
#include <chrono>
#include <thread>
#include <exception>
#include <map>

using namespace std::chrono_literals;

// Определим размер поля
const int BOARD_SIZE = 10;
zmq::context_t serverContext(3);
zmq::socket_t serverControlSocket(serverContext, ZMQ_REP);

bool sendZmqMessage(zmq::socket_t &socket, const std::string &message) {
   zmq::message_t msg(message.size());
   memcpy(msg.data(), message.c_str(), message.size());
   return socket.send(msg);
}

std::string receiveZmqMessage(zmq::socket_t &socket) {
   zmq::message_t message;
   bool ok = false;
   try {
       ok = socket.recv(&message);
   } catch (...) {
       ok = false;
   }
   std::string received(std::string(static_cast<char*>(message.data()), message.size()));
   if (received.empty() || !ok) {
       return "Ошибка получения сообщения!";
   }
   return received;
}

// Класс игрока
class SeaBattlePlayer {
    public:
        std::vector<std::vector<char>> board;
        int playerNumber;  // Чтобы понимать, какой это игрок

        SeaBattlePlayer() {
            board.resize(BOARD_SIZE, std::vector<char>(BOARD_SIZE, ' '));
            playerNumber = 0;
        }

        // Расставление кораблей
        void placeShips(zmq::socket_t &playerSocket) {
            // Логика расстановки
            int shipsCount = 5;
            for (int shipSize = 1; shipSize <= 4; ++shipSize) {
                shipsCount--;
                for (int j = 0; j < shipsCount; j++) {
                    std::string msg = "Разместите корабль " + std::to_string(shipSize)
                                      + " (1x" + std::to_string(shipSize) + "): ";
                    sendZmqMessage(playerSocket, msg);

                    std::string receivedMessage = receiveZmqMessage(playerSocket);
                    std::cout << "Получил запрос: " << receivedMessage << std::endl;

                    std::string token;
                    std::stringstream strs(receivedMessage);
                    std::getline(strs, token, ':');  // "coords"
                    if (token == "coords") {
                        int x, y;
                        char orientation;
                        std::getline(strs, token, ':');  // X
                        x = std::stoi(token);

                        std::getline(strs, token, ':');  // Y
                        y = std::stoi(token);

                        std::getline(strs, token, ':');  // orientation
                        orientation = token.empty() ? 'H' : token[0];

                        // Проверяем валидность
                        if (orientation != 'H' && orientation != 'V') {
                            sendZmqMessage(playerSocket, "Error : Неверно указана ориентация (H/V)");
                            (void)receiveZmqMessage(playerSocket);
                            j--;
                            continue;
                        }
                        if (isValidPlacement(x, y, shipSize, orientation)) {
                            placeShip(x, y, shipSize, orientation);
                        } else {
                            sendZmqMessage(playerSocket, "Error : Неверное местоположение! Попробуйте еще раз.");
                            (void)receiveZmqMessage(playerSocket);
                            j--;
                            continue;
                        }

                        // Отправляем текущее состояние доски
                        std::string boardState = "board" + getBoard();
                        sendZmqMessage(playerSocket, boardState);
                        (void)receiveZmqMessage(playerSocket);
                    }
                }
            }
        }

        // Проверяем, можем ли мы разместить корабль
        bool isValidPlacement(int x, int y, int size, char orientation) const {
            if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE) {
                return false;
            }
            if (orientation == 'V') {
                if (x + size - 1 >= BOARD_SIZE) {
                    return false;
                }
                for (int i = x; i < x + size; ++i) {
                    if (board[i][y] != ' ') {
                        return false;
                    }
                }
            }
            else if (orientation == 'H') {
                if (y + size - 1 >= BOARD_SIZE) {
                    return false;
                }
                for (int j = y; j < y + size; ++j) {
                    if (board[x][j] != ' ') {
                        return false;
                    }
                }
            }
            // Проверяем окружение, чтобы корабль не касался других
            for (int i = 0; i < size; i++) {
                if (orientation == 'H') {
                    if (!isEmptyAround(x, y + i)) {
                        return false;
                    }
                } else {
                    if (!isEmptyAround(x + i, y)) {
                        return false;
                    }
                }
            }
            return true;
        }

        // Проверяем клетки вокруг
        bool isEmptyAround(int row, int col) const {
            for (int i = row - 1; i <= row + 1; ++i) {
                for (int j = col - 1; j <= col + 1; ++j) {
                    if (i >= 0 && i < BOARD_SIZE && j >= 0 && j < BOARD_SIZE && board[i][j] != ' ') {
                        return false;
                    }
                }
            }
            return true;
        }

        // Ставим корабль на поле
        void placeShip(int x, int y, int size, char orientation) {
            if (orientation == 'V') {
                for (int i = x; i < x + size; ++i) {
                    board[i][y] = 'O';
                }
            } else {
                for (int j = y; j < y + size; ++j) {
                    board[x][j] = 'O';
                }
            }
        }

        // Получаем доску для отображения
        std::string getBoard() const {
            std::string result;
            result = "\n  0 1 2 3 4 5 6 7 8 9\n";
            for (int i = 0; i < BOARD_SIZE; ++i) {
                result += std::to_string(i) + " ";
                for (int j = 0; j < BOARD_SIZE; ++j) {
                    result += board[i][j];
                    result += ' ';
                }
                result += '\n';
            }
            result += '\n';
            return result;
        }

        // Версия доски без отображения кораблей (например, чтобы отправлять противнику)
        std::string getClearBoard() const {
            std::string result;
            result = "\n  0 1 2 3 4 5 6 7 8 9\n";
            for (int i = 0; i < BOARD_SIZE; ++i) {
                result += std::to_string(i) + " ";
                for (int j = 0; j < BOARD_SIZE; ++j) {
                    if (board[i][j] == 'O') {
                        result += "  ";
                    } else {
                        result += board[i][j];
                        result += ' ';
                    }
                }
                result += '\n';
            }
            result += '\n';
            return result;
        }
};

// Класс Game, реализующий логику морского боя
class SeaBattleGame {
    public:
        SeaBattlePlayer player1;
        SeaBattlePlayer player2;

        void play(zmq::socket_t &player1Socket, zmq::socket_t &player2Socket) {
            std::cout << "Игра \"Морской бой\" началась!" << std::endl;

            // Запоминаем, что player1 — первый, player2 — второй
            player1.playerNumber = 1;
            player2.playerNumber = 2;

            // Предлагаем игрокам расставить корабли
            player1.placeShips(player1Socket);
            player2.placeShips(player2Socket);

            int turn = 0;
            while (!gameOver()) {
                if (turn % 2 == 0) {
                    // Ход первого игрока
                    sendZmqMessage(player1Socket, "your_turn");
                    (void)receiveZmqMessage(player1Socket);

                    sendZmqMessage(player2Socket, "not_your_turn");
                    (void)receiveZmqMessage(player2Socket);

                    std::cout << "Ход игрока 1:" << std::endl;
                    bool shotResult = handlePlayerTurn(player1, player2, player1Socket, player2Socket);
                    if (shotResult && gameOver()) {
                        // Игрок 1 добил противника
                        std::cout << "Победил игрок 1" << std::endl;
                        sendZmqMessage(player1Socket, "win");
                        (void)receiveZmqMessage(player1Socket);
                        sendZmqMessage(player2Socket, "lose");
                        (void)receiveZmqMessage(player2Socket);
                        break;
                    }
                    if (!shotResult) {
                        turn++;
                    }
                } else {
                    // Ход второго игрока
                    sendZmqMessage(player2Socket, "your_turn");
                    (void)receiveZmqMessage(player2Socket);

                    sendZmqMessage(player1Socket, "not_your_turn");
                    (void)receiveZmqMessage(player1Socket);

                    std::cout << "Ход игрока 2:" << std::endl;
                    bool shotResult = handlePlayerTurn(player2, player1, player2Socket, player1Socket);
                    if (shotResult && gameOver()) {
                        // Игрок 2 добил противника
                        std::cout << "Победил игрок 2" << std::endl;
                        sendZmqMessage(player2Socket, "win");
                        (void)receiveZmqMessage(player2Socket);
                        sendZmqMessage(player1Socket, "lose");
                        (void)receiveZmqMessage(player1Socket);
                        break;
                    }
                    if (!shotResult) {
                        turn++;
                    }
                }
            }

            std::cout << "Игра завершена!" << std::endl;
        }

        // Проверяем, закончилась ли игра (у кого-то больше нет кораблей)
        bool gameOver() const {
            return allShipsSunk(player1) || allShipsSunk(player2);
        }

        bool allShipsSunk(const SeaBattlePlayer &player) const {
            for (auto &row : player.board) {
                for (char cell : row) {
                    if (cell == 'O') {
                        return false;
                    }
                }
            }
            return true;
        }

        // Ход игрока: возвращаем true, если было попадание
        bool handlePlayerTurn(SeaBattlePlayer &attacker, SeaBattlePlayer &defender,
                              zmq::socket_t &attackerSocket, zmq::socket_t &defenderSocket)
        {
            // Запрашиваем координаты выстрела
            sendZmqMessage(attackerSocket, "shoot");
            std::string incomingShot = receiveZmqMessage(attackerSocket);

            std::stringstream ss(incomingShot);
            std::string token;
            std::getline(ss, token, ':'); // "coords"

            // Извлекаем x, y
            int x, y;
            std::getline(ss, token, ':');
            x = std::stoi(token);

            std::getline(ss, token, ':');
            y = std::stoi(token);

            // Проверяем
            if (isValidShot(x, y, defender)) {
                if (defender.board[x][y] == 'O') {
                    // Попадание
                    sendZmqMessage(attackerSocket, "shooted");
                    (void)receiveZmqMessage(attackerSocket);

                    sendZmqMessage(defenderSocket, "shooted");
                    (void)receiveZmqMessage(defenderSocket);

                    std::cout << "Попадание!" << std::endl;
                    defender.board[x][y] = 'X';  // отмечаем подбитую палубу
                    return true;
                } else {
                    // Промах
                    sendZmqMessage(attackerSocket, "miss");
                    (void)receiveZmqMessage(attackerSocket);

                    sendZmqMessage(defenderSocket, "miss");
                    (void)receiveZmqMessage(defenderSocket);

                    std::cout << "Промах!" << std::endl;
                    defender.board[x][y] = '*';

                    // Отправим обоим актуальные доски
                    std::string defenderBoard = defender.getBoard();
                    std::string defenderClearBoard = defender.getClearBoard();

                    sendZmqMessage(attackerSocket, "board" + defenderClearBoard);
                    (void)receiveZmqMessage(attackerSocket);

                    sendZmqMessage(defenderSocket, "board" + defenderBoard);
                    (void)receiveZmqMessage(defenderSocket);

                    return false;
                }
            } else {
                std::cout << "Неверные координаты! Повторяем ход." << std::endl;
                // Рекурсия для повторного ввода
                return handlePlayerTurn(attacker, defender, attackerSocket, defenderSocket);
            }
        }

        bool isValidShot(int x, int y, const SeaBattlePlayer &defender) const {
            return x >= 0 && x < BOARD_SIZE &&
                   y >= 0 && y < BOARD_SIZE &&
                   (defender.board[x][y] == ' ' || defender.board[x][y] == 'O');
        }
};

int main() {
    // Создаём сокеты для двух игроков
    zmq::socket_t firstPlayerSocket(serverContext, ZMQ_REQ);
    zmq::socket_t secondPlayerSocket(serverContext, ZMQ_REQ);

    // Привязываемся
    serverControlSocket.bind("tcp://*:5555");
    firstPlayerSocket.bind("tcp://*:5556");
    secondPlayerSocket.bind("tcp://*:5557");

    std::cout << "Сервер запущен и ожидает подключения..." << std::endl;

    std::map<int, std::string> playerLoginMap;
    int currentPlayerId = 1;

    while (true) {
        std::string incomingMessage = receiveZmqMessage(serverControlSocket);
        std::cout << "На сервер поступил запрос: '" << incomingMessage << "'" << std::endl;

        std::stringstream ss(incomingMessage);
        std::string command;
        std::getline(ss, command, ':'); // Берём первое слово ("login"/"invite"/...)

        if (command == "login") {
            if (currentPlayerId > 2) {
                sendZmqMessage(serverControlSocket, "Error:TwoPlayersAlreadyExist");
            } else {
                // Пропускаем PID
                std::string pidString;
                std::getline(ss, pidString, ':');

                // Считываем логин
                std::string playerLogin;
                std::getline(ss, playerLogin, ':');

                // Проверим, занят ли логин
                // Допустим, хотим, чтобы login_map[1] и login_map[2] были разными логинами
                if (playerLoginMap[1] == playerLogin || playerLoginMap[2] == playerLogin) {
                    std::cout << "Игрок ввёл занятый логин: " << playerLogin << std::endl;
                    sendZmqMessage(serverControlSocket, "Error:NameAlreadyExist");
                } else {
                    playerLoginMap[currentPlayerId] = playerLogin;
                    std::cout << "Логин игрока номер " << currentPlayerId
                              << ": " << playerLogin << std::endl;
                    sendZmqMessage(serverControlSocket, "Ok:" + std::to_string(currentPlayerId));
                    currentPlayerId++;
                }
            }
        }
        else if (command == "invite") {
            std::cout << "Обрабатываю приглашение" << std::endl;
            std::this_thread::sleep_for(100ms);

            std::string senderIdString;
            std::getline(ss, senderIdString, ':');
            int senderId = std::stoi(senderIdString);

            std::string inviteLogin;
            std::getline(ss, inviteLogin, ':');

            // Проверяем на самоприглашение
            if (inviteLogin == playerLoginMap[senderId]) {
                std::cout << "Игрок пригласил сам себя" << std::endl;
                sendZmqMessage(serverControlSocket, "Error:SelfInvite");
            }
            // Если отправил игрок 1 -> смотрим, есть ли такой логин
            else if (inviteLogin == playerLoginMap[2]) {
                // Приглашаем через secondPlayerSocket
                std::cout << "Игрок " << playerLoginMap[1]
                          << " пригласил " << playerLoginMap[2] << std::endl;
                sendZmqMessage(secondPlayerSocket, "invite:" + playerLoginMap[1]);

                std::string inviteAnswer = receiveZmqMessage(secondPlayerSocket);
                secondPlayerSocket.set(zmq::sockopt::rcvtimeo, -1);

                if (inviteAnswer == "accept") {
                    std::cout << "Игрок " << playerLoginMap[2] << " принял запрос" << std::endl;
                    sendZmqMessage(serverControlSocket, inviteAnswer);
                    break;
                }
                else if (inviteAnswer == "reject") {
                    std::cout << "Игрок " << playerLoginMap[2] << " отклонил запрос" << std::endl;
                    sendZmqMessage(serverControlSocket, inviteAnswer);
                }
                else {
                    std::cout << "Ошибка при обработке приглашения" << std::endl;
                }
            }
            else if (inviteLogin == playerLoginMap[1]) {
                // Приглашаем через firstPlayerSocket
                std::cout << "Игрок " << playerLoginMap[2]
                          << " пригласил " << playerLoginMap[1] << std::endl;
                sendZmqMessage(firstPlayerSocket, "invite:" + playerLoginMap[2]);

                std::string inviteAnswer = receiveZmqMessage(firstPlayerSocket);
                if (inviteAnswer == "accept") {
                    std::cout << "Игрок " << playerLoginMap[1] << " принял запрос" << std::endl;
                    sendZmqMessage(serverControlSocket, inviteAnswer);
                    break;
                }
                else if (inviteAnswer == "reject") {
                    std::cout << "Игрок " << playerLoginMap[1] << " отклонил запрос" << std::endl;
                    sendZmqMessage(serverControlSocket, inviteAnswer);
                }
                else {
                    std::cout << "Ошибка при обработке приглашения." << std::endl;
                }
            } else {
                // Такого логина не существует
                std::cout << "Логин " << inviteLogin << " не найден в базе" << std::endl;
                sendZmqMessage(serverControlSocket, "Error:LoginNotExist");
            }
        }
    }

    std::cout << "Запрашиваем готовность у игроков..." << std::endl;

    // Отправляем обоим игрокам "ping"
    sendZmqMessage(firstPlayerSocket, "ping");
    sendZmqMessage(secondPlayerSocket, "ping");

    std::string player1Answer = receiveZmqMessage(firstPlayerSocket);
    std::string player2Answer = receiveZmqMessage(secondPlayerSocket);

    if (player1Answer == "pong") {
        std::cout << "Игрок " << playerLoginMap[1] << " готов!" << std::endl;
    } else {
        std::cout << "Игрок " << playerLoginMap[1] << " отказался от игры." << std::endl;
        return 0;
    }

    if (player2Answer == "pong") {
        std::cout << "Игрок " << playerLoginMap[2] << " готов!" << std::endl;
    } else {
        std::cout << "Игрок " << playerLoginMap[2] << " отказался от игры." << std::endl;
        return 0;
    }

    std::cout << "Начинаем игру!" << std::endl;
    SeaBattleGame game;
    game.play(firstPlayerSocket, secondPlayerSocket);

    return 0;
}