//
//  ofxOceanodeMidi.h
//  example
//
//  Created by Eduard Frigola Bagué on 22/02/2021.
//

#ifndef ofxOceanodeMidi_h
#define ofxOceanodeMidi_h

#include "midiOut.h"
#include "midiGateIn.h"
#include "midiGateInFast.h"
#include "midiControllerIn.h"
#include "midiHelpers.h"

namespace ofxOceanodeMidi{
static void registerModels(ofxOceanode &o){
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
static void registerType(ofxOceanode &o){
    
}
static void registerScope(ofxOceanode &o){
    
}
static void registerCollection(ofxOceanode &o){
    registerModels(o);
    registerType(o);
    registerScope(o);
}
}


#endif /* ofxOceanodeMidi_h */
