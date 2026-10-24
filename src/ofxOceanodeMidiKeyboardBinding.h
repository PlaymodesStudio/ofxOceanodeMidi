#pragma once
#include "ofxOceanodeMidiBinding.h"
#include "ofxOceanodeMidiKeyboardNotes.h"
#include "ofxOceanodeNodeModel.h"
#include <functional>
#include <memory>

// One binding owns Pitch/Gate/Velocity together. Input callbacks only queue;
// parameter writes, learning and device registration run on the main thread.
class ofxOceanodeMidiKeyboardBinding : public ofxOceanodeAbstractMidiBinding {
public:
    using Instrument = ofxOceanodeTimelineInstrument;
    using Notes = ofParameter<std::vector<float>>;
    using Resolve = std::function<Instrument()>;
    static bool supports(const Instrument& i) {
        auto floats = [](ofxOceanodeAbstractParameter* p){ return p && p->valueType() == typeid(std::vector<float>).name(); };
        return i.voices > 0 && floats(i.gate) && (i.kit || floats(i.pitch)) && (!i.velocity || floats(i.velocity));
    }
    ofxOceanodeMidiKeyboardBinding(Resolve resolve, const std::string& name) : resolve(std::move(resolve)) {
        this->name = name + " (MIDI Keyboard)";
    }
    ~ofxOceanodeMidiKeyboardBinding() { release(); }
    void newMidiMessage(ofxMidiMessage& message) override {
        std::lock_guard<ofMutex> lock(mutex);
        lastsMidiMessage.push_back(message);
    }
    void savePreset(ofJson& json) override {
        ofxOceanodeAbstractMidiBinding::savePreset(json);
        json["Keyboard"] = true;
    }
    void loadPreset(ofJson& json) override {
        release();
        // An unlearned binding must still learn after reloading its preset.
        if(json.value("Port", std::string()).empty()) {
            portName.clear(); isListening = true;
            return;
        }
        ofxOceanodeAbstractMidiBinding::loadPreset(json);
    }
    void update() override {
        std::vector<ofxMidiMessage> messages;
        { std::lock_guard<ofMutex> lock(mutex); messages.swap(lastsMidiMessage); }
        const auto instrument = resolve();
        if(!supports(instrument)) { release(); return; }
        if(gateSource != instrument.gate || pitchSource != instrument.pitch || velocitySource != instrument.velocity) {
            release();
            gateSource = instrument.gate; pitchSource = instrument.pitch; velocitySource = instrument.velocity;
            gate = copy(instrument.gate); pitch = copy(instrument.pitch); velocity = copy(instrument.velocity);
        }
        for(auto& message : messages) {
            if(isListening) {
                if(message.status != MIDI_NOTE_ON || message.velocity == 0) continue;
                portName = message.portName; channel = message.channel; status = MIDI_NOTE_ON;
                messageType = ofxMidiMessage::getStatusString(status);
                isListening = false;
                unregisterUnusedMidiIns.notify(this, portName);
            }
            if(message.portName != portName || message.channel != channel) continue;
            if(message.status == MIDI_NOTE_ON && message.velocity > 0) {
                // Repeated note-ons retrigger even if the same voice keeps it.
                closeNote(message.pitch);
                notes.noteOn(message.pitch, message.velocity);
            } else if(message.status == MIDI_NOTE_OFF || (message.status == MIDI_NOTE_ON && message.velocity == 0)) {
                notes.noteOff(message.pitch);
            } else if(message.status == MIDI_CONTROL_CHANGE && message.control == 64) {
                notes.pedal(message.value >= 64);
            } else if(message.status == MIDI_CONTROL_CHANGE && (message.control == 120 || message.control == 123)) {
                notes.clear();
            } else continue;
            apply(instrument);
        }
        // Also follows changes to the instrument's voice count without a new note.
        if(!slots.empty() || !notes.held.empty()) apply(instrument);
    }
    void release() {
        if(gate) {
            auto g = gate->get();
            for(int v = 0; v < std::min(int(g.size()), int(slots.size())); ++v) if(slots[v] >= 0) g[v] = 0;
            if(g != gate->get()) gate->set(g);
        }
        notes.clear(); slots.clear();
    }
private:
    static std::shared_ptr<Notes> copy(ofxOceanodeAbstractParameter* p) {
        return p ? std::make_shared<Notes>(p->cast<std::vector<float>>().getParameter()) : nullptr;
    }
    void closeNote(int note) {
        if(!gate) return;
        auto g = gate->get();
        for(int v = 0; v < std::min(int(g.size()), int(slots.size())); ++v) if(slots[v] == note) g[v] = 0;
        if(g != gate->get()) gate->set(g);
    }
    void apply(const Instrument& instrument) {
        auto next = notes.allocate(instrument.voices, slots, instrument.kit);
        auto g = gate->get();
        g.resize(std::max(g.size(), std::max(slots.size(), next.size())), 0);
        for(size_t v = 0; v < std::max(slots.size(), next.size()); ++v)
            if(v >= slots.size() || v >= next.size() || slots[v] != next[v]) g[v] = 0;
        if(g != gate->get()) gate->set(g);
        if(pitch) {
            auto p = pitch->get(); p.resize(std::max(p.size(), next.size()), 60);
            for(size_t v = 0; v < next.size(); ++v) if(next[v] >= 0) p[v] = float(next[v]);
            if(p != pitch->get()) pitch->set(p);
        }
        if(velocity) {
            auto vel = velocity->get(); vel.resize(std::max(vel.size(), next.size()), 0);
            for(size_t v = 0; v < next.size(); ++v) if(next[v] >= 0) vel[v] = notes.velocity(next[v]);
            if(vel != velocity->get()) velocity->set(vel);
        }
        for(size_t v = 0; v < next.size(); ++v) g[v] = next[v] < 0 ? 0 : (velocity ? 1 : notes.velocity(next[v]));
        if(g != gate->get()) gate->set(g);
        slots = std::move(next);
    }
    Resolve resolve;
    ofxOceanodeMidiKeyboardNotes notes;
    std::vector<int> slots;
    std::shared_ptr<Notes> pitch, gate, velocity;
    ofxOceanodeAbstractParameter *pitchSource = nullptr, *gateSource = nullptr, *velocitySource = nullptr;
};
