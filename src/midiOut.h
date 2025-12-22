//Created by Eduard Frigola
//Modified by Santi Vilanova


#ifndef midiOut_h
#define midiOut_h

#include "ofxOceanodeNodeModel.h"
#include "ofxMidi.h"

// NoteOut class
class noteOut : public ofxOceanodeNodeModel{
public:
    noteOut() : ofxOceanodeNodeModel("Note Out") {};

    void setup() {
        addParameterDropdown(midiPort, "Port", 0, midiOut.getOutPortList());
        addParameter(midiChannel.set("Ch", 1, 1, 16));
        addParameter(notes.set("Notes", {0}, {0}, {1}));

        listeners.push(midiPort.newListener([this](int &device){
            midiOut.openPort(device);
        }));

        listeners.push(notes.newListener([this](vector<float> &vf){
            if(vf.size() != notesStore.size()){
                notesStore.resize(vf.size(), 0.0);
            }
            for(int i = 0; i < vf.size(); i++){
                if(vf[i] != notesStore[i]){
                    if(vf[i] != 0){
                        if(notesStore[i] != 0){
                            midiOut.sendNoteOff(midiChannel, i);
                        }
                        midiOut.sendNoteOn(midiChannel, i, vf[i]*127);
                    }else{
                        midiOut.sendNoteOff(midiChannel, i);
                    }
                }
            }
            notesStore = vf;
        }));
    }
private:
    ofParameter<int> midiPort;
    ofParameter<int> midiChannel;
    ofParameter<vector<float>> notes;

    ofEventListeners listeners;

    ofxMidiOut midiOut;
    vector<float> notesStore;
};

// CtlOut class
class ctlOut : public ofxOceanodeNodeModel{
public:
    ctlOut() : ofxOceanodeNodeModel("Control Out") {}

    void setup() {
        addParameterDropdown(midiPort, "Port", 0, midiOut.getOutPortList());
        addParameter(midiChannel.set("Ch", 1, 1, 16));
        addParameter(cc.set("CC", {0}, {0}, {1}));

        listeners.push(midiPort.newListener([this](int &device){
            midiOut.openPort(device);
        }));

        listeners.push(cc.newListener([this](vector<float> &vf){
            if(vf.size() != ccStore.size()){
                ccStore.resize(vf.size(), 0.0);
            }
            for(int i = 0; i < vf.size(); i++){
                if(vf[i] != ccStore[i]){
                    midiOut.sendControlChange(midiChannel, i, vf[i]*127);
                }
            }
            ccStore = vf;
        }));
    }

private:
    ofParameter<int> midiPort;
    ofParameter<int> midiChannel;
    ofParameter<vector<float>> cc;

    ofEventListeners listeners;

    ofxMidiOut midiOut;
    vector<float> ccStore;
};

class controlChange : public ofxOceanodeNodeModel {
public:
    controlChange() : ofxOceanodeNodeModel("Control Change") {}

    void setup() {
        addParameterDropdown(midiPort, "Port", 0, midiOut.getOutPortList());
        addParameter(midiChannel.set("Ch", {1}, {1}, {16}));
        addParameter(ccNum.set("CC num", {0}, {0}, {127}));
        addParameter(ccVal.set("CC val", {0}, {0}, {127}));

        listeners.push(midiPort.newListener([this](int &device){
            midiOut.openPort(device);
        }));

        auto sendCC = [this]() {
            vector<int> ccNumbers = ccNum.get();
            vector<float> values = ccVal.get();
            vector<int> channels = midiChannel.get();

            // Ensure the sizes are consistent
            int minSize = std::min(std::min(ccNumbers.size(), values.size()), channels.size());

            for(int i = 0; i < minSize; i++){
                midiOut.sendControlChange(channels[i], ccNumbers[i], static_cast<int>(values[i]));
            }
        };

        listeners.push(ccNum.newListener([sendCC](vector<int> &){ sendCC(); }));
        listeners.push(ccVal.newListener([sendCC](vector<float> &){ sendCC(); }));
    }

private:
    ofParameter<int> midiPort;
    ofParameter<vector<int>> midiChannel;
    ofParameter<vector<int>> ccNum;
    ofParameter<vector<float>> ccVal;

