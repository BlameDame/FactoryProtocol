# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread
LDFLAGS = -pthread
SFML_LIBS = `pkg-config --libs sfml-all`

# Object files
AUDIO_OBJ = audio.o
MENU_OBJ = menu.o

# Target executables
TARGETS = server mechanical_client electrical_client

# Source files
SERVER_SRC = server.cpp
MECHANICAL_SRC = mechanical_client.cpp
ELECTRICAL_SRC = electrical_client.cpp
AUDIO_SRC = audio.cpp
MENU_SRC = menu.cpp

# Build all targets
all: $(TARGETS)

# Object file compilation
$(AUDIO_OBJ): $(AUDIO_SRC) audio.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(MENU_OBJ): $(MENU_SRC) menus.h audio.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Server executable (doesn't need SFML or the modules)
server: $(SERVER_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

# Mechanical client executable
mechanical_client: $(MECHANICAL_SRC) $(AUDIO_OBJ) $(MENU_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $< $(AUDIO_OBJ) $(MENU_OBJ) $(SFML_LIBS) $(LDFLAGS)

# Electrical client executable
electrical_client: $(ELECTRICAL_SRC) $(AUDIO_OBJ) $(MENU_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $< $(AUDIO_OBJ) $(MENU_OBJ) $(SFML_LIBS) $(LDFLAGS)

# Clean build artifacts
clean:
	rm -f $(TARGETS) *.o

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

# Test build (compile only, don't link)
test-compile: $(AUDIO_OBJ) $(MENU_OBJ)
	$(CXX) $(CXXFLAGS) -fsyntax-only $(SERVER_SRC)
	$(CXX) $(CXXFLAGS) -fsyntax-only $(MECHANICAL_SRC)
	$(CXX) $(CXXFLAGS) -fsyntax-only $(ELECTRICAL_SRC)
	@echo "All source files compile successfully"

# Check for missing dependencies
check-deps:
	@echo "Checking for SFML installation..."
	@pkg-config --exists sfml-all && echo "SFML found" || echo "SFML not found - install with: sudo apt-get install libsfml-dev"
	@echo "Checking for required headers..."
	@ls -la *.h 2>/dev/null || echo "Missing header files"
	@echo "Checking for asset directories..."
	@ls -la assets/ 2>/dev/null || echo "Missing assets directory"

# Create directory structure
setup-dirs:
	mkdir -p assets/images assets/audios
	@echo "Created asset directories"
	@echo "Place your images in assets/images/ and audio files in assets/audios/"

# Help
help:
	@echo "Available targets:"
	@echo "  all              - Build all executables"
	@echo "  clean            - Remove build artifacts"
	@echo "  server           - Build server only"
	@echo "  mechanical_client - Build mechanical client only"
	@echo "  electrical_client - Build electrical client only"
	@echo "  debug            - Build with debug symbols"
	@echo "  test-compile     - Test compilation without linking"
	@echo "  check-deps       - Check for required dependencies"
	@echo "  setup-dirs       - Create asset directory structure"
	@echo "  run-server       - Build and run server"
	@echo "  run-mechanical   - Build and run mechanical client"
	@echo "  run-electrical   - Build and run electrical client"
	@echo "  install          - Install to system (requires sudo)"
	@echo "  uninstall        - Remove from system (requires sudo)"

.PHONY: all clean install uninstall run-server run-mechanical run-electrical debug help test-compile check-deps setup-dirs