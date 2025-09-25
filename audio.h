#ifndef AUDIO_H
#define AUDIO_H

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
    AudioManager();
    void toggleMuteMusic();
    void playSound(const string& soundName);
    void playSound();
    void startPlaylist(bool random = false);
    void stopPlaylist();
    void nextTrack();
    bool isCurrentTrackFinished();
    void update();
    void stopMusic();
};

#endif // AUDIO_H