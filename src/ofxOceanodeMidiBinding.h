//
//  ofxOceanodeMidiBinding.h
//  example-midi
//
//  Created by Eduard Frigola Bagué on 20/08/2018.
//

#ifndef ofxOceanodeMidiBinding_h
#define ofxOceanodeMidiBinding_h

#include "ofParameter.h"
#include "ofJson.h"
#include "ofxMidi.h"
#include "ofTypes.h"

using namespace std;

class ofxOceanodeAbstractMidiBinding : public ofxMidiListener{
public:
    ofxOceanodeAbstractMidiBinding(){
        isListening = true;
        messageType.set("Message Type", "Not Assigned");
        channel.set("Channel", -1, 1, 16);
        control.set("Control", -1, 0, 127);
        lsb.set("LSB", -1, 0, 127);
        value.set("Value", -1, 0, 127);
        portName = "";
        modifiyingParameter = false;
    };
    ~ofxOceanodeAbstractMidiBinding(){};
    
    void newMidiMessage(ofxMidiMessage& message) {
        if(isListening){
            portName = message.portName;
            channel = message.channel;
            status = message.status;
            messageType = ofxMidiMessage::getStatusString(status);
            if(message.status == MIDI_CONTROL_CHANGE){
                control = message.control;
            }
            if(message.status == MIDI_NOTE_ON || message.status == MIDI_NOTE_OFF){
                control = message.pitch;
            }
            isListening = false;
            unregisterUnusedMidiIns.notify(this, portName);
        }
        else{
            if(mutex.try_lock()){
                lastsMidiMessage.push_back(message);
                mutex.unlock();
            }
        }
    }
    
    virtual void savePreset(ofJson &json){
        json["Port"] = portName;
        json["Channel"] = channel.get();
        json["Control"] = control.get();
        json["Type"] = messageType.get();
        json["LSB"] = lsb.get();
    }
    
    virtual void loadPreset(ofJson &json){
        if(json.count("Port") == 1){
            portName = json["Port"];
            isListening = false;
            unregisterUnusedMidiIns.notify(this, portName);
        }
        
        channel.set(json["Channel"]);
        control.set(json["Control"]);
        messageType.set(json["Type"]);
        if(messageType.get() == ofxMidiMessage::getStatusString(MIDI_CONTROL_CHANGE)) status = MIDI_CONTROL_CHANGE;
        else if(messageType.get() == ofxMidiMessage::getStatusString(MIDI_NOTE_ON)) status = MIDI_NOTE_ON;
        else ofLog() << "Type Error";
        
        if(json.count("LSB") == 1){
            lsb = json["LSB"];
        }
    }
    
    string type() const{
        return typeid(*this).name();
    }
    
    virtual void update(){}
    
    string getName(){return name;};
    
    ofParameter<string> &getMessageType(){return messageType;};
    ofParameter<int> &getChannel(){return channel;};
    ofParameter<int> &getControl(){return control;};
    ofParameter<int> &getValue(){return value;};
    ofParameter<int> &getLSB(){return lsb;};
    //TODO: ADD Midi device?
    
    string getPortName(){return portName;};
    
//    virtual ofxMidiMessage& sendMidiMessage() = 0;//{ofxMidiMessage m; return m;};
    virtual void bindParameter(){};
    
    ofEvent<string> unregisterUnusedMidiIns;
    ofEvent<ofxMidiMessage> midiMessageSender;
    
    ofMutex mutex;
    vector<ofxMidiMessage> lastsMidiMessage;
protected:
    bool isListening;
    
    string portName;
    ofParameter<string> messageType;
    MidiStatus status;
    ofParameter<int> channel;
    ofParameter<int> control;
    ofParameter<int> value;
    ofParameter<int> lsb;
    
    string name;
    ofxMidiMessage firstMidiMessage;
    
    bool modifiyingParameter;
    ofEventListener listener;
};


template<typename T, typename Enable = void>
class ofxOceanodeMidiBinding : public ofxOceanodeAbstractMidiBinding{
public:
    ofxOceanodeMidiBinding(ofParameter<T>& _parameter, int _id) : parameter(_parameter), ofxOceanodeAbstractMidiBinding(){
        name = parameter.getGroupHierarchyNames()[0] + "-|-" + parameter.getEscapedName() + " " + ofToString(_id);
    }
    
