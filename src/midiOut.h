//  midiOut.h
//  example
//
//  Created by Eduard Frigola Bagué on 22/02/2021.
//

#ifndef midiOut_h
#define midiOut_h

#include "ofxOceanodeNodeModel.h"
#include "ofxMidi.h"

class noteOut : public ofxOceanodeNodeModel{
public:
    noteOut() : ofxOceanodeNodeModel("Note Out"){};
    
    void setup(){
        addParameterDropdown(midiPort, "Port", 0, midiOut.getOutPortList());
        addParameter(midiChannel.set("Ch", 1, 1, 16));
        addParameter(notes.set("Notes", {0}, {0}, {1}));
        addParameter(cc.set("CC", {0}, {0}, {1}));
        
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
    ofParameter<vector<float>> notes;
    ofParameter<vector<float>> cc;
    
    ofEventListeners listeners;
    
    ofxMidiOut midiOut;
    vector<float> notesStore;
    vector<float> ccStore;
};

#endif /* midiOut_h */
