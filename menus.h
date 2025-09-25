#ifndef MENUS_H
#define MENUS_H

#include <SFML/Graphics.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <chrono>
#include <cmath>
#include "audio.h"

using namespace sf;
using namespace std;

class Menus {
private:
    AudioManager audios;

public:
    Menus() = default;
    
    int MainMenu(RenderWindow &window, Font &font, bool &gameStart, bool &newGame);
};

#endif // MENUS_H