//Created by Eduard Frigola
//Modified by Santi Vilanova


#ifndef midiOut_h
#define midiOut_h

#include "ofxOceanodeNodeModel.h"
#include "ofxMidi.h"

// NoteOut class
class noteOut : public ofxOceanodeNodeModel{
public:
    noteOut() : ofxOceanodeNodeModel("Note Out") {};

    void setup() {
        addParameterDropdown(midiPort, "Port", 0, midiOut.getOutPortList());
        addParameter(midiChannel.set("Ch", 1, 1, 16));
        addParameter(notes.set("Notes", {0}, {0}, {1}));

        listeners.push(midiPort.newListener([this](int &device){
            midiOut.openPort(device);
        }));

        listeners.push(notes.newListener([this](vector<float> &vf){
            if(vf.size() != notesStore.size()){
                notesStore.resize(vf.size(), 0.0);
            }
            for(int i = 0; i < vf.size(); i++){
                if(vf[i] != notesStore[i]){
                    if(vf[i] != 0){
                        if(notesStore[i] != 0){
                            midiOut.sendNoteOff(midiChannel, i);
                        }
                        midiOut.sendNoteOn(midiChannel, i, vf[i]*127);
                    }else{
                        midiOut.sendNoteOff(midiChannel, i);
                    }
                }
            }
            notesStore = vf;
        }));
    }
private:
    ofParameter<int> midiPort;
    ofParameter<int> midiChannel;
    ofParameter<vector<float>> notes;

    ofEventListeners listeners;

    ofxMidiOut midiOut;
    vector<float> notesStore;
};

// CtlOut class
class ctlOut : public ofxOceanodeNodeModel{
public:
    ctlOut() : ofxOceanodeNodeModel("Control Out") {}

    void setup() {
        addParameterDropdown(midiPort, "Port", 0, midiOut.getOutPortList());
        addParameter(midiChannel.set("Ch", 1, 1, 16));
        addParameter(cc.set("CC", {0}, {0}, {1}));

        listeners.push(midiPort.newListener([this](int &device){
            midiOut.openPort(device);
        }));

        listeners.push(cc.newListener([this](vector<float> &vf){
            if(vf.size() != ccStore.size()){
                ccStore.resize(vf.size(), 0.0);
            }
            for(int i = 0; i < vf.size(); i++){
                if(vf[i] != ccStore[i]){
                    midiOut.sendControlChange(midiChannel, i, vf[i]*127);
                }
            }
            ccStore = vf;
        }));
    }

private:
    ofParameter<int> midiPort;
    ofParameter<int> midiChannel;
    ofParameter<vector<float>> cc;

    ofEventListeners listeners;

    ofxMidiOut midiOut;
    vector<float> ccStore;
};

class controlChange : public ofxOceanodeNodeModel {
public:
    controlChange() : ofxOceanodeNodeModel("Control Change") {}

    void setup() {
        addParameterDropdown(midiPort, "Port", 0, midiOut.getOutPortList());
        addParameter(midiChannel.set("Ch", {1}, {1}, {16}));
        addParameter(ccNum.set("CC num", {0}, {0}, {127}));
        addParameter(ccVal.set("CC val", {0}, {0}, {127}));

        listeners.push(midiPort.newListener([this](int &device){
            midiOut.openPort(device);
        }));

        auto sendCC = [this]() {
            vector<int> ccNumbers = ccNum.get();
            vector<float> values = ccVal.get();
            vector<int> channels = midiChannel.get();

            // Ensure the sizes are consistent
            int minSize = std::min(std::min(ccNumbers.size(), values.size()), channels.size());

            for(int i = 0; i < minSize; i++){
                midiOut.sendControlChange(channels[i], ccNumbers[i], static_cast<int>(values[i]));
            }
        };

        listeners.push(ccNum.newListener([sendCC](vector<int> &){ sendCC(); }));
        listeners.push(ccVal.newListener([sendCC](vector<float> &){ sendCC(); }));
    }

private:
    ofParameter<int> midiPort;
    ofParameter<vector<int>> midiChannel;
    ofParameter<vector<int>> ccNum;
    ofParameter<vector<float>> ccVal;

    ofEventListeners listeners;

    ofxMidiOut midiOut;
};


class noteGate : public ofxOceanodeNodeModel {
public:
    noteGate() : ofxOceanodeNodeModel("Note Gate") {}

    void setup() {
        addParameterDropdown(midiPort, "Port", 0, midiOut.getOutPortList());
        addParameter(midiChannel.set("Ch", {1}, {1}, {16}));
        addParameter(noteGroup.set("Note", {0}, {-FLT_MAX}, {FLT_MAX}));
        addParameter(gateGroup.set("Gate", {0}, {0}, {1}));

        listeners.push(midiPort.newListener([this](int &device){
            midiOut.openPort(device);
        }));

        listeners.push(noteGroup.newListener([this](vector<float> &vf){
            vector<float> gate = gateGroup.get();
            vector<int> channels = midiChannel.get();

            if(vf.size() != noteGroupStore.size() || gate.size() != gateGroupStore.size()){
                for(auto it = activeNotes.begin(); it != activeNotes.end();){
                    midiOut.sendNoteOff(it->second, it->first);
                    it = activeNotes.erase(it);
                }
                noteGroupStore.resize(vf.size(), 0.0);
                gateGroupStore.resize(gate.size(), 0.0);
                channelsStore.resize(channels.size(), 0);
            }

            for(int i = 0; i < vf.size(); i++){
                if(i < gate.size() && i < noteGroupStore.size() && i < gateGroupStore.size() && i < channels.size()){
                    bool noteChanged = vf[i] != noteGroupStore[i];
                    bool gateChanged = gate[i] != gateGroupStore[i] && gate[i] != 0;

                    if(gateChanged || noteChanged){
                        if(activeNotes.find(static_cast<int>(noteGroupStore[i])) != activeNotes.end()) {
                            midiOut.sendNoteOff(channels[i], static_cast<int>(noteGroupStore[i]));
                        }
                        if(gate[i] != 0){
                            midiOut.sendNoteOn(channels[i], static_cast<int>(vf[i]), gate[i]*127);
                            activeNotes[static_cast<int>(vf[i])] = channels[i];
                        }
                    }
                }
            }

            for(auto it = activeNotes.begin(); it != activeNotes.end();){
                if(std::find(vf.begin(), vf.end(), it->first) == vf.end()){
                    midiOut.sendNoteOff(it->second, it->first);
                    it = activeNotes.erase(it);
                }else{
                    ++it;
                }
            }

            noteGroupStore = vf;
            gateGroupStore = gate;
            channelsStore = channels;
        }));
    }

private:
    ofParameter<int> midiPort;
    ofParameter<vector<int>> midiChannel;
    ofParameter<vector<float>> noteGroup;
    ofParameter<vector<float>> gateGroup;

    ofEventListeners listeners;

    ofxMidiOut midiOut;
    vector<float> noteGroupStore;
    vector<float> gateGroupStore;
    vector<int> channelsStore;
    std::map<int, int> activeNotes;
};

class programChange : public ofxOceanodeNodeModel {
public:
    programChange() : ofxOceanodeNodeModel("Program Change") {}

    void setup() {
        addParameterDropdown(midiPort, "Port", 0, midiOut.getOutPortList());
        addParameter(midiChannel.set("Ch", {1}, {1}, {16}));
        addParameter(prgNum.set("Prg Num", {0}, {0}, {127}));

        listeners.push(midiPort.newListener([this](int &device){
            midiOut.openPort(device);
        }));

        listeners.push(prgNum.newListener([this](vector<int> &pn){
            vector<int> channels = midiChannel.get();

            // Ensure the sizes are consistent
            int minSize = std::min(pn.size(), channels.size());

            for(int i = 0; i < minSize; i++){
                midiOut.sendProgramChange(channels[i], pn[i]);
            }
        }));
    }

private:
    ofParameter<int> midiPort;
    ofParameter<vector<int>> midiChannel;
    ofParameter<vector<int>> prgNum;

    ofEventListeners listeners;

    ofxMidiOut midiOut;
};

#endif /* midiOut_h */
