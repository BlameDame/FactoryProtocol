#include <SFML/Graphics.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <chrono>
#include <cmath>
#include "menus.h"
#include "audio.h"
using namespace std;
using namespace sf;

// namespace Menus {

int Menus::MainMenu(RenderWindow &window, Font &font, bool &gameStart, bool &newGame) {
        Text title("Factory Protocol", font, 69);
        title.setStyle(Text::Bold);
        title.setFillColor(Color::Black);
        title.setPosition(window.getSize().x / 2.f - title.getLocalBounds().width / 2.f, window.getSize().y / 2.f - title.getLocalBounds().height / 2.f - 200);
    
        Text waterMark("Aquarius Games", font, 20);
        waterMark.setStyle(Text::Italic);
        waterMark.setFillColor(Color::White);
        waterMark.setPosition(window.getSize().x / 2.f - waterMark.getLocalBounds().width / 2.f + 85, window.getSize().y / 2.f - waterMark.getLocalBounds().height / 2.f - 150);
    
        Text startButton("Start New Game", font, 50);
        startButton.setStyle(Text::Italic);
        startButton.setFillColor(Color::White);
        startButton.setPosition(window.getSize().x / 2.f - startButton.getLocalBounds().width / 2.f, window.getSize().y / 2.f - startButton.getLocalBounds().height / 2.f + 50);
        Text loadButton("Load Game", font, 50);
        loadButton.setStyle(Text::Italic);
        loadButton.setFillColor(Color::Green);
        loadButton.setPosition(window.getSize().x / 2.f - loadButton.getLocalBounds().width / 2.f, window.getSize().y / 2.f - loadButton.getLocalBounds().height / 2.f + 150);
    
        Text exitButton("Exit", font, 50);
        exitButton.setStyle(Text::Underlined);
        exitButton.setFillColor(Color::Red);
        exitButton.setPosition(window.getSize().x / 2.f - exitButton.getLocalBounds().width / 2.f, window.getSize().y / 2.f - exitButton.getLocalBounds().height / 2.f + 300);
    
        Image background;
        if (!background.loadFromFile("/home/dame/brck/background.png"))
        {
            cerr << "Error Loading Image" << endl;
            return -1;
        }
        Texture backgroundTexture;
        backgroundTexture.loadFromImage(background);
        Sprite background1(backgroundTexture);
        background1.setScale(
            static_cast<float>(window.getSize().x) / backgroundTexture.getSize().x,
            static_cast<float>(window.getSize().y) / backgroundTexture.getSize().y
        );
        
        audios.startPlaylist(true); // Start playlist in random mode
        while (window.isOpen() && !gameStart)
        {
            Event event;
            while (window.pollEvent(event))
            {
                if (event.type == Event::Closed)
                    window.close();
                if (event.type == Event::KeyPressed && event.key.code == Keyboard::Escape)
                    window.close();
            
                if (event.type == Event::MouseButtonPressed && event.mouseButton.button == Mouse::Left)
                {
                    if (startButton.getGlobalBounds().contains(event.mouseButton.x, event.mouseButton.y))
                    {
                    gameStart = true;
                    newGame = true;
                    return 1;
                   }
                    if (loadButton.getGlobalBounds().contains(event.mouseButton.x, event.mouseButton.y))
                    {
                       continue;
                    }
                    if (exitButton.getGlobalBounds().contains(event.mouseButton.x, event.mouseButton.y))
                    {
                        window.close();
                    }
                }
            }
            audios.update(); // Update audio manager to handle playlist
            window.clear(Color::Blue);
            window.draw(background1);
            window.draw(title);
            window.draw(waterMark);
            window.draw(startButton);
            window.draw(loadButton);    
            window.draw(exitButton);    
            window.display();
        }       
        return 0;
    }
// }