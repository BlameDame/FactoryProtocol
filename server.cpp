#include <SFML/Graphics.hpp>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <algorithm>
#include <unistd.h>
#include <iostream>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <cmath>
#include <ctime>
#include <mutex>
#include <map>


using namespace std;

// Game state structures (same as your original)
struct Mechanical {
    string gear;   // "Clockwise", "Counterclockwise", "Stopped"
    string lever;  // "Up", "Middle", "Down"
    string valve;  // "Open", "Partial", "Closed"
    int dial;      // 0 to 10
};

struct Electrical {
    string switchA;  // "On", "Off"
    string button;   // "Idle", "Pressed"
};

struct Machine {
    double pressure;
    double temperature;
};

struct GameState {
    Mechanical mechanical;
    Electrical electrical;
    Machine machine;
    double targetPressure;
    double targetTemperature;
    int timeLeft;
    bool gameActive;
    bool gameWon;
    bool gameFailed;
    bool playAgainRequested;
    bool mechanicalWantsReplay;
    bool electricalWantsReplay;
};

class GameServer {
private:
    int mechanicalSocket;
    int electricalSocket;
    GameState gameState;
    mutex stateMutex;
    bool playersConnected;

public:
    GameServer() {
        mechanicalSocket = -1;
        electricalSocket = -1;
        playersConnected = false;
        
        // Initialize game state
        gameState.mechanical = {"Stopped", "Middle", "Closed", 5};
        gameState.electrical = {"Off", "Idle"};
        gameState.machine = {100.0, 200.0};
        gameState.targetPressure = 150.0;
        gameState.targetTemperature = 300.0;
        gameState.timeLeft = 600;
        gameState.gameActive = true;
        gameState.gameWon = false;
        gameState.gameFailed = false;
        gameState.playAgainRequested = false;
        gameState.mechanicalWantsReplay = false;
        gameState.electricalWantsReplay = false;
    }

    ~GameServer() {
        if (mechanicalSocket != -1) close(mechanicalSocket);
        if (electricalSocket != -1) close(electricalSocket);
    }

    bool startServer() {
        int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (listenSocket < 0) {
            perror("socket creation failed");
            return false;
        }

        // Allow socket reuse
        int opt = 1;
        if (setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            perror("setsockopt failed");
            close(listenSocket);
            return false;
        }

        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(8888);

        if (bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            perror("bind failed");
            close(listenSocket);
            return false;
        }

        if (listen(listenSocket, 5) < 0) {
            perror("listen failed");
            close(listenSocket);
            return false;
        }

        cout << "Server started on port 8888. Waiting for players...\n";

        // Accept first player (Mechanical)
        mechanicalSocket = accept(listenSocket, NULL, NULL);
        if (mechanicalSocket < 0) {
            perror("accept mechanical player failed");
            close(listenSocket);
            return false;
        }
        cout << "Mechanical player connected!\n";

        // Accept second player (Electrical)
        electricalSocket = accept(listenSocket, NULL, NULL);
        if (electricalSocket < 0) {
            perror("accept electrical player failed");
            close(mechanicalSocket);
            close(listenSocket);
            return false;
        }
        cout << "Electrical player connected!\n";

        close(listenSocket);
        playersConnected = true;

        // Send initial game state to both players
        sendGameStateToPlayers();
        return true;
    }

    void sendGameStateToPlayers() {
        lock_guard<mutex> lock(stateMutex);
        
        string gameStateMsg = "STATE|" + 
                            to_string(gameState.machine.pressure) + "|" +
                            to_string(gameState.machine.temperature) + "|" +
                            to_string(gameState.targetPressure) + "|" +
                            to_string(gameState.targetTemperature) + "|" +
                            to_string(gameState.timeLeft) + "|" +
                            (gameState.gameActive ? "1" : "0") + "|" +
                            (gameState.gameWon ? "1" : "0") + "|" +
                            (gameState.gameFailed ? "1" : "0") + "|" + 
                            (gameState.mechanicalWantsReplay ? "1" : "0") + "|" +
                            (gameState.electricalWantsReplay ? "1" : "0") + "\n";

        ssize_t sent1 = send(mechanicalSocket, gameStateMsg.c_str(), gameStateMsg.length(), MSG_NOSIGNAL);
        ssize_t sent2 = send(electricalSocket, gameStateMsg.c_str(), gameStateMsg.length(), MSG_NOSIGNAL);
        
        if (sent1 < 0 || sent2 < 0) {
            cout << "Warning: Failed to send to one or both clients\n";
        }
    }

