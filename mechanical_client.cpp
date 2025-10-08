#include <SFML/Graphics.hpp>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "menus.h"
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <chrono>
#include <cmath>

using namespace std;
using namespace sf;

class MechanicalClient {
private:
    int clientSocket;
    bool connected;
    bool gameStart;
    bool newGame;
    struct LeverAnimation {
        float currentFrame = 1.0f;     
        string targetState = "UP"; 
        string currentState = "Middle"; 
        bool isGold = false;           
        float animationSpeed = 6.0f;   
    } leverAnim;
    struct LocalControls {
        string gear = "Stopped";
        string lever = "Middle";
        string valve = "Closed";
        int dial = 5;
    } controls;
    RenderWindow window;
    Font font;
    
    enum LeverFrame {
        BLACK_DOWN = 0,
        BLACK_MID = 1,  
        BLACK_UP = 2,
        GOLD_DOWN = 3,
        GOLD_MID = 4,
        GOLD_UP = 5
    };
    
    sf::IntRect leverFrameRect;

    Menus menus;

    // UI Elements
    RectangleShape pressureGauge;
    RectangleShape temperatureGauge;
    Texture gearTexture;
    Sprite gearSprite;
    Texture leverTexture;
    Sprite leverSprite;
    Clock spriteClock;
    Text statusText;
    Text gearStopText;
    Text leverResetText;

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

    void initializeLeverFrames() {
    
    int frameWidth = (leverTexture.getSize().x / 3);
    int frameHeight = leverTexture.getSize().y / 2;
    
    leverFrameRect = sf::IntRect(0, 0, frameWidth, frameHeight);
    leverSprite.setTextureRect(leverFrameRect);
    
    // Initialize animation state to middle frame
    leverAnim.currentFrame = (leverAnim.isGold ? GOLD_MID : BLACK_MID);
    leverAnim.currentState = "Middle";
    leverAnim.targetState = "Middle";
}

    void initializeGraphics() {
        window.create(VideoMode(800, 600), "Machine Game - Mechanical");
        window.setFramerateLimit(60);
        
        if (!font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")) {
            std::cout << "Error loading font\n";
        }
        
        // Initialize UI elements
        pressureGauge.setSize(Vector2f(30, 200));
        pressureGauge.setPosition(250, 200);
        pressureGauge.setFillColor(Color::Red);
        
        temperatureGauge.setSize(Vector2f(30, 200));
        temperatureGauge.setPosition(350, 200);
        temperatureGauge.setFillColor(sf::Color::Yellow);
        
        if (!gearTexture.loadFromFile("./assets/images/gear.png")) {
            std::cout << "Warning: failed to load ./assets/images/gear.png — using placeholder\n";
            sf::Image img; img.create(128, 128, sf::Color(150,150,150));
            gearTexture.loadFromImage(img);
        }
        if (!leverTexture.loadFromFile("./assets/images/lever.png")) {
            std::cout << "Warning: failed to load ./assets/images/lever.png — using placeholder\n";
            sf::Image img; img.create(32, 128, sf::Color(120,120,120));
            leverTexture.loadFromImage(img);
        }

        gearSprite.setTexture(gearTexture);
        gearSprite.setOrigin(gearTexture.getSize().x / 2.f, gearTexture.getSize().y / 2.f);
        gearSprite.setScale(4.5f, 4.5f);
        gearSprite.setPosition(200.f, 470.f);

        int frameWidth = leverTexture.getSize().x / 3;
        leverSprite.setTexture(leverTexture);
        leverSprite.setOrigin(frameWidth / 2.f, (leverTexture.getSize().y / 2) * 0.1f);
        leverSprite.setScale(2.f, 2.f);
        leverSprite.setPosition(450.f, 400.f);
        
        initializeLeverFrames();

        statusText.setFont(font);
        statusText.setCharacterSize(24);
        statusText.setFillColor(sf::Color::White);
        statusText.setPosition(10, 10);

        gearStopText.setFont(font);
        gearStopText.setCharacterSize(16);
        gearStopText.setFillColor(sf::Color::White);
        gearStopText.setString("Click Gear or press 'G' to toggle\nPress 'S' to Stop");
        gearStopText.setPosition(200, 550);

        leverResetText.setFont(font);
        leverResetText.setCharacterSize(16);
        leverResetText.setFillColor(sf::Color::White);
        leverResetText.setString("Click Lever or press 'L' to toggle\nPress 'M' for Middle");
        leverResetText.setPosition(400, 550);

    }
    void updateSpriteStates() {
        // compute delta time
        float dt = spriteClock.restart().asSeconds();

        // Gear: continuous rotation when running; stopped = no rotation
        updateLeverAnimationSmooth(dt);
        float gearSpeedDegPerSec = 0.f;
        if (controls.gear == "Clockwise") gearSpeedDegPerSec = 180.f;          // tweak as needed
        else if (controls.gear == "Counterclockwise") gearSpeedDegPerSec = -180.f;
        gearSprite.rotate(gearSpeedDegPerSec * dt); // accumulates rotation over frames
    }

