#include <SFML/Graphics.hpp>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "audio.cpp"
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <chrono>

using namespace std;

class ElectricalClient {
private:
    int clientSocket;
    bool connected;
    bool debugMode;
    
    // Current machine state (received from server)
    double pressure;
    double temperature;
    double targetPressure;
    double targetTemperature;
    int timeLeft;
    bool gameActive;
    bool gameWon;
    bool gameFailed;
    bool mechanicalWantsReplay;
    bool electricalWantsReplay;

    // SFML members
    sf::RenderWindow window;
    sf::Font font;

    AudioManager audioManager;
    
    // UI Elements
    sf::RectangleShape switchButton;
    sf::RectangleShape stabilizeButton;
    sf::Text statusText;
    sf::Text switchText;
    sf::Text buttonText;

    struct LocalControls {
        string switchA = "Off";
        string button = "Idle";
    } controls;

    void initializeGraphics() {
        window.create(sf::VideoMode(800, 600), "Machine Game - Electrical");
        window.setFramerateLimit(60);
        
        if (!font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")) {
            cout << "Error loading font\n";
        }
        
        // Initialize switch button
        switchButton.setSize(sf::Vector2f(100, 50));
        switchButton.setPosition(100, 300);
        switchButton.setFillColor(sf::Color::Red);
        
        // Initialize stabilize button
        stabilizeButton.setSize(sf::Vector2f(100, 50));
        stabilizeButton.setPosition(300, 300);
        stabilizeButton.setFillColor(sf::Color::Blue);
        
        // Initialize text elements
        statusText.setFont(font);
        statusText.setCharacterSize(24);
        statusText.setFillColor(sf::Color::White);
        statusText.setPosition(10, 10);
        
        switchText.setFont(font);
        switchText.setString("Switch: OFF");
        switchText.setPosition(100, 360);
        
        buttonText.setFont(font);
        buttonText.setString("Button: IDLE");
        buttonText.setPosition(300, 360);
    }

    void handleEvents() {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
            
            if (event.type == sf::Event::MouseButtonPressed) {
                // Switch toggle
                if (event.mouseButton.button == sf::Mouse::Left) {
                    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
                    if (switchButton.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
                        controls.switchA = (controls.switchA == "Off") ? "On" : "Off";
                        sendElectricalUpdate(controls.switchA, controls.button);
                    }
                    // Stabilize button
                    if (stabilizeButton.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
                        controls.button = "Pressed";
                        sendElectricalUpdate(controls.switchA, controls.button);
                    }
                }
            }
            
            if (event.type == sf::Event::MouseButtonReleased) {
                if (event.mouseButton.button == sf::Mouse::Left) {
                        controls.button = "Idle";
                        sendElectricalUpdate(controls.switchA, controls.button);
                    
                }
            }
        }
    }

    void render() {
        window.clear(sf::Color(50, 50, 50));
        
        // Update status text
        stringstream ss;
        ss << "Pressure: " << pressure << "\n"
           << "Temperature: " << temperature << "\n"
           << "Time Left: " << timeLeft;
        statusText.setString(ss.str());
        
        // Update control texts
        switchText.setString("Switch: " + controls.switchA);
        buttonText.setString("Button: " + controls.button);
        
        // Draw everything
        window.draw(statusText);
        window.draw(switchButton);
        window.draw(stabilizeButton);
        window.draw(switchText);
        window.draw(buttonText);
        
        window.display();
    }