    ~ofxOceanodeMidiBinding(){};
    
    //    void newMidiMessage(ofxMidiMessage& message){};
    void bindParameter(){};
    void update(){}
    
private:
    ofParameter<T>& parameter;
};

template<typename T>
class ofxOceanodeMidiBinding<T, typename std::enable_if<std::is_same<T, int>::value || std::is_same<T, float>::value>::type>: public ofxOceanodeAbstractMidiBinding{
public:
    ofxOceanodeMidiBinding(ofParameter<T>& _parameter, int _id) : parameter(_parameter), ofxOceanodeAbstractMidiBinding(){
        name = parameter.getGroupHierarchyNames()[0] + "-|-" + parameter.getEscapedName() + " " + ofToString(_id);
        min.set("Min", parameter.getMin(), parameter.getMin(), parameter.getMax());
        max.set("Max", parameter.getMax(), parameter.getMin(), parameter.getMax());
    }
    
    ~ofxOceanodeMidiBinding(){};
    
    void savePreset(ofJson &json){
        ofxOceanodeAbstractMidiBinding::savePreset(json);
        json["Min"] = min.get();
        json["Max"] = max.get();
    }
    
    void loadPreset(ofJson &json){
        ofxOceanodeAbstractMidiBinding::loadPreset(json);
        min.set(json["Min"]);
        max.set(json["Max"]);
    }
    
    void update(){
        mutex.lock();
        bool validMessage = false;
        int msb_val = -1;
        int lsb_val = -1;
        bool is14bit = lsb != -1;
        for(int i = lastsMidiMessage.size()-1; i >= 0 && !validMessage ; i--){
            ofxMidiMessage &message = lastsMidiMessage[i];
            if(message.status == MIDI_NOTE_OFF){
                message.status = MIDI_NOTE_ON;
                message.velocity = 0;
            }
            if(isListening) ofxOceanodeAbstractMidiBinding::newMidiMessage(message);
            if(message.channel == channel && message.status == status){
                switch(status){
                    case MIDI_CONTROL_CHANGE:
                    {
                        if(message.control == control){
                            if(!is14bit){
                                value = message.value;
                                validMessage = true;
                            }
                            else{
                                msb_val = message.value;
                                if(lsb_val != -1){
                                    value = (msb_val * 128) + lsb_val;
                                    validMessage = true;
                                }
                            }
                        }else if(message.control == lsb){
                            lsb_val = message.value;
                            if(msb_val != -1){
                                value = (msb_val * 128) + lsb_val;
                                validMessage = true;
                            }
                        }
                        break;
                    }
                    case MIDI_NOTE_ON:
                    {
                        if(message.pitch == control){
                            value = message.velocity;
                            validMessage = true;
                            
                        }
                        break;
                    }
                    default:
                    {
                        ofLog() << "Midi Type " << ofxMidiMessage::getStatusString(message.status) << " not supported for parameter of type " << typeid(T).name();
                    }
                }
                if(validMessage){
                    modifiyingParameter = true;
                    parameter.set(ofMap(value, 0, is14bit ? 16383 : 127, min, max, true));
                    modifiyingParameter = false;
                }
            }
        }
        lastsMidiMessage.clear();
        mutex.unlock();
    };
    void bindParameter(){
        listener = parameter.newListener([this](T &f){
            bool is14bit = lsb != -1;
            if(!modifiyingParameter){
                if(is14bit){
                    value = ofMap(f, min, max, 0, 16383, true);
                    value = value >> 7 & 0x007f;
                }else{
                    value = ofMap(f, min, max, 0, 127, true);
                }
                ofxMidiMessage message;
                message.status = status;
                message.channel = channel;
                switch(status){
                    case MIDI_CONTROL_CHANGE:
                        message.control = control;
                        message.value = value;
                        break;
                    case MIDI_NOTE_ON:
                        message.pitch = control;
                        message.velocity = value;
                        break;
                    default:
                        ofLog() << "Midi Type " << ofxMidiMessage::getStatusString(message.status) << " not supported for parameter of type " << typeid(T).name();
                }
                midiMessageSender.notify(this, message);
                if(is14bit){
                    value = ofMap(f, min, max, 0, 16383, true);
                    value = value & 0x007f;
                    ofxMidiMessage message;
                    message.status = status;
                    message.channel = channel;
                    message.control = lsb;
                    message.value = value;
                    midiMessageSender.notify(this, message);
                }
            }
        });
    };
    
