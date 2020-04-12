//
//  ofxOceanodeMidiController.cpp
//  example-midi
//
//  Created by Eduard Frigola Bagué on 19/08/2018.
//

#include "ofxOceanodeMidiController.h"
#include "ofxOceanodeContainer.h"
#include "ofxOceanodeMidiBinding.h"
#include "imgui.h"

ofxOceanodeMidiController::ofxOceanodeMidiController(shared_ptr<ofxOceanodePresetsController> _presetsController, shared_ptr<ofxOceanodeContainer> _container) : ofxOceanodeBaseController(_container, "MIDI"){
    midiLearn = false;
    midiDevices = container->getMidiDevices();
    container->setIsListeningMidi(midiLearn);
    presetsControl.setPresetsController(_presetsController);
//
//    gui->addLabel("Presets");
//    gui->addDropdown("Midi Device", midiDevices);
//    gui->addSlider(presetsControl.getChannel());
//    gui->addDropdown("Type", {"Note On", "Control Change", "Program Change"});
//
//    gui->addLabel("===========Bindings===========");
////
//    gui->onDropdownEvent(this, &ofxOceanodeMidiController::onGuiDropdownEvent);
//    gui->onTextInputEvent(this, &ofxOceanodeMidiController::onGuiTextInputEvent);
//
    container->addNewMidiMessageListener(&presetsControl);
}

void ofxOceanodeMidiController::draw(){
    ImGui::Begin(controllerName.c_str());
    if(ImGui::Checkbox("Midi Learn", &midiLearn)){
        container->setIsListeningMidi(midiLearn);
    }
    //TODO: Presets
    //TODO: Activate / Deactivate Midi Devices && refresh devices (or auto refresh?)
    float child_h = (ImGui::GetContentRegionAvail().y - (3 * ImGui::GetStyle().ItemSpacing.x));
    float child_w = ImGui::GetContentRegionAvail().x;
    ImGuiWindowFlags child_flags = ImGuiWindowFlags_MenuBar;
    ImGui::BeginChild("Midi Bindings", ImVec2(child_w, child_h), true, child_flags);
    if (ImGui::BeginMenuBar())
    {
        ImGui::TextUnformatted("Midi Bindings");
        ImGui::EndMenuBar();
    }
    
    if (ImGui::CollapsingHeader("Persistent")){
        for(auto &parameterBindings : container->getPersistentMidiBindings()){
            for(auto &binding : parameterBindings.second){
                createGuiForBinding(binding);
            }
        }
    }
    if (ImGui::CollapsingHeader("Current Preset")){
        for(auto &parameterBindings : container->getMidiBindings()){
            for(auto &binding : parameterBindings.second){
                createGuiForBinding(binding);
            }
        }
    }
    ImGui::EndChild();
    ImGui::End();
}

