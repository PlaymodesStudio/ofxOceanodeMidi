//
//  midiGateIn.cpp
//  MIRABCN_Generator
//
//  Created by Eduard Frigola on 14/07/2017.
//
//

#include "midiGateIn.h"

midiGateIn::midiGateIn() : ofxOceanodeNodeModel("Note In"){
    midiIn = nullptr;
}

void midiGateIn::setup(){
    midiIn = new ofxMidiIn();
    
    midiports = {"None"};
    midiports.resize(1+midiIn->getNumInPorts());
    for(int i = 0; i < midiIn->getNumInPorts(); i++){
        midiports[i+1] = midiIn->getInPortList()[i];
    }
    addParameterDropdown(midiDevice, "Device", 0, midiports, ofxOceanodeParameterFlags_DisableSavePreset);
    addParameter(midiChannel.set("Channel", 0, 0, 16));
    ofParameter<char> noteslabel("Notes", ' ');
    addParameter(noteslabel, ofxOceanodeParameterFlags_DisableSavePreset);
    addParameter(noteOnStart.set("Start", 0, 0, 127));
    addParameter(noteOnEnd.set("End", 127, 0, 127));
    addOutputParameter(output.set("Output", {0}, {0}, {1}));
    
    outputStore.resize(noteOnEnd - noteOnStart + 1, 0);
    listeners.push(noteOnStart.newListener(this, &midiGateIn::noteRangeChanged));
    listeners.push(noteOnEnd.newListener(this, &midiGateIn::noteRangeChanged));
    listeners.push(midiDevice.newListener(this, &midiGateIn::midiDeviceListener));
}

void midiGateIn::update(ofEventArgs &e){
    if(mutex.try_lock()){
        output = outputStore;
        activatedNotes.clear();
        for(auto &n : toShutNotes){
            outputStore[n] = 0;
        }
        toShutNotes.clear();
        mutex.unlock();
    }
}

void midiGateIn::newMidiMessage(ofxMidiMessage &eventArgs){
    if((eventArgs.status == MIDI_NOTE_ON && eventArgs.velocity != 0) && (eventArgs.channel == midiChannel || midiChannel == 0)){
        if(eventArgs.pitch >= noteOnStart && eventArgs.pitch <= noteOnEnd){
            {
                mutex.lock();
                outputStore[eventArgs.pitch - noteOnStart] = (float)eventArgs.velocity/(float)127;
                activatedNotes.push_back(eventArgs.pitch - noteOnStart);
                mutex.unlock();
            }
        }
    }else if((eventArgs.status == MIDI_NOTE_OFF || (eventArgs.status == MIDI_NOTE_ON && eventArgs.velocity == 0)) && (eventArgs.channel == midiChannel || midiChannel == 0)){
        if(eventArgs.pitch >= noteOnStart && eventArgs.pitch <= noteOnEnd){
            {
                mutex.lock();
                if(find(activatedNotes.begin(), activatedNotes.end(), eventArgs.pitch - noteOnStart) != activatedNotes.end()){
                    toShutNotes.push_back(eventArgs.pitch - noteOnStart);
                }else{
                    outputStore[eventArgs.pitch - noteOnStart] = 0;
                }
                mutex.unlock();
            }
        }
    }
}

void midiGateIn::midiDeviceListener(int &device){
    outputStore = vector<float>(noteOnEnd - noteOnStart + 1, 0);
    midiIn->closePort();
    if(device > 0){
        midiIn->openPort(device-1);
        midiIn->addListener(this);
    }
}

void midiGateIn::noteRangeChanged(int &note){
    if(noteOnEnd < noteOnStart){
        noteOnStart = 0;
    }else{
        outputStore.resize(noteOnEnd - noteOnStart + 1, 0);
    }
}

void midiGateIn::presetSave(ofJson &json){
    json["DeviceName"] = midiports[midiDevice];
}

void midiGateIn::presetRecallBeforeSettingParameters(ofJson &json){
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