    ofParameter<T> &getMinParameter(){return min;};
    ofParameter<T> &getMaxParameter(){return max;};
    
private:
    ofParameter<T>& parameter;
    ofParameter<T> min;
    ofParameter<T> max;
    
};

template<typename>
struct is_std_vector2 : std::false_type {};

template<typename T, typename A>
struct is_std_vector2<std::vector<T,A>> : std::true_type {};

template<typename T>
class ofxOceanodeMidiBinding<vector<T>> : public ofxOceanodeAbstractMidiBinding{
public:
    ofxOceanodeMidiBinding(ofParameter<vector<T>>& _parameter, int _id) : parameter(_parameter), ofxOceanodeAbstractMidiBinding(){
        name = parameter.getGroupHierarchyNames()[0] + "-|-" + parameter.getEscapedName() + " " + ofToString(_id);
        min.set("Min", parameter.getMin()[0], parameter.getMin()[0], parameter.getMax()[0]);
        max.set("Max", parameter.getMax()[0], parameter.getMin()[0], parameter.getMax()[0]);
    }
    
    ~ofxOceanodeMidiBinding(){};
    
    void savePreset(ofJson &json){
        ofxOceanodeAbstractMidiBinding::savePreset(json);
        json["Min"] = min.get();
        json["Max"] = max.get();
    }
    
    void loadPreset(ofJson &json){
        ofxOceanodeAbstractMidiBinding::loadPreset(json);
        min.set(json["Min"]);
        max.set(json["Max"]);
    }
    
    void update(){
        mutex.lock();
        bool validMessage = false;
        int msb_val = -1;
        int lsb_val = -1;
        bool is14bit = lsb != -1;
        for(int i = lastsMidiMessage.size()-1; i >= 0 && !validMessage ; i--){
            ofxMidiMessage &message = lastsMidiMessage[i];
            
            if(message.status == MIDI_NOTE_OFF){
                message.status = MIDI_NOTE_ON;
                message.velocity = 0;
            }
            if(isListening) ofxOceanodeAbstractMidiBinding::newMidiMessage(message);
            if(message.channel == channel && message.status == status){
                switch(status){
                    case MIDI_CONTROL_CHANGE:
                    {
                        if(message.control == control){
                            if(!is14bit){
                                value = message.value;
                                validMessage = true;
                            }
                            else{
                                msb_val = message.value;
                                if(lsb_val != -1){
                                    value = (msb_val * 128) + lsb_val;
                                    validMessage = true;
                                }
                            }
                        }else if(message.control == lsb){
                            lsb_val = message.value;
                            if(msb_val != -1){
                                value = (msb_val * 128) + lsb_val;
                                validMessage = true;
                            }
                        }
                        break;
                    }
                    case MIDI_NOTE_ON:
                    {
                        if(message.pitch == control){
                            value = message.velocity;
                            validMessage = true;
                        }
                        break;
                    }
                    default:
                    {
                        ofLog() << "Midi Type " << ofxMidiMessage::getStatusString(message.status) << " not supported for parameter of type " << typeid(vector<T>).name();
                    }
                }
                if(validMessage){
                    modifiyingParameter = true;
                    parameter.set(vector<T>(1, ofMap(value, 0, is14bit ? 16383 : 127, min, max, true)));
                    modifiyingParameter = false;
                }
            }
        }
        lastsMidiMessage.clear();
        mutex.unlock();
    };
    