    ofEventListeners listeners;

    ofxMidiOut midiOut;
};


class noteGate : public ofxOceanodeNodeModel {
public:
    noteGate() : ofxOceanodeNodeModel("Note Gate") {}

    void setup() {
        addParameterDropdown(midiPort, "Port", 0, midiOut.getOutPortList());
        addParameter(midiChannel.set("Ch", {1}, {1}, {16}));
        addParameter(noteGroup.set("Note", {0}, {-FLT_MAX}, {FLT_MAX}));
        addParameter(gateGroup.set("Gate", {0}, {0}, {1}));
        addParameter(noteTrig.set("Note Trig", false));
        addParameter(panicTrigger.set("Panic", false));

        listeners.push(midiPort.newListener([this](int &device){
            midiOut.openPort(device);
        }));

        listeners.push(noteGroup.newListener([this](vector<float> &vf){
            if(noteTrig) {
                updateMidiMessages();
            }
        }));

        listeners.push(gateGroup.newListener([this](vector<float> &vf){
            updateMidiMessages();
        }));

        listeners.push(panicTrigger.newListener([this](bool &val){
            if(val) {
                sendAllNoteOff();
                panicTrigger = false;  // Reset the trigger after sending
            }
        }));
    }

    void updateMidiMessages() {
        vector<float> vf = noteGroup.get();
        vector<float> gate = gateGroup.get();
        vector<int> channels = midiChannel.get();

        // Resize channels vector to match note vector size by duplicating last value
        if (!channels.empty() && channels.size() < vf.size()) {
            int lastChannel = channels.back();
            channels.resize(vf.size(), lastChannel);
        }

        // Resize storage vectors if needed
        if(vf.size() != noteGroupStore.size() || gate.size() != gateGroupStore.size()){
            noteGroupStore.resize(vf.size(), 0.0);
            gateGroupStore.resize(gate.size(), 0.0);
            channelsStore.resize(vf.size(), channels.back()); // Resize channel store to match note size
        }

        for(int i = 0; i < vf.size(); i++){
            if(i < gate.size() && i < noteGroupStore.size() && i < gateGroupStore.size()){
                bool noteChanged = noteTrig && (vf[i] != noteGroupStore[i]);
                bool gateChanged = gate[i] != gateGroupStore[i];

                // If the note has changed or gate is turned off, send a note-off for the previously active note on this channel
                if(noteChanged || (gateChanged && gate[i] == 0)) {
                    if (activeNotes.find(i) != activeNotes.end()) {
                        midiOut.sendNoteOff(channels[i], activeNotes[i], 0);
                        activeNotes.erase(i);
                    }
                }
                
                // If noteTrig=false, we only want to act upon gate changes, not note changes.
                if(!noteTrig){
                    noteChanged = false;
                }

                // Handle gate messages based on note or gate changes.
                if(gateChanged || noteChanged){
                    if(gate[i] != 0){
                        // If the previous gate value was not zero, send a note off for the current note
                        if(gateGroupStore[i] != 0 && activeNotes.find(i) != activeNotes.end()){
                            midiOut.sendNoteOff(channels[i], activeNotes[i], 0);
                        }

                        midiOut.sendNoteOn(channels[i], static_cast<int>(vf[i]), gate[i]*127);
                        activeNotes[i] = static_cast<int>(vf[i]);
                    }
                }
            }
        }

        noteGroupStore = vf;
        gateGroupStore = gate;
        channelsStore = channels;
    }

private:
    ofParameter<int> midiPort;
    ofParameter<vector<int>> midiChannel;
    ofParameter<vector<float>> noteGroup;
    ofParameter<vector<float>> gateGroup;
    ofParameter<bool> noteTrig;
    ofParameter<bool> panicTrigger;

    ofEventListeners listeners;

    ofxMidiOut midiOut;
    vector<float> noteGroupStore;
    vector<float> gateGroupStore;
    vector<int> channelsStore;
    std::map<int, int> activeNotes;

