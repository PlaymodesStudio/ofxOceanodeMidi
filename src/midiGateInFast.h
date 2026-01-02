//
//  midiGateInFast.h
//
//  Fast MIDI Gate Input - updates output immediately on MIDI arrival
//  instead of waiting for 60fps update cycle
//

#ifndef midiGateInFast_h
#define midiGateInFast_h

#include "ofxOceanodeNodeModel.h"
#include "ofxMidi.h"
#include <mutex>
#include <map>

class midiGateInFast : public ofxOceanodeNodeModel, ofxMidiListener {
public:
	midiGateInFast() : ofxOceanodeNodeModel("Note In Fast") {
		midiIn = nullptr;
	}
	
	~midiGateInFast() {
		if(midiIn != nullptr) {
			delete midiIn;
		}
	}
	
	void setup() override {
		midiIn = new ofxMidiIn();
		
		midiports = {"None"};
		midiports.resize(1 + midiIn->getNumInPorts());
		for(int i = 0; i < midiIn->getNumInPorts(); i++) {
			midiports[i+1] = midiIn->getInPortList()[i];
		}
		
		addParameterDropdown(midiDevice, "Device", 0, midiports, ofxOceanodeParameterFlags_DisableSavePreset);
		addParameter(midiChannel.set("Channel", 0, 0, 16));
		
		ofParameter<char> noteslabel("Notes", ' ');
		addParameter(noteslabel, ofxOceanodeParameterFlags_DisableSavePreset);
		
		addParameter(noteOnStart.set("Start", 0, 0, 127));
		addParameter(noteOnEnd.set("End", 127, 0, 127));
		
		addOutputParameter(output.set("Output", {0}, {0}, {1}));
		addOutputParameter(pitch.set("Pitch", {0.0f}, {0.0f}, {127.0f}));
		addOutputParameter(gate.set("Gate", {0.0f}, {0.0f}, {1.0f}));
		
		outputStore.resize(noteOnEnd - noteOnStart + 1, 0);
		
		listeners.push(noteOnStart.newListener(this, &midiGateInFast::noteRangeChanged));
		listeners.push(noteOnEnd.newListener(this, &midiGateInFast::noteRangeChanged));
		listeners.push(midiDevice.newListener(this, &midiGateInFast::midiDeviceListener));
	}
	
	void update(ofEventArgs &e) override {
		// Clean up note-off queue in main thread
		// This prevents accumulation but doesn't block MIDI processing
		std::lock_guard<std::mutex> lock(mutex);
		
		activatedNotes.clear();
		
		for(auto &n : toShutNotes) {
			if(n < outputStore.size()) {
				outputStore[n] = 0;
			}
			// Remove from active notes map
			activeNotesMap.erase(n + noteOnStart);
		}
		toShutNotes.clear();
		
		// Update pitch/gate outputs from active notes
		updatePitchGateOutputs();
	}
	
	void presetSave(ofJson &json) override {
		json["DeviceName"] = midiports[midiDevice];
	}
	
