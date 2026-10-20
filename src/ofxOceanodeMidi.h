//
//  ofxOceanodeMidi.h
//  example
//
//  Created by Eduard Frigola Bagué on 22/02/2021.
//

#ifndef ofxOceanodeMidi_h
#define ofxOceanodeMidi_h

// Declarations only: the node headers and the bodies live in ofxOceanodeMidi.cpp, so
// including this header does not compile every node of the addon again.

class ofxOceanode;

namespace ofxOceanodeMidi{

void registerModels(ofxOceanode &o);
void registerType(ofxOceanode &o);
void registerScope(ofxOceanode &o);
void registerCollection(ofxOceanode &o);

}

#endif /* ofxOceanodeMidi_h */
