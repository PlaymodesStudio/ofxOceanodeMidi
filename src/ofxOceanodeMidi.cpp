//
//  ofxOceanodeMidi.cpp
//  example
//
//  Created by Eduard Frigola Bagué on 22/02/2021.
//

#include "ofxOceanodeMidi.h"
#include "ofxOceanode.h"
#include "midiOut.h"
#include "midiGateIn.h"
#include "midiGateInFast.h"
#include "midiControllerIn.h"
#include "midiHelpers.h"

namespace ofxOceanodeMidi{
void registerModels(ofxOceanode &o){
    o.registerModel<noteOut>("MIDI");
    o.registerModel<ctlOut>("MIDI");
    o.registerModel<controlChange>("MIDI");
    o.registerModel<programChange>("MIDI");
    o.registerModel<noteGate>("MIDI");
    o.registerModel<midiGateIn>("MIDI");
	o.registerModel<midiGateInFast>("MIDI");
    o.registerModel<controllerIn>("MIDI");
	o.registerModel<midiClockOut>("MIDI");
    o.registerModel<vectorHold>("Helpers");
}
void registerType(ofxOceanode &o){
    
}
void registerScope(ofxOceanode &o){
    
}
void registerCollection(ofxOceanode &o){
    registerModels(o);
    registerType(o);
    registerScope(o);
}
}