    void handleMechanicalPlayer() {
        char buffer[1024];
       /*while (gameState.gameActive)*/ while(playersConnected) {
            ssize_t bytesReceived = recv(mechanicalSocket, buffer, sizeof(buffer) - 1, 0);
            if (bytesReceived > 0) {
                buffer[bytesReceived] = '\0';
                string message(buffer);
                
                // Parse mechanical input: "MECH|gear|lever|valve|dial"
                if (message.substr(0, 5) == "MECH|") {
                    vector<string> tokens;
                    stringstream ss(message);
                    string token;
                    
                    while (getline(ss, token, '|')) {
                        tokens.push_back(token);
                    }
                    
                    if (tokens.size() >= 5) {
                        lock_guard<mutex> lock(stateMutex);
                        gameState.mechanical.gear = tokens[1];
                        gameState.mechanical.lever = tokens[2];
                        gameState.mechanical.valve = tokens[3];
                        try {
                            gameState.mechanical.dial = stoi(tokens[4]);
                        } catch (const exception& e) {
                            cout << "Invalid dial value: " << tokens[4] << "\n";
                            continue;
                        }
                        
                        cout << "Mechanical update: Gear=" << tokens[1] 
                             << " Lever=" << tokens[2] << " Valve=" << tokens[3] 
                             << " Dial=" << tokens[4] << "\n";
                    }
                } else if (message.substr(0, 11) == "PLAY_AGAIN|") {
                vector<string> tokens;
                stringstream ss(message);
                string token;
                
                while (getline(ss, token, '|')) {
                    tokens.push_back(token);
                }
                
                if (tokens.size() >= 2) {
                    lock_guard<mutex> lock(stateMutex);
                    gameState.mechanicalWantsReplay = (tokens[1] == "YES");
                    cout << "Mechanical player wants replay: " << tokens[1] << "\n";
                    
                    // Check if both players want to replay
                    if (gameState.mechanicalWantsReplay && gameState.electricalWantsReplay) {
                        resetGame();
                    }
                }
            }
            } else if (bytesReceived == 0) {
                cout << "Mechanical player disconnected\n";
                gameState.gameActive = false;
                playersConnected = false;
                break;
            } else if (bytesReceived < 0) {
                perror("recv from mechanical player failed");
                gameState.gameActive = false;
                playersConnected = false;
                break;
            }
        }
    }

    void handleElectricalPlayer() {
        char buffer[1024];
        /*while (gameState.gameActive)*/ while(playersConnected) {
            ssize_t bytesReceived = recv(electricalSocket, buffer, sizeof(buffer) - 1, 0);
            if (bytesReceived > 0) {
                buffer[bytesReceived] = '\0';
                string message(buffer);
                
                // Parse electrical input: "ELEC|switchA|button"
                if (message.substr(0, 5) == "ELEC|") {
                    vector<string> tokens;
                    stringstream ss(message);
                    string token;
                    
                    while (getline(ss, token, '|')) {
                        tokens.push_back(token);
                    }
                    
                    if (tokens.size() >= 3) {
                        lock_guard<mutex> lock(stateMutex);
                        gameState.electrical.switchA = tokens[1];
                        gameState.electrical.button = tokens[2];
                        
                        cout << "Electrical update: Switch=" << tokens[1] 
                             << " Button=" << tokens[2] << "\n";
                    }
                } else if (message.substr(0, 11) == "PLAY_AGAIN|") {
                vector<string> tokens;
                stringstream ss(message);
                string token;
                
                while (getline(ss, token, '|')) {
                    tokens.push_back(token);
                }
                
                if (tokens.size() >= 2) {
                    lock_guard<mutex> lock(stateMutex);
                    gameState.electricalWantsReplay = (tokens[1] == "YES");
                    cout << "Electrical player wants replay: " << tokens[1] << "\n";
                    
                    // Check if both players want to replay
                    if (gameState.mechanicalWantsReplay && gameState.electricalWantsReplay) {
                        resetGame();
                    }
                }
            }
            } else if (bytesReceived == 0) {
                cout << "Electrical player disconnected\n";
                gameState.gameActive = false;
                break;
            } else if (bytesReceived < 0) {
                perror("recv from electrical player failed");
                gameState.gameActive = false;
                break;
            }
        }
    }

    void resetGame() {
    lock_guard<mutex> lock(stateMutex);
    
    // Reset machine state
    gameState.mechanical = {"Stopped", "Middle", "Closed", 5};
    gameState.electrical = {"Off", "Idle"};
    gameState.machine = {100.0, 200.0};
    
    // Reset game state
    gameState.timeLeft = 60;
    gameState.gameActive = true;
    gameState.gameWon = false;
    gameState.gameFailed = false;
    
    // Reset play again flags
    gameState.playAgainRequested = false;
    gameState.mechanicalWantsReplay = false;
    gameState.electricalWantsReplay = false;
    
    cout << "Game reset! Starting new round...\n";
}
    // Your original game logic functions
    double gearEffect(string gear) {
        if (gear == "Clockwise") return 5.0;
        else if (gear == "Counterclockwise") return -5.0;
        else return 0.0;
    }