    void sendAllNoteOff() {
        for (int channel = 1; channel <= 16; ++channel) {
            for (int note = 0; note <= 127; ++note) {
                midiOut.sendNoteOff(channel, note, 0);
            }
        }
    }
};





class programChange : public ofxOceanodeNodeModel {
public:
    programChange() : ofxOceanodeNodeModel("Program Change") {}

    void setup() {
        addParameterDropdown(midiPort, "Port", 0, midiOut.getOutPortList());
        addParameter(midiChannel.set("Ch", {1}, {1}, {16}));
        addParameter(prgNum.set("Prg Num", {0}, {0}, {127}));

        listeners.push(midiPort.newListener([this](int &device){
            midiOut.openPort(device);
        }));

        listeners.push(prgNum.newListener([this](vector<int> &pn){
            vector<int> channels = midiChannel.get();

            // Ensure the sizes are consistent
            int minSize = std::min(pn.size(), channels.size());

            for(int i = 0; i < minSize; i++){
                midiOut.sendProgramChange(channels[i], pn[i]);
            }
        }));
    }

private:
    ofParameter<int> midiPort;
    ofParameter<vector<int>> midiChannel;
    ofParameter<vector<int>> prgNum;

    ofEventListeners listeners;

    ofxMidiOut midiOut;
};

class midiClockOut : public ofxOceanodeNodeModel {
public:
	midiClockOut() : ofxOceanodeNodeModel("MIDI Clock Out") {}

	void setup() override {

		// ---- MIDI ----
		addParameterDropdown(midiPort, "Port", 0, midiOut.getOutPortList());
		addParameter(enable.set("Enable", false));

		// ---- Inputs ----
		addParameter(ppq24.set("PPQ 24", 0, 0, INT_MAX));
		addParameter(play.set("Play", true));
		addParameter(reset.set("Reset"));

		// ---- Listeners ----
		listeners.push(midiPort.newListener([this](int &device){
			restartMidi(device);
		}));

		listeners.push(enable.newListener([this](bool &e){
			e ? startMidi() : stopMidi();
		}));

		listeners.push(ppq24.newListener([this](int &v){
			handlePpq(v);
		}));

		listeners.push(reset.newListener([this](){
			sendStart();
			lastPpq = 0;
		}));
	}

private:

	// ---------- MIDI management ----------

	void startMidi() {
		if(midiOut.isOpen()) return;
		midiOut.openPort(midiPort);
	}

	void stopMidi() {
		if(midiOut.isOpen()) midiOut.closePort();
	}

	void restartMidi(int device) {
		stopMidi();
		if(enable) startMidi();
	}

	// ---------- Core logic ----------

	void handlePpq(int ppq) {
		if(!enable || !midiOut.isOpen()) return;

		// Detect scrub / jump
		if(ppq != lastPpq + 1) {
			sendSPP(ppq);
			if(play) sendContinue();
		}
		// Normal playback
		else if(play) {
			sendClock();
		}

		lastPpq = ppq;
	}

	// ---------- MIDI send helpers ----------

	void sendClock() {
		midiOut.sendMidiByte(0xF8); // MIDI Clock
	}

	void sendStart() {
		midiOut.sendMidiByte(0xFA); // Start
	}

	void sendStop() {
		midiOut.sendMidiByte(0xFC); // Stop
	}

	void sendContinue() {
		midiOut.sendMidiByte(0xFB); // Continue
	}

	void sendSPP(int ppq) {
		// SPP units = MIDI beats = PPQ24 / 6
		int spp = ppq / 6;

		unsigned char lsb = spp & 0x7F;
		unsigned char msb = (spp >> 7) & 0x7F;

		std::vector<unsigned char> msg;
		msg.push_back(0xF2); // Song Position Pointer
		msg.push_back(lsb);
		msg.push_back(msb);

		midiOut.sendMidiBytes(msg);
	}

	// ---------- Parameters ----------

	ofParameter<int>  midiPort;
	ofParameter<bool> enable;

	ofParameter<int>  ppq24;
	ofParameter<bool> play;
	ofParameter<void> reset;

	// ---------- State ----------

	int lastPpq = -1;

	ofEventListeners listeners;
	ofxMidiOut midiOut;
};




#endif /* midiOut_h */
