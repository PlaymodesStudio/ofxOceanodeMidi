//
//  midiGateIn.h
//  MIRABCN_Generator
//
//  Created by Eduard Frigola on 14/07/2017.
//
//

#ifndef midiGateIn_h
#define midiGateIn_h

#include "ofxOceanodeNodeModel.h"
#include "ofxMidi.h"

class midiGateIn : public ofxOceanodeNodeModel, ofxMidiListener {
public:
    midiGateIn();
    ~midiGateIn(){
        if(midiIn != nullptr)
            delete midiIn;
    };
    
    void setup() override;
    void update(ofEventArgs &e) override;
    
    void presetSave(ofJson &json) override;
    void presetRecallBeforeSettingParameters(ofJson &json) override;
    
private:
    void newMidiMessage(ofxMidiMessage& eventArgs);
    void midiDeviceListener(int &device);
    void noteRangeChanged(int &note);
    
    ofEventListeners listeners;
    
    ofParameter<int>    midiDevice;
    ofParameter<int>    midiChannel;
    ofParameter<int>    noteOnStart;
    ofParameter<int>    noteOnEnd;
    
    ofParameter<vector<float>> output;
    
    ofxMidiIn*   midiIn;
    vector<float>   outputStore;
    vector<int> activatedNotes;
    vector<int> toShutNotes;
    ofMutex mutex;
    vector<string> midiports;
};

#endif /* midiGateIn_h */
