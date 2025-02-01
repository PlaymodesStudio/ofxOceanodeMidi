#include "midiControllerIn.h"

controllerIn::controllerIn() : ofxOceanodeNodeModel("Controller In"){
    midiIn = nullptr;
}

void controllerIn::setup(){
    midiIn = new ofxMidiIn();
    
    // Setup available MIDI ports
    midiports = {"None"};
    midiports.resize(1 + midiIn->getNumInPorts());
    for(int i = 0; i < midiIn->getNumInPorts(); i++){
        midiports[i+1] = midiIn->getInPortList()[i];
    }
    
    // Add parameters
    addParameterDropdown(midiDevice, "Device", 0, midiports, ofxOceanodeParameterFlags_DisableSavePreset);
    addParameter(midiChannel.set("Channel", 0, 0, 16));
    ofParameter<char> cclabel("Controllers", ' ');
    addParameter(cclabel, ofxOceanodeParameterFlags_DisableSavePreset);
    addParameter(controllerStart.set("Start CC", 0, 0, 127));
    addParameter(controllerEnd.set("End CC", 127, 0, 127));
    addOutputParameter(output.set("Output", {0}, {0}, {1}));
    
    // Initialize output store
    outputStore.resize(controllerEnd - controllerStart + 1, 0);
    
    // Add listeners
    listeners.push(controllerStart.newListener(this, &controllerIn::controllerRangeChanged));
    listeners.push(controllerEnd.newListener(this, &controllerIn::controllerRangeChanged));
    listeners.push(midiDevice.newListener(this, &controllerIn::midiDeviceListener));
}

void controllerIn::update(ofEventArgs &e){
    if(mutex.try_lock()){
        output = outputStore;
        mutex.unlock();
    }
}

void controllerIn::newMidiMessage(ofxMidiMessage &eventArgs){
    // Handle Control Change messages
    if(eventArgs.status == MIDI_CONTROL_CHANGE && (eventArgs.channel == midiChannel || midiChannel == 0)){
        if(eventArgs.control >= controllerStart && eventArgs.control <= controllerEnd){
            mutex.lock();
            outputStore[eventArgs.control - controllerStart] = (float)eventArgs.value/127.0f;
            mutex.unlock();
        }
    }
}

void controllerIn::midiDeviceListener(int &device){
    outputStore = vector<float>(controllerEnd - controllerStart + 1, 0);
    midiIn->closePort();
    if(device > 0){
        midiIn->openPort(device-1);
        midiIn->addListener(this);
    }
}

void controllerIn::controllerRangeChanged(int &controller){
    if(controllerEnd < controllerStart){
        controllerStart = 0;
    }else{
        outputStore.resize(controllerEnd - controllerStart + 1, 0);
    }
}

void controllerIn::presetSave(ofJson &json){
    json["DeviceName"] = midiports[midiDevice];
}

void controllerIn::presetRecallBeforeSettingParameters(ofJson &json){
    string name = "None";
    if(json.count("DeviceName") == 1){
        name = json["DeviceName"];
    }
    auto it = std::find(midiports.begin(), midiports.end(), name);
    
    if(it != midiports.end()){
        midiDevice = it - midiports.begin();
    }else{
        midiDevice = 0;
    }
}