    void updateLeverAnimationSmooth(float deltaTime) {
    LeverFrame targetFrame;

    if (controls.lever == "Up") {
        targetFrame = leverAnim.isGold ? GOLD_UP : BLACK_UP;
    } else if (controls.lever == "Down") {
        targetFrame = leverAnim.isGold ? GOLD_DOWN : BLACK_DOWN;
    } else {
        targetFrame = leverAnim.isGold ? GOLD_MID : BLACK_MID;
    }

    float targetFrameFloat = static_cast<float>(targetFrame);
    
    if (abs(leverAnim.currentFrame - targetFrameFloat) > 0.1f) {
        // Smoothly interpolate towards target
        leverAnim.currentFrame += (targetFrameFloat - leverAnim.currentFrame) * leverAnim.animationSpeed * deltaTime;
    } else {
        // Animation complete - snap to target and update state
        leverAnim.currentFrame = targetFrameFloat;
        leverAnim.currentState = controls.lever;
    }

    // Update sprite frame for 3x2 layout
    int displayFrame = static_cast<int>(round(leverAnim.currentFrame));
    displayFrame = max(0, min(5, displayFrame));
    
    // Calculate 2D position in sprite sheet
    int frameWidth = leverTexture.getSize().x / 3;
    int frameHeight = leverTexture.getSize().y / 2;
    
    int col = displayFrame % 3;  // Column: 0=Up, 1=Mid, 2=Down
    int row = displayFrame / 3;  // Row: 0=Black, 1=Gold
    
    leverFrameRect.left = col * frameWidth;
    leverFrameRect.top = row * frameHeight;
    leverFrameRect.width = frameWidth;
    leverFrameRect.height = frameHeight;
    
    leverSprite.setTextureRect(leverFrameRect);
}

    void toggleLeverColor() {
        
        string currentLogicalState = leverAnim.currentState;
        leverAnim.isGold = !leverAnim.isGold;
        
        
        if (currentLogicalState == "Up") {
            leverAnim.currentFrame = leverAnim.isGold ? GOLD_UP : BLACK_UP;
        } else if (currentLogicalState == "Down") {
            leverAnim.currentFrame = leverAnim.isGold ? GOLD_DOWN : BLACK_DOWN;
        } else {
            leverAnim.currentFrame = leverAnim.isGold ? GOLD_MID : BLACK_MID;
        }
    }
    void handleEvents() {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::MouseButtonPressed) {
                if (event.mouseButton.button == sf::Mouse::Left) {
                    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
                    if (leverSprite.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
                        controls.lever = (controls.lever == "Up") ? "Down" : "Up";
                        sendMechanicalUpdate(controls.gear, controls.lever, controls.valve, controls.dial);
                    }
                    if(gearSprite.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
                        controls.gear = (controls.gear == "Clockwise") ? "Counterclockwise" : "Clockwise";
                        sendMechanicalUpdate(controls.gear, controls.lever, controls.valve, controls.dial);
                    }
                }
            }