void ofxOceanodeMidiController::createGuiForBinding(shared_ptr<ofxOceanodeAbstractMidiBinding> binding){
    //TODO: Change Style
    if (ImGui::TreeNode(binding->getName().c_str())){
//        enum MesageTypes { MIDI_CONTROL_CHANGE, MIDI_NOTE_ON, MESSAGE_TYPES_COUNT};
//        const char* messageTypes_names[MESSAGE_TYPES_COUNT] = {"Control Change", "Note On"};
//        static int current_messageType = 0;//binding->getMessageType();
//        const char* current_messageType_name = binding->getMessageType()->c_str();//(current_element >= 0 && current_element < Element_COUNT) ? element_names[current_element] : "Unknown";
//        if(ImGui::SliderInt("Message Type", &current_messageType, 0, MESSAGE_TYPES_COUNT - 1, current_messageType_name)){
//            //TODO: Change controller type
//        }
        ImGui::Text("%s", binding->getMessageType()->c_str());
        if(ImGui::SliderInt("Channel", (int *)&binding->getChannel().get(), 0, 16)){
            binding->getChannel() = binding->getChannel();
        }
        //TODO: Change name if is note?
        if(ImGui::SliderInt("Control", (int *)&binding->getControl().get(), 0, 127)){
            binding->getControl() = binding->getControl();
        }
        if(ImGui::SliderInt("Midi Value", (int *)&binding->getValue().get(), 0, 127)){
            //TODO: Implement that can change parameter with this (if controller is not present;
            binding->getValue() = binding->getValue();
        }
        if(binding->type() == typeid(ofxOceanodeMidiBinding<float>).name()){
            auto midiBindingCasted = dynamic_pointer_cast<ofxOceanodeMidiBinding<float>>(binding);
            if(ImGui::SliderFloat("Min", (float*)&midiBindingCasted->getMinParameter().get(), midiBindingCasted->getMinParameter().getMin(), midiBindingCasted->getMinParameter().getMax())){
                midiBindingCasted->getMinParameter() = midiBindingCasted->getMinParameter();
            }
            if(ImGui::SliderFloat("Max", (float*)&midiBindingCasted->getMaxParameter().get(), midiBindingCasted->getMaxParameter().getMin(), midiBindingCasted->getMaxParameter().getMax())){
                midiBindingCasted->getMaxParameter() = midiBindingCasted->getMaxParameter();
            }
        }
        else if(binding->type() == typeid(ofxOceanodeMidiBinding<int>).name()){
            auto midiBindingCasted = dynamic_pointer_cast<ofxOceanodeMidiBinding<int>>(binding);
            if(ImGui::SliderInt("Min", (int*)&midiBindingCasted->getMinParameter().get(), midiBindingCasted->getMinParameter().getMin(), midiBindingCasted->getMinParameter().getMax())){
                midiBindingCasted->getMinParameter() = midiBindingCasted->getMinParameter();
            }
            if(ImGui::SliderInt("Max", (int*)&midiBindingCasted->getMaxParameter().get(), midiBindingCasted->getMaxParameter().getMin(), midiBindingCasted->getMaxParameter().getMax())){
                midiBindingCasted->getMaxParameter() = midiBindingCasted->getMaxParameter();
            }
        }
        else if(binding->type() == typeid(ofxOceanodeMidiBinding<vector<float>>).name()){
            auto midiBindingCasted = dynamic_pointer_cast<ofxOceanodeMidiBinding<vector<float>>>(binding);
            if(ImGui::SliderFloat("Min", (float*)&midiBindingCasted->getMinParameter().get(), midiBindingCasted->getMinParameter().getMin(), midiBindingCasted->getMinParameter().getMax())){
                midiBindingCasted->getMinParameter() = midiBindingCasted->getMinParameter();
            }
            if(ImGui::SliderFloat("Max", (float*)&midiBindingCasted->getMaxParameter().get(), midiBindingCasted->getMaxParameter().getMin(), midiBindingCasted->getMaxParameter().getMax())){
                midiBindingCasted->getMaxParameter() = midiBindingCasted->getMaxParameter();
            }
        }
        else if(binding->type() == typeid(ofxOceanodeMidiBinding<vector<int>>).name()){
            auto midiBindingCasted = dynamic_pointer_cast<ofxOceanodeMidiBinding<vector<int>>>(binding);
            if(ImGui::SliderInt("Min", (int*)&midiBindingCasted->getMinParameter().get(), midiBindingCasted->getMinParameter().getMin(), midiBindingCasted->getMinParameter().getMax())){
                midiBindingCasted->getMinParameter() = midiBindingCasted->getMinParameter();
            }
            if(ImGui::SliderInt("Max", (int*)&midiBindingCasted->getMaxParameter().get(), midiBindingCasted->getMaxParameter().getMin(), midiBindingCasted->getMaxParameter().getMax())){
                midiBindingCasted->getMaxParameter() = midiBindingCasted->getMaxParameter();
            }
        }
        else if(binding->type() == typeid(ofxOceanodeMidiBinding<bool>).name()){
            auto midiBindingCasted = dynamic_pointer_cast<ofxOceanodeMidiBinding<bool>>(binding);
            if(ImGui::Checkbox("Toggle", (bool *)&midiBindingCasted->getToggleParameter().get())){
                midiBindingCasted->getToggleParameter() = midiBindingCasted->getToggleParameter();
            }
        }
        else if(binding->type() == typeid(ofxOceanodeMidiBinding<void>).name()){
            auto midiBindingCasted = dynamic_pointer_cast<ofxOceanodeMidiBinding<void>>(binding);
            if(ImGui::Checkbox("Filter!=0", (bool *)&midiBindingCasted->getModeParameter().get())){
                midiBindingCasted->getModeParameter() = midiBindingCasted->getModeParameter();
            }
        }
        ImGui::Separator();
        ImGui::TreePop();
    }
}

//void ofxOceanodeMidiController::onGuiDropdownEvent(ofxDatGuiDropdownEvent e){
//    if(e.target->getName() == "Midi Device"){
//        presetsControl.setMidiDevice(e.target->getSelected()->getName());
//    }
//    else if(e.target->getName() == "Type"){
//        presetsControl.setType(e.target->getSelected()->getName());
//    }
//}

//void ofxOceanodeMidiController::setIsMidiLearn(bool b){
//    midiLearn->setChecked(b);
//    container->setIsListeningMidi(b);
//}