public:
    ElectricalClient() {
        clientSocket = -1;
        connected = false;
        
        // Initialize display values
        pressure = 100.0;
        temperature = 200.0;
        targetPressure = 150.0;
        targetTemperature = 300.0;
        timeLeft = 60;
        gameActive = true;
        gameWon = false;
        gameFailed = false;
        mechanicalWantsReplay = false;
        electricalWantsReplay = false;

        initializeGraphics();
    }

    ~ElectricalClient() {
        if (clientSocket != -1) close(clientSocket);
        debugMode = false;
    }

    bool connectToServer(const string& serverIP = "127.0.0.1") {
        clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket < 0) {
            perror("socket creation failed");
            return false;
        }

        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        if (inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr) <= 0) {
            perror("invalid address");
            close(clientSocket);
            clientSocket = -1;
            return false;
        }
        serverAddr.sin_port = htons(8888);

        if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            perror("connection failed");
            close(clientSocket);
            clientSocket = -1;
            return false;
        }

        connected = true;
        cout << "Connected to server as Electrical Player!\n";
        return true;
    }

    void receiveGameState() {
        char buffer[1024];
        while (connected) {
            ssize_t bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
            if (bytesReceived > 0) {
                buffer[bytesReceived] = '\0';
                string message(buffer);
                
                // Parse game state: "STATE|pressure|temp|targetP|targetT|time|active|won|failed"
                if (message.substr(0, 6) == "STATE|") {
                    vector<string> tokens;
                    stringstream ss(message);
                    string token;
                    
                    while (getline(ss, token, '|')) {
                        tokens.push_back(token);
                    }
                    
                    if (tokens.size() >= 11) {
                        try {
                            pressure = stod(tokens[1]);
                            temperature = stod(tokens[2]);
                            targetPressure = stod(tokens[3]);
                            targetTemperature = stod(tokens[4]);
                            timeLeft = stoi(tokens[5]);
                            gameActive = (tokens[6] == "1");
                            gameWon = (tokens[7] == "1");
                            gameFailed = (tokens[8] == "1");
                            mechanicalWantsReplay = (tokens[9] == "1");
                            electricalWantsReplay = (tokens[10] == "1");
                        } catch (const exception& e) {
                            cout << "Error parsing game state: " << e.what() << "\n";
                        }
                    }
                }
            } else if (bytesReceived == 0) {
                cout << "Server disconnected\n";
                connected = false;
                break;
            } else {
                perror("recv failed");
                connected = false;
                break;
            }
        }
    }

void showPlayAgainPrompt() {
    cout << "=== PLAY AGAIN? ===\n";
    cout << "Mechanical player wants replay: " << (mechanicalWantsReplay ? "YES" : "waiting...") << "\n";
    cout << "Electrical player wants replay: " << (electricalWantsReplay ? "YES" : "waiting...") << "\n";
    cout << "Type: 'replay yes' or 'replay no'\n\n";
}
void sendPlayAgainResponse(bool wantsReplay) {
    string message = "PLAY_AGAIN|" + string(wantsReplay ? "YES" : "NO") + "\n";
    ssize_t sent = send(clientSocket, message.c_str(), message.length(), MSG_NOSIGNAL);
    if (sent < 0) {
        perror("send failed");
    }
}

void sendElectricalUpdate(const string& switchState, const string& buttonState) {
        string message = "ELEC|" + switchState + "|" + buttonState + "\n";
        ssize_t sent = send(clientSocket, message.c_str(), message.length(), MSG_NOSIGNAL);
        if (sent < 0) {
            perror("send failed");
        }
    }

void run() {
        thread receiveThread(&ElectricalClient::receiveGameState, this);
        
        // Wait a moment for initial state
    this_thread::sleep_for(chrono::milliseconds(500));

        while (window.isOpen() && connected){
            handleEvents();
            audioManager.update();
            render();
        }

        if (receiveThread.joinable()) {
            receiveThread.join();
        }
}
};

int main() {
    ElectricalClient client;
    
    cout << "Enter server IP (or press Enter for localhost): ";
    string serverIP;
    getline(cin, serverIP);
    if (serverIP.empty()) {
        serverIP = "127.0.0.1";
    }

    if (!client.connectToServer(serverIP)) {
        cout << "Failed to connect to server!\n";
        return 1;
    }

    client.run();
    return 0;
}