    double valveMultiplier(string valve) {
        if (valve == "Open") return 2.0;
        else if (valve == "Partial") return 1.0;
        else return 0.0;
    }

    double dialMultiplier(int dial) {
        return dial / 10.0;
    }

    pair<double, double> leverMultiplier(string lever) {
        if (lever == "Up") return {1.0, 0.0};
        else if (lever == "Down") return {0.0, 1.0};
        else return {0.5, 0.5};
    }
   

    void updateGameState() {
        lock_guard<mutex> lock(stateMutex);

        cout << "\n=== Before Update ===\n";
    cout << "Pressure: " << gameState.machine.pressure << "\n";
    cout << "Temperature: " << gameState.machine.temperature << "\n";
        
        double base = gearEffect(gameState.mechanical.gear);
        double effect = base * valveMultiplier(gameState.mechanical.valve) * 
                       dialMultiplier(gameState.mechanical.dial);

        pair<double,double> leverMap = leverMultiplier(gameState.mechanical.lever);
        double pressureChange = effect * leverMap.first;
        double tempChange = effect * leverMap.second;

        // Debug intermediate calculations
    cout << "Base effect: " << base << "\n";
    cout << "Valve multiplier: " << valveMultiplier(gameState.mechanical.valve) << "\n";
    cout << "Dial multiplier: " << dialMultiplier(gameState.mechanical.dial) << "\n";
    cout << "Lever multipliers: P=" << leverMap.first << " T=" << leverMap.second << "\n";
    cout << "Final changes: P=" << pressureChange << " T=" << tempChange << "\n";


        // Apply switch stabilizer
        if (gameState.electrical.switchA == "On") {
            pressureChange /= 2.0;
            tempChange /= 2.0;
        }

        gameState.machine.pressure += pressureChange;
        gameState.machine.temperature += tempChange;

           cout << "=== After Update ===\n";
    cout << "New Pressure: " << gameState.machine.pressure << "\n";
    cout << "New Temperature: " << gameState.machine.temperature << "\n";
    

        // Apply button reset if pressed
        if (gameState.electrical.button == "Pressed") {
            printf("Button pressed: Machine reset to safe values.\n");
            gameState.machine.pressure = 100.0;
            gameState.machine.temperature = 200.0;
            // gameState.electrical.button = "Idle";
        }

        // Check win/fail conditions
        if (gameState.machine.pressure < 50.0 || gameState.machine.pressure > 200.0 || 
            gameState.machine.temperature < 100.0 || gameState.machine.temperature > 400.0) {
            gameState.gameFailed = true;
            gameState.gameActive = false;
        }

        if (abs(gameState.machine.pressure - gameState.targetPressure) <= 10.0 &&
            abs(gameState.machine.temperature - gameState.targetTemperature) <= 10.0) {
            gameState.gameWon = true;
            gameState.gameActive = false;
        }

        gameState.timeLeft--;
        if (gameState.timeLeft <= 0) {
            gameState.gameActive = false;
        }
    }

void gameLoop() {
    thread mechanicalThread(&GameServer::handleMechanicalPlayer, this);
    thread electricalThread(&GameServer::handleElectricalPlayer, this);

    // Main game loop - continues until players disconnect
    while (playersConnected) {
        while (gameState.gameActive && playersConnected) {
            updateGameState();
            sendGameStateToPlayers();
            this_thread::sleep_for(chrono::seconds(1));
        }

        // Game ended - send final state and wait for play again decisions
        if (playersConnected) {
            sendGameStateToPlayers();
            
            if (gameState.gameWon) {
                cout << "Game Won! Machine stabilized!\n";
            } else if (gameState.gameFailed) {
                cout << "Game Failed! Machine failure!\n";
            } else {
                cout << "Game Over! Time expired!\n";
            }
            
            cout << "Waiting for players to decide if they want to play again...\n";
            
            // Wait for play again decision or disconnection
            while (!gameState.gameActive && playersConnected) {
                if (gameState.mechanicalWantsReplay && gameState.electricalWantsReplay) {
                    resetGame();
                    break;
                }
                    sendGameStateToPlayers();
                    this_thread::sleep_for(chrono::milliseconds(500));
            }
        }
    }

        if (mechanicalThread.joinable()) mechanicalThread.join();
        if (electricalThread.joinable()) electricalThread.join();
    }
};

int main() {
    GameServer server;
    


    if (!server.startServer()) {
        cout << "Failed to start server!\n";
        return 1;
    }

    server.gameLoop();
    return 0;
}