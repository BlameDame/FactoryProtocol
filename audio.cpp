// #include <SFML/Audio.hpp>
// #include <algorithm>
// #include <iostream>
// #include <vector>
// #include <string>
// #include <map>
// #include <cstdlib>
#include "audio.h"
using namespace sf;
using namespace std;

    AudioManager::AudioManager() : musicMuted(false), currentTrackIndex(0), randomMode(false), playlistActive(false), currentlyPlaying("") {
    vector<pair<string, string>> soundFiles = {
        {"Chester_Fries", "assets/audios/Chester_Fries.mp3"},
        {"Fat_Tuesdays", "assets/audios/Fat_Tuesdays.mp3"},
        {"ONO", "assets/audios/ONO.mp3"},
        {"please", "assets/audios/please.mp3"},
        {"Teaser", "assets/audios/Teaser.mp3"},
        {"Toast", "assets/audios/Toast.mp3"},
    };

    // Build playlist from soundFiles
    for (const auto& soundFile : soundFiles) {
        playlist.push_back(soundFile.first);
    }

    // Load all sounds dynamically
    for (const auto& soundFile : soundFiles) {
        if (soundBuffers[soundFile.first].loadFromFile(soundFile.second)) {
            sounds[soundFile.first].setBuffer(soundBuffers[soundFile.first]);
            sounds[soundFile.first].setVolume(60);
            cout << "Loaded sound: " << soundFile.first << "\n";
        } else {
            cout << "Warning: Could not load " << soundFile.first << " from " << soundFile.second << "\n";
        }
    }
}

void AudioManager::toggleMuteMusic() {
    musicMuted = !musicMuted;
    if (musicMuted) {
        // Stop current song if playing
        if (!currentlyPlaying.empty() && sounds.find(currentlyPlaying) != sounds.end()) {
            sounds[currentlyPlaying].pause();
        }
        cout << "Music muted\n";
    } else {
        // Resume current song if it was playing
        if (!currentlyPlaying.empty() && sounds.find(currentlyPlaying) != sounds.end()) {
            sounds[currentlyPlaying].play();
        }
        cout << "Music unmuted\n";
    }
}

void AudioManager::playSound(const string& soundName) {
    if (musicMuted) return;
    
    if (sounds.find(soundName) != sounds.end()) {
        sounds[soundName].play();
    } else {
        cout << "Warning: Sound '" << soundName << "' not found\n";
    }
}

//play songs in order from list above
void AudioManager::playSound() {
    if (!playlistActive || musicMuted) return;
    
    if (playlist.empty()) {
        cout << "No songs in playlist\n";
        return;
    }
    
    // Stop current song if playing
    if (!currentlyPlaying.empty() && sounds.find(currentlyPlaying) != sounds.end()) {
        sounds[currentlyPlaying].stop();
    }
    
    // Play next song based on mode
    if (randomMode) {
        currentTrackIndex = rand() % playlist.size();
    }
    
    currentlyPlaying = playlist[currentTrackIndex];
    if (sounds.find(currentlyPlaying) != sounds.end()) {
        sounds[currentlyPlaying].play();
        cout << "Now playing: " << currentlyPlaying << "\n";
    }
    
    // Advance to next track for sequential mode
    if (!randomMode) {
        currentTrackIndex = (currentTrackIndex + 1) % playlist.size();
    }
}

void AudioManager::startPlaylist(bool random) {
    randomMode = random;
    playlistActive = true;
    currentTrackIndex = 0;
    cout << "Starting playlist in " << (random ? "random" : "sequential") << " mode\n";
    playSound(); // Start first song
}

void AudioManager::stopPlaylist() {
    playlistActive = false;
    if (!currentlyPlaying.empty() && sounds.find(currentlyPlaying) != sounds.end()) {
        sounds[currentlyPlaying].stop();
    }
    currentlyPlaying = "";
    cout << "Playlist stopped\n";
}

void AudioManager::nextTrack() {
    if (!playlistActive) return;
    playSound();
}

bool AudioManager::isCurrentTrackFinished() {
    if (currentlyPlaying.empty()) return true;
    if (sounds.find(currentlyPlaying) == sounds.end()) return true;
    return sounds[currentlyPlaying].getStatus() == Sound::Stopped;
}

void AudioManager::update() {
    // Auto-advance to next track when current one finishes
    if (playlistActive && isCurrentTrackFinished()) {
        playSound();
    }
}

void AudioManager::stopMusic() {
    stopPlaylist();
}