#ifndef midiControllerIn_h
#define midiControllerIn_h

#include "ofxOceanodeNodeModel.h"
#include "ofxMidi.h"

class controllerIn : public ofxOceanodeNodeModel, public ofxMidiListener {
public:
    controllerIn();
    ~controllerIn(){
        if(midiIn != nullptr){
            midiIn->closePort();
            midiIn->removeListener(this);
            delete midiIn;
        }
    }
    
    void setup() override;
    void update(ofEventArgs &e) override;
    void newMidiMessage(ofxMidiMessage& eventArgs) override;
    
    void presetSave(ofJson &json) override;
    void presetRecallBeforeSettingParameters(ofJson &json) override;
    
private:
    void midiDeviceListener(int &device);
    void controllerRangeChanged(int &note);
    
    ofxMidiIn* midiIn;
    vector<string> midiports;
    
    ofParameter<int> midiDevice;
    ofParameter<int> midiChannel;
    ofParameter<int> controllerStart;
    ofParameter<int> controllerEnd;
    ofParameter<vector<float>> output;
    
    vector<float> outputStore;
    std::mutex mutex;
    
    ofEventListeners listeners;
};

#endif /* midiControllerIn_h */
