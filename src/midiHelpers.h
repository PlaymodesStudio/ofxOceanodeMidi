//
//  midiHelpers.h
//  example
//
//  Created by Eduard Frigola Bagué on 23/02/2021.
//

#ifndef midiHelpers_h
#define midiHelpers_h

#include "ofxOceanodeNodeModel.h"

class vectorHold : public ofxOceanodeNodeModel{
public:
    vectorHold() : ofxOceanodeNodeModel("Vector Hold"){};
    
    void setup(){
        addParameter(input.set("Input", {0}, {0}, {1}));
        addParameter(keepNotes.set("Keep", false));
        addParameter(holding.set("Hold", false));
        addParameter(exitHold.set("Trig"));
        addParameter(output.set("Output", {0}, {0}, {1}));
        
        listeners.push(keepNotes.newListener([this](bool &b){
            outputStore = output;
        }));
        
        listeners.push(input.newListener([this](vector<float> &vf){
            if(vf.size() == outputStore.size()){
                if(vf != outputStore){
                    bool someOn = false;
                    bool changedToTrue = false;
                    for(int i = 0; i < vf.size(); i++){
                        if(vf[i] != 0){
                            someOn = true;
                            if(outputStore[i] == 0){
                                changedToTrue = true;
                                break;
                            }
                        }
                    }
                    if(!someOn){
                        holding = true;
                        output = outputStore;
                        if(!keepNotes){
                            outputStore = vector<float>(vf.size(), 0);
                        }
                    }else{
                        if(!keepNotes){
                            if(holding ){
                                exitHold.trigger();
                            }
                            holding = false;
                        }
                        if(changedToTrue && !keepNotes){
                            outputStore = vf;
                        }else{
                            for(int i = 0; i < vf.size(); i++) outputStore[i] = std::max(vf[i], outputStore[i]);
                        }
                        output = outputStore;
                    }
                }else{
                    //output = vf;
                }
            }else{
                outputStore = vf;
                output = vf;
            }
        }));
    }
    
private:
    
    ofParameter<vector<float>> input;
    ofParameter<bool> keepNotes;
    ofParameter<bool> holding;
    ofParameter<void> exitHold;
    ofParameter<vector<float>> output;
    
    ofEventListeners listeners;
    
    vector<float> outputStore;
};

#endif /* midiHelpers_h */