    void bindParameter(){
        listener = parameter.newListener([this](vector<T> &vf){
            bool is14bit = lsb != -1;
            if(!modifiyingParameter && vf.size() == 1){
                if(is14bit){
                    value = ofMap(vf[0], min, max, 0, 16383, true);
                    value = value >> 7 & 0x007f;
                }else{
                    value = ofMap(vf[0], min, max, 0, 127, true);
                }
                ofxMidiMessage message;
                message.status = status;
                message.channel = channel;
                switch(status){
                    case MIDI_CONTROL_CHANGE:
                        message.control = control;
                        message.value = value;
                        break;
                    case MIDI_NOTE_ON:
                        message.pitch = control;
                        message.velocity = value;
                        break;
                    default:
                        ofLog() << "Midi Type " << ofxMidiMessage::getStatusString(message.status) << " not supported for parameter of type " << typeid(vector<T>).name();
                }
                midiMessageSender.notify(this, message);
                if(is14bit){
                    value = ofMap(vf[0], min, max, 0, 16383, true);
                    value = value & 0x007f;
                    ofxMidiMessage message;
                    message.status = status;
                    message.channel = channel;
                    message.control = lsb;
                    message.value = value;
                    midiMessageSender.notify(this, message);
                }
            }
        });
    };
    
    ofParameter<T> &getMinParameter(){return min;};
    ofParameter<T> &getMaxParameter(){return max;};
    
private:
    ofParameter<vector<T>>& parameter;
    ofParameter<T> min;
    ofParameter<T> max;
};

template<>
class ofxOceanodeMidiBinding<bool>: public ofxOceanodeAbstractMidiBinding{
public:
    ofxOceanodeMidiBinding(ofParameter<bool>& _parameter, int _id) : parameter(_parameter), ofxOceanodeAbstractMidiBinding(){
        name = parameter.getGroupHierarchyNames()[0] + "-|-" + parameter.getEscapedName() + " " + ofToString(_id);
        toggle.set("Toggle", false);
        filterOff.set("FilterOff", false);
    }
    
    ~ofxOceanodeMidiBinding(){};
    
    void savePreset(ofJson &json){
        ofxOceanodeAbstractMidiBinding::savePreset(json);
        json["Toggle"] = toggle.get();
        json["FilterOff"] = filterOff.get();
    }
    
    void loadPreset(ofJson &json){
        ofxOceanodeAbstractMidiBinding::loadPreset(json);
        toggle.set(json["Toggle"]);
        if(json.count("FilterOff") == 1){
            filterOff.set(json["FilterOff"]);
        }else{
            filterOff = false;
        }
    }
    
    void update(){
        mutex.lock();
        bool validMessage = false;
        for(int i = lastsMidiMessage.size()-1; i >= 0 && !validMessage ; i--){
            ofxMidiMessage &message = lastsMidiMessage[i];
            
            if(message.status == MIDI_NOTE_OFF){
                message.status = MIDI_NOTE_ON;
                message.velocity = 0;
            }
            if(isListening) ofxOceanodeAbstractMidiBinding::newMidiMessage(message);
            if(message.channel == channel && message.status == status){
                switch(status){
                    case MIDI_CONTROL_CHANGE:
                    {
                        if(message.control == control){
                            value = message.value;
                            validMessage = true;
                        }
                        break;
                    }
                    case MIDI_NOTE_ON:
                    {
                        if(message.pitch == control){
                            value = message.velocity;
                            validMessage = true;
                        }
                        break;
                    }
                    default:
                    {
                        ofLog() << "Midi Type " << ofxMidiMessage::getStatusString(message.status) << " not supported for parameter of type Bool";
                    }
                }
                if(validMessage){
                    modifiyingParameter = true;
                    if(toggle){
                        if(value > 63){
                            parameter.set(!parameter.get());
                        }
                    }else{
                        if(!(value == 0 && filterOff)){
                            parameter.set(value > 63 ? true : false);
                        }
                    }
                    modifiyingParameter = false;
                }
            }
        }
        lastsMidiMessage.clear();
        mutex.unlock();
    };
    void bindParameter(){
        listener = parameter.newListener([this](bool &f){
            if(!modifiyingParameter){
                value = f ? 127 : 0;
                ofxMidiMessage message;
                message.status = status;
                message.channel = channel;
                switch(status){
                    case MIDI_CONTROL_CHANGE:
                        message.control = control;
                        message.value = value;
                        break;
                    case MIDI_NOTE_ON:
                        message.pitch = control;
                        message.velocity = value;
                        break;
                    default:
                        ofLog() << "Midi Type " << ofxMidiMessage::getStatusString(message.status) << " not supported for parameter of type Bool";
                }
                midiMessageSender.notify(this, message);
//                if(value == 0 && status == MIDI_NOTE_ON){
//                    message.status = MIDI_NOTE_OFF;
//                    midiMessageSender.notify(this, message);
//                }
            }
        });
    };
    