	void presetRecallBeforeSettingParameters(ofJson &json) override {
		string name = "None";
		if(json.count("DeviceName") == 1) {
			name = json["DeviceName"];
		}
		auto it = std::find(midiports.begin(), midiports.end(), name);
		
		if(it != midiports.end()) {
			midiDevice = it - midiports.begin();
		} else {
			midiDevice = 0;
		}
	}
	
private:
	void newMidiMessage(ofxMidiMessage &eventArgs) {
		bool shouldUpdateOutput = false;
		
		// Note ON
		if((eventArgs.status == MIDI_NOTE_ON && eventArgs.velocity != 0) &&
		   (eventArgs.channel == midiChannel || midiChannel == 0)) {
			if(eventArgs.pitch >= noteOnStart && eventArgs.pitch <= noteOnEnd) {
				std::lock_guard<std::mutex> lock(mutex);
				
				int index = eventArgs.pitch - noteOnStart;
				if(index >= 0 && index < outputStore.size()) {
					float velocity = (float)eventArgs.velocity / 127.0f;
					outputStore[index] = velocity;
					activatedNotes.push_back(index);
					
					// Add to active notes map
					activeNotesMap[eventArgs.pitch] = velocity;
					
					shouldUpdateOutput = true;
				}
			}
		}
		// Note OFF
		else if((eventArgs.status == MIDI_NOTE_OFF ||
				(eventArgs.status == MIDI_NOTE_ON && eventArgs.velocity == 0)) &&
				(eventArgs.channel == midiChannel || midiChannel == 0)) {
			if(eventArgs.pitch >= noteOnStart && eventArgs.pitch <= noteOnEnd) {
				std::lock_guard<std::mutex> lock(mutex);
				
				int index = eventArgs.pitch - noteOnStart;
				if(index >= 0 && index < outputStore.size()) {
					// Check if this note was recently activated (should be queued for shutdown)
					if(find(activatedNotes.begin(), activatedNotes.end(), index) != activatedNotes.end()) {
						toShutNotes.push_back(index);
					} else {
						// Immediate shutdown
						outputStore[index] = 0;
						activeNotesMap.erase(eventArgs.pitch);
						shouldUpdateOutput = true;
					}
				}
			}
		}
		
		// CRITICAL: Update output parameter immediately from MIDI thread
		// This allows downstream nodes to receive MIDI at full rate
		if(shouldUpdateOutput) {
			// Make a copy while holding the lock
			vector<float> currentOutput;
			{
				std::lock_guard<std::mutex> lock(mutex);
				currentOutput = outputStore;
			}
			
			// Update parameter outside the lock to prevent deadlocks
			// NOTE: This happens in the MIDI callback thread!
			// Downstream nodes MUST be thread-safe or only do pure computation
			output = currentOutput;
			
			// Update pitch/gate outputs
			updatePitchGateOutputs();
		}
	}
	
	void updatePitchGateOutputs() {
		// Already locked by caller
		
		vector<float> pitchOut;
		vector<float> gateOut;
		
		// Build vectors from active notes map (sorted by pitch)
		for(const auto& pair : activeNotesMap) {
			pitchOut.push_back((float)pair.first);  // MIDI note number
			gateOut.push_back(pair.second);         // Velocity (0-1)
		}
		
		// If no active notes, output single zero
		if(pitchOut.empty()) {
			pitchOut.push_back(0.0f);
			gateOut.push_back(0.0f);
		}
		
		pitch = pitchOut;
		gate = gateOut;
	}
	
	void midiDeviceListener(int &device) {
		std::lock_guard<std::mutex> lock(mutex);
		
		outputStore = vector<float>(noteOnEnd - noteOnStart + 1, 0);
		activeNotesMap.clear();
		
		midiIn->closePort();
		if(device > 0) {
			midiIn->openPort(device - 1);
			midiIn->addListener(this);
		}
		
		updatePitchGateOutputs();
	}
	
	void noteRangeChanged(int &note) {
		std::lock_guard<std::mutex> lock(mutex);
		
		if(noteOnEnd < noteOnStart) {
			noteOnStart = 0;
		} else {
			outputStore.resize(noteOnEnd - noteOnStart + 1, 0);
		}
		
		// Clear active notes that are now out of range
		auto it = activeNotesMap.begin();
		while(it != activeNotesMap.end()) {
			if(it->first < noteOnStart || it->first > noteOnEnd) {
				it = activeNotesMap.erase(it);
			} else {
				++it;
			}
		}
		
		updatePitchGateOutputs();
	}
	
	ofEventListeners listeners;
	
	ofParameter<int> midiDevice;
	ofParameter<int> midiChannel;
	ofParameter<int> noteOnStart;
	ofParameter<int> noteOnEnd;
	
	ofParameter<vector<float>> output;
	ofParameter<vector<float>> pitch;
	ofParameter<vector<float>> gate;
	
	ofxMidiIn* midiIn;
	vector<float> outputStore;
	vector<int> activatedNotes;
	vector<int> toShutNotes;
	std::map<int, float> activeNotesMap;  // pitch -> velocity
	std::mutex mutex;
	vector<string> midiports;
};

#endif /* midiGateInFast_h */
