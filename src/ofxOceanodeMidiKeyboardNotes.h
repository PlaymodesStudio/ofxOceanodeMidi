#pragma once
#include <algorithm>
#include <vector>

// Hardware-independent note allocation, shared by every MIDI keyboard binding.
class ofxOceanodeMidiKeyboardNotes {
public:
    struct Note { int pitch; float velocity; bool down; };
    std::vector<Note> held;
    bool sustain = false;
    void noteOn(int pitch, int velocity) {
        erase(pitch);
        held.push_back({pitch, float(velocity) / 127.0f, true});
    }
    void noteOff(int pitch) {
        if(sustain) {
            for(auto& note : held) if(note.pitch == pitch) note.down = false;
        } else erase(pitch);
    }
    void pedal(bool down) {
        sustain = down;
        if(!down) held.erase(std::remove_if(held.begin(), held.end(), [](const Note& n){ return !n.down; }), held.end());
    }
    void clear() { held.clear(); sustain = false; }
    std::vector<int> allocate(int voices, const std::vector<int>& previous, bool kit = false) const {
        std::vector<int> next(std::max(1, voices), -1);
        if(kit) {
            for(const auto& n : held) if(n.pitch >= 36 && n.pitch < 36 + int(next.size())) next[n.pitch - 36] = n.pitch;
            return next;
        }
        const int begin = std::max(0, int(held.size()) - int(next.size()));
        for(int v = 0; v < std::min(int(next.size()), int(previous.size())); ++v)
            for(int k = begin; k < int(held.size()); ++k)
                if(previous[v] == held[k].pitch) next[v] = previous[v];
        for(int k = begin; k < int(held.size()); ++k) {
            if(std::find(next.begin(), next.end(), held[k].pitch) != next.end()) continue;
            auto slot = std::find(next.begin(), next.end(), -1);
            if(slot != next.end()) *slot = held[k].pitch;
        }
        return next;
    }
    float velocity(int pitch) const {
        for(const auto& note : held) if(note.pitch == pitch) return note.velocity;
        return 0;
    }
private:
    void erase(int pitch) {
        held.erase(std::remove_if(held.begin(), held.end(), [pitch](const Note& n){ return n.pitch == pitch; }), held.end());
    }
};