    ofParameter<bool> &getToggleParameter(){return toggle;};
    ofParameter<bool> &getFilterOffParameter(){return filterOff;};
    
private:
    ofParameter<bool>& parameter;
    ofParameter<bool> toggle;
    ofParameter<bool> filterOff;
};

template<>
class ofxOceanodeMidiBinding<void>: public ofxOceanodeAbstractMidiBinding{
public:
    ofxOceanodeMidiBinding(ofParameter<void>& _parameter, int _id) : parameter(_parameter), ofxOceanodeAbstractMidiBinding(){
        name = parameter.getGroupHierarchyNames()[0] + "-|-" + parameter.getEscapedName() + " " + ofToString(_id);
        mode.set("Filter!=0", false);
    }
    
    ~ofxOceanodeMidiBinding(){};
    
    void savePreset(ofJson &json){
        ofxOceanodeAbstractMidiBinding::savePreset(json);
        json["Mode"] = mode.get();
    }
    
    void loadPreset(ofJson &json){
        ofxOceanodeAbstractMidiBinding::loadPreset(json);
        mode.set(json["Mode"]);
    }
    
    void update(){
        mutex.lock();
        bool validMessage = false;
        for(int i = lastsMidiMessage.size()-1; i >= 0 && !validMessage ; i--){
            ofxMidiMessage &message = lastsMidiMessage[i];
            
            if(message.status == MIDI_NOTE_OFF){
                message.status = MIDI_NOTE_ON;
                message.velocity = 0;
            }
            if(isListening) ofxOceanodeAbstractMidiBinding::newMidiMessage(message);
            if(message.channel == channel && message.status == status){
                switch(status){
                    case MIDI_CONTROL_CHANGE:
                    {
                        if(message.control == control){
                            value = message.value;
                            validMessage = true;
                        }
                        break;
                    }
                    case MIDI_NOTE_ON:
                    {
                        if(message.pitch == control){
                            value = message.velocity;
                            validMessage = true;
                        }
                        break;
                    }
                    default:
                    {
                        ofLog() << "Midi Type " << ofxMidiMessage::getStatusString(message.status) << " not supported for parameter of type Void";
                    }
                }
                if(validMessage){
                    modifiyingParameter = true;
                    if(mode){
                        if(value > 63){
                            parameter.trigger();
                        }
                    }else{
                        parameter.trigger();
                    }
                    modifiyingParameter = false;
                }
            }
        }
        lastsMidiMessage.clear();
        mutex.unlock();
    };
    void bindParameter(){
        listener = parameter.newListener([this](){
            if(!modifiyingParameter){
                value = 127;
                ofxMidiMessage message;
                message.status = status;
                message.channel = channel;
                switch(status){
                    case MIDI_CONTROL_CHANGE:
                    {
                        message.control = control;
                        message.value = value;
                        midiMessageSender.notify(this, message);
                        ofxMidiMessage copyMessage = message;
                        message.value = 0;
                        midiMessageSender.notify(this, copyMessage);
                        break;
                    }
                    case MIDI_NOTE_ON:
                    {
                        message.pitch = control;
                        message.velocity = value;
                        ofxMidiMessage copyMessage = message;
                        message.pitch = 0;
                        midiMessageSender.notify(this, copyMessage);
                        break;
                    }
                    default:
                        ofLog() << "Midi Type " << ofxMidiMessage::getStatusString(message.status) << " not supported for parameter of type Void";
                }
                
            }
        });
    };
    
    ofParameter<bool> &getModeParameter(){return mode;};
    
private:
    ofParameter<void>& parameter;
    ofParameter<bool> mode;
};
#endif /* ofxOceanodeMidiBinding_h */
