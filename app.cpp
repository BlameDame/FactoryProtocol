#include "electrical_client.cpp"
#include "mechanical_client.cpp"
#include "server.cpp"

int main() {
    // Start the server in a separate thread
    std::thread serverThread([]() {
        GameServer server;
        if (server.startServer()) {
            server.gameLoop();
        } else {
            std::cout << "Failed to start server!\n";
        }
    });
    serverThread.detach();

    // Start the mechanical client
    MechanicalClient mechanicalClient;
    mechanicalClient.run();

    // Start the electrical client
    ElectricalClient electricalClient;
    electricalClient.run();

    return 0;
}