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

// NoteGate class
class noteGate : public ofxOceanodeNodeModel{
public:
    noteGate() : ofxOceanodeNodeModel("Note Gate") {}

    void setup() {
        addParameterDropdown(midiPort, "Port", 0, midiOut.getOutPortList());
        addParameter(midiChannel.set("Ch", 1, 1, 16));
        addParameter(noteGroup.set("Note", {0}, {-FLT_MAX}, {FLT_MAX}));
        addParameter(gateGroup.set("Gate", {0}, {0}, {1}));

        listeners.push(midiPort.newListener([this](int &device){
            midiOut.openPort(device);
        }));

        listeners.push(noteGroup.newListener([this](vector<float> &vf){
            vector<float> gate = gateGroup.get();

            if(vf.size() != noteGroupStore.size() || gate.size() != gateGroupStore.size()){
                for(auto it = activeNotes.begin(); it != activeNotes.end();){
                    midiOut.sendNoteOff(midiChannel, *it);
                    it = activeNotes.erase(it);
                }
                noteGroupStore.resize(vf.size(), 0.0);
                gateGroupStore.resize(gate.size(), 0.0);
            }

            for(int i = 0; i < vf.size(); i++){
                if(vf[i] != noteGroupStore[i] || gate[i] != gateGroupStore[i]){
                    if(gate[i] != 0){
                        midiOut.sendNoteOn(midiChannel, static_cast<int>(vf[i]), gate[i]*127);
                        activeNotes.insert(static_cast<int>(vf[i]));
                    }else{
                        midiOut.sendNoteOff(midiChannel, static_cast<int>(vf[i]));
                        activeNotes.erase(static_cast<int>(vf[i]));
                    }
                }
            }

            for(auto it = activeNotes.begin(); it != activeNotes.end();){
                if(std::find(vf.begin(), vf.end(), *it) == vf.end()){
                    midiOut.sendNoteOff(midiChannel, *it);
                    it = activeNotes.erase(it);
                }else{
                    ++it;
                }
            }

            noteGroupStore = vf;
            gateGroupStore = gate;
        }));
    }

private:
    ofParameter<int> midiPort;
    ofParameter<int> midiChannel;
    ofParameter<vector<float>> noteGroup;
    ofParameter<vector<float>> gateGroup;

    ofEventListeners listeners;

    ofxMidiOut midiOut;
    vector<float> noteGroupStore;
    vector<float> gateGroupStore;
    std::set<int> activeNotes;
};

#endif /* midiOut_h */