                                //Keyboard Controls
            if (event.type == sf::Event::KeyPressed) {
                switch (event.key.code) {
                    case sf::Keyboard::G:
                    controls.gear = (controls.gear == "Clockwise") ? "Counterclockwise" : "Clockwise";
                        sendMechanicalUpdate(controls.gear, controls.lever, controls.valve, controls.dial);
                        break;
                    case sf::Keyboard::S:
                    controls.gear = "Stopped";
                        sendMechanicalUpdate(controls.gear, controls.lever, controls.valve, controls.dial);
                        break;
                    case sf::Keyboard::L:
                    controls.lever = (controls.lever == "Up") ? "Down" : "Up";
                        sendMechanicalUpdate(controls.gear, controls.lever, controls.valve, controls.dial);
                        break;
                    case sf::Keyboard::M:
                    controls.lever = "Middle";
                        sendMechanicalUpdate(controls.gear, controls.lever, controls.valve, controls.dial);
                        break;
                    case sf::Keyboard::V:
                    controls.valve = (controls.valve == "Open") ? "Partial" : "Open";
                        sendMechanicalUpdate(controls.gear, controls.lever, controls.valve, controls.dial);
                        break;
                    case sf::Keyboard::C:
                    controls.valve = "Closed";
                        sendMechanicalUpdate(controls.gear, controls.lever, controls.valve, controls.dial);
                        break;
                                                // Dial 0-9 //
                    case sf::Keyboard::Num0: case sf::Keyboard::Num1: case sf::Keyboard::Num2:
                    case sf::Keyboard::Num3: case sf::Keyboard::Num4: case sf::Keyboard::Num5:
                    case sf::Keyboard::Num6: case sf::Keyboard::Num7: case sf::Keyboard::Num8:
                    case sf::Keyboard::Num9:
                    controls.dial = event.key.code - sf::Keyboard::Num0;
                        sendMechanicalUpdate(controls.gear, controls.lever, controls.valve, controls.dial);
                    break;
                    default:
                        break;
                }
            }
        }
    }
    void render() {
        window.clear(sf::Color(50, 50, 50));
        
        // Update gauges based on machine state
        float pressureHeight = (pressure / 200.0f) * 200.0f;
        pressureGauge.setSize(Vector2f(30, pressureHeight));
        
        float tempHeight = (temperature / 400.0f) * 200.0f;
        temperatureGauge.setSize(Vector2f(30, tempHeight));
        
        // Draw UI elements
        window.draw(pressureGauge);
        window.draw(temperatureGauge);

          updateSpriteStates();
        window.draw(pressureGauge);
        window.draw(temperatureGauge);
        window.draw(leverResetText);
        window.draw(gearStopText);
        window.draw(gearSprite);
        window.draw(leverSprite);
        
        // Update status text
        std::stringstream ss;
        ss << "Pressure: " << pressure << "\n"
           << "Temperature: " << temperature << "\n"
           << "Time Left: " << timeLeft << "\n"
           << "\nCurrent Controls:\t(Valve and Gears must be operating)\n"
           << "Gear: " << controls.gear << "\n"
           << "Lever: " << controls.lever << "\n"
           << "Valve: " << controls.valve << "\n"
           << "Dial: " << controls.dial;
        statusText.setString(ss.str());
        window.draw(statusText);
        
        window.display();
    }


public:
    MechanicalClient() {
        clientSocket = -1;
        connected = false;
        
        // Initialize display values
        pressure = 100.0;
        temperature = 200.0;
        targetPressure = 150.0;
        targetTemperature = 300.0;
        timeLeft = 60;
        gameActive = false;
        gameWon = false;
        gameFailed = false;
        mechanicalWantsReplay = false;
        electricalWantsReplay = false;
        initializeGraphics();
    }

    ~MechanicalClient() {
        if (clientSocket != -1) close(clientSocket);
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
        cout << "Connected to server as Mechanical Player!\n";
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

    void sendMechanicalUpdate(const string& gear, const string& lever, 
                             const string& valve, int dial) {
        string message = "MECH|" + gear + "|" + lever + "|" + valve + "|" + to_string(dial) + "\n";
        ssize_t sent = send(clientSocket, message.c_str(), message.length(), MSG_NOSIGNAL);
        if (sent < 0) {
            perror("send failed");
        }
    }
    void sendPlayerReady() {
    string message = "READY\n";
    ssize_t sent = send(clientSocket, message.c_str(), message.length(), MSG_NOSIGNAL);
    if (sent < 0) {
        perror("send ready failed");
    }
}

    void sendPlayAgainResponse(bool wantsReplay) {
    string message = "PLAY_AGAIN|" + string(wantsReplay ? "YES" : "NO") + "\n";
    ssize_t sent = send(clientSocket, message.c_str(), message.length(), MSG_NOSIGNAL);
    if (sent < 0) {
        perror("send failed");
    }
}

void run() {
    thread receiveThread(&MechanicalClient::receiveGameState, this);

    bool playerReady = false;  // Track if this player has clicked start

    while (window.isOpen() && connected) {
        if (!playerReady) {
            // Show menu until player clicks start
            menus.MainMenu(window, font, gameStart, newGame);
            
            if (gameStart) {
                // Player clicked start - they're now ready
                playerReady = true;
                sendPlayerReady();
                cout << "Mechanical player is ready! Waiting for electrical player...\n";
            }
        } else if (gameActive) {
            // Both players ready and game is active - play normally
            handleEvents();
            updateSpriteStates();
            render();
        } else {
            // Player is ready but game not active yet - show waiting screen
            window.clear(sf::Color(50, 50, 50));
            Text waitingText("Waiting for other player...", font, 48);
            waitingText.setFillColor(sf::Color::White);
            waitingText.setPosition(150, 250);
            window.draw(waitingText);
            window.display();
            
            // Handle window events while waiting
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed)
                    window.close();
            }
        }
    }

    if (receiveThread.joinable()) {
        receiveThread.join();
    }
}
};

int main() {
    MechanicalClient client;
    
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