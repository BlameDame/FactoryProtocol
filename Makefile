# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread
LDFLAGS = -pthread
# Changed order of SFML libs to fix linking
SFML_LIBS = `pkg-config --libs sfml-all`

# Target executables
TARGETS = server mechanical_client electrical_client

# Source files
SERVER_SRC = server.cpp
MECHANICAL_SRC = mechanical_client.cpp
ELECTRICAL_SRC = electrical_client.cpp

# Build all targets
all: $(TARGETS)

# Server executable
server: $(SERVER_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

# Mechanical client executable (fixed linking order)
mechanical_client: $(MECHANICAL_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $< $(SFML_LIBS) $(LDFLAGS)

# Electrical client executable
electrical_client: $(ELECTRICAL_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $< $(SFML_LIBS) $(LDFLAGS)

# Clean build artifacts
clean:
	rm -f $(TARGETS)

# Install (optional - copies to /usr/local/bin)
install: all
	sudo cp $(TARGETS) /usr/local/bin/

# Uninstall
uninstall:
	sudo rm -f $(addprefix /usr/local/bin/, $(TARGETS))

# Run server (for testing)
run-server: server
	./server

# Run mechanical client (for testing)
run-mechanical: mechanical_client
	./mechanical_client

# Run electrical client (for testing)
run-electrical: electrical_client
	./electrical_client

# Debug builds (with debugging symbols)
debug: CXXFLAGS += -g -DDEBUG
debug: all

# Help
help:
	@echo "Available targets:"
	@echo "  all              - Build all executables"
	@echo "  server           - Build server only"
	@echo "  mechanical_client - Build mechanical client only"
	@echo "  electrical_client - Build electrical client only"
	@echo "  clean            - Remove build artifacts"
	@echo "  debug            - Build with debug symbols"
	@echo "  run-server       - Build and run server"
	@echo "  run-mechanical   - Build and run mechanical client"
	@echo "  run-electrical   - Build and run electrical client"
	@echo "  install          - Install to system (requires sudo)"
	@echo "  uninstall        - Remove from system (requires sudo)"

.PHONY: all clean install uninstall run-server run-mechanical run-electrical debug help