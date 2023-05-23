//
//  ofxOceanodeMidiController.h
//  example-midi
//
//  Created by Eduard Frigola Bagué on 19/08/2018.
//

#ifndef ofxOceanodeMidiController_h
#define ofxOceanodeMidiController_h

#include "ofxOceanodeBaseController.h"
#include "ofxOceanodePresetsController.h"
#include "ofxMidi.h"
#include "ofParameter.h"

class ofxOceanodeAbstractMidiBinding;


class ofxOceanodePresetsMidiControl : public ofxMidiListener{
public:
    ofxOceanodePresetsMidiControl(){
        channel.set("Channel", 0, 0, 16);
        midiDevice == "-";
        type == "--";
    }
    ~ofxOceanodePresetsMidiControl(){};
    
    void newMidiMessage(ofxMidiMessage &message){
        if(message.portName == midiDevice && (message.channel == channel || channel == 0)){
            if(message.status == MIDI_NOTE_ON && type == "Note On"){
                if(message.velocity!=0) presetsController->loadPresetFromNumber(message.pitch);
            }else if(message.status == MIDI_CONTROL_CHANGE && type == "Control Change"){
                
            }else if(message.status == MIDI_PROGRAM_CHANGE && type == "Program Change"){
                
            }
        }
    }
    
    ofParameter<int> &getChannel(){return channel;};
    
    void setMidiDevice(std::string device){midiDevice = device;};
    void setType(std::string _type){type = _type;};
    void setPresetsController(std::shared_ptr<ofxOceanodePresetsController> _presetsController){presetsController = _presetsController;};
    
private:
    ofParameter<int> channel;
    std::string type;
    std::string midiDevice;
    std::shared_ptr<ofxOceanodePresetsController> presetsController;
};

class ofxOceanodeMidiController : public ofxOceanodeBaseController{
public:
    ofxOceanodeMidiController(std::shared_ptr<ofxOceanodePresetsController> presetsController, std::shared_ptr<ofxOceanodeContainer> _container);
    ~ofxOceanodeMidiController(){};
    
//    void midiLearnPressed(ofxDatGuiToggleEvent e);
//
    void createGuiForBinding(std::shared_ptr<ofxOceanodeAbstractMidiBinding> midiBinding);
    void removeParameterBinding(ofxOceanodeAbstractMidiBinding &midiBinding);
//
//    void onGuiDropdownEvent(ofxDatGuiDropdownEvent e);
//    void onGuiTextInputEvent(ofxDatGuiTextInputEvent e);
//
//    bool getIsMidiLearn(){return midiLearn->getChecked();};
//    void setIsMidiLearn(bool b);
    void draw();
    
private:
    bool midiLearn;
    ofEventListeners listeners;
    std::vector<std::string> midiDevices;
    
    ofxOceanodePresetsMidiControl presetsControl;
    std::shared_ptr<ofxOceanodeContainer> container;
};

#endif /* ofxOceanodeMidiController_h */
