#include <SFML/Audio.hpp>
#include <algorithm>
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <cstdlib>
using namespace sf;
using namespace std;

class AudioManager {
private:
    map<string, SoundBuffer> soundBuffers;
    map<string, Sound> sounds;
    bool musicMuted;
    vector<string> playlist;
    int currentTrackIndex;
    bool randomMode;
    bool playlistActive;
    string currentlyPlaying;

public:
    AudioManager() : musicMuted(false), currentTrackIndex(0), randomMode(false), playlistActive(false), currentlyPlaying("") {

        vector<pair<string, string>> soundFiles = {
            {"Chester_Fries", "assets/audios/Chester_Fries.m4a"},
            {"Dead_Dawgs", "assets/audios/Dead_Dawgs.m4a"},
            {"Fat_Tuesdays", "assets/audios/Fat_Tuesdays.m4a"},
            {"Mrs_dumpa_rumpa", "assets/audios/Mrs_dumpa_rumpa.m4a"},
            {"ONO", "assets/audios/ONO.m4a"},
            {"Toast", "assets/audios/Toast.mp3"},
            {"tonight_sheff", "assets/audios/tonight_sheff.m4a"},
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

    void toggleMuteMusic() {
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

    void playSound(const string& soundName) {
        if (musicMuted) return;
        
        if (sounds.find(soundName) != sounds.end()) {
            sounds[soundName].play();
        } else {
            cout << "Warning: Sound '" << soundName << "' not found\n";
        }
    }

    //play songs in order from list above
    void playSound() {
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
    
    void startPlaylist(bool random = false) {
        randomMode = random;
        playlistActive = true;
        currentTrackIndex = 0;
        cout << "Starting playlist in " << (random ? "random" : "sequential") << " mode\n";
        playSound(); // Start first song
    }
    
    void stopPlaylist() {
        playlistActive = false;
        if (!currentlyPlaying.empty() && sounds.find(currentlyPlaying) != sounds.end()) {
            sounds[currentlyPlaying].stop();
        }
        currentlyPlaying = "";
        cout << "Playlist stopped\n";
    }
    
    void nextTrack() {
        if (!playlistActive) return;
        playSound();
    }
    
    bool isCurrentTrackFinished() {
        if (currentlyPlaying.empty()) return true;
        if (sounds.find(currentlyPlaying) == sounds.end()) return true;
        return sounds[currentlyPlaying].getStatus() == Sound::Stopped;
    }
    
    void update() {
        // Auto-advance to next track when current one finishes
        if (playlistActive && isCurrentTrackFinished()) {
            playSound();
        }
    }

    void stopMusic() {
        stopPlaylist();
    }
};