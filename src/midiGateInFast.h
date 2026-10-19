#ifndef midiGateInFast_h
#define midiGateInFast_h

#include "ofxOceanodeNodeModel.h"
#include "ofxMidi.h"
#include <mutex>
#include <map>
#include <atomic>

class midiGateInFast : public ofxOceanodeNodeModel, ofxMidiListener {
public:
	midiGateInFast() : ofxOceanodeNodeModel("Note In Fast") {
		midiIn = nullptr;
	}

	~midiGateInFast() {
		if(midiIn != nullptr) {
			midiIn->removeListener(this);
			midiIn->closePort();
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

		addParameter(panicTrigger.set("Panic"));

		ofParameter<char> noteslabel("Notes", ' ');
		addParameter(noteslabel, ofxOceanodeParameterFlags_DisableSavePreset);

		addParameter(noteOnStart.set("Start", 0, 0, 127));
		addParameter(noteOnEnd.set("End", 127, 0, 127));

		addOutputParameter(output.set("Output", {0}, {0}, {1}));
		addOutputParameter(pitch.set("Pitch", {0.0f}, {0.0f}, {127.0f}));
		addOutputParameter(velocity.set("Velocity", {0.0f}, {0.0f}, {1.0f}));
		addOutputParameter(gate.set("Gate", {0}, {0}, {1}));

		outputStore.resize(noteOnEnd - noteOnStart + 1, 0);

		listeners.push(noteOnStart.newListener(this, &midiGateInFast::noteRangeChanged));
		listeners.push(noteOnEnd.newListener(this, &midiGateInFast::noteRangeChanged));
		listeners.push(midiDevice.newListener(this, &midiGateInFast::midiDeviceListener));

		listeners.push(panicTrigger.newListener([this](){
			std::lock_guard<std::mutex> lock(mutex);

			std::fill(outputStore.begin(), outputStore.end(), 0.0f);
			activatedNotes.clear();
			toShutNotes.clear();

			output = outputStore;
			updateSlotOutputs();
		}));
	}

	void update(ofEventArgs &e) override {
		std::lock_guard<std::mutex> lock(mutex);

		for(auto &n : toShutNotes) {
			if(n < (int)outputStore.size()) {
				outputStore[n] = 0;
			}
		}

		if(!toShutNotes.empty()) {
			toShutNotes.clear();
			activatedNotes.clear();

			output = outputStore;
			updateSlotOutputs();
		}
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
				if(index >= 0 && index < (int)outputStore.size()) {
					float vel = (float)eventArgs.velocity / 127.0f;
					outputStore[index] = vel;
					activatedNotes.push_back(index);
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
				if(index >= 0 && index < (int)outputStore.size()) {
					if(find(activatedNotes.begin(), activatedNotes.end(), index) != activatedNotes.end()) {
						toShutNotes.push_back(index);
					} else {
						outputStore[index] = 0;
						shouldUpdateOutput = true;
					}
				}
			}
		}

		if(shouldUpdateOutput) {
			std::lock_guard<std::mutex> lock(mutex);
			output = outputStore;
			updateSlotOutputs();
		}
	}

	// Called with mutex held.
	// Outputs slot-based vectors compatible with scVST pitch/velocity/gate inputs:
	//   pitch[i]    = (float)(noteOnStart + i)   — always the note for that slot
	//   velocity[i] = outputStore[i]              — normalized 0..1, 0 when inactive
	//   gate[i]     = 1 when active, 0 otherwise  — int, edge-detected by scVST
	void updateSlotOutputs() {
		int numSlots = (int)outputStore.size();

		vector<float> pitchOut(numSlots);
		vector<float> velocityOut(numSlots);
		vector<int>   gateOut(numSlots);

		for(int i = 0; i < numSlots; i++) {
			pitchOut[i]    = (float)(noteOnStart + i);
			velocityOut[i] = outputStore[i];
			gateOut[i]     = outputStore[i] > 0.0f ? 1 : 0;
		}

		pitch    = pitchOut;
		velocity = velocityOut;
		gate     = gateOut;
	}

	void midiDeviceListener(int &device) {
		std::lock_guard<std::mutex> lock(mutex);

		outputStore = vector<float>(noteOnEnd - noteOnStart + 1, 0);
		activatedNotes.clear();
		toShutNotes.clear();

		midiIn->closePort();
		if(device > 0) {
			midiIn->openPort(device - 1);
			midiIn->addListener(this);
		}

		output = outputStore;
		updateSlotOutputs();
	}

	void noteRangeChanged(int &note) {
		std::lock_guard<std::mutex> lock(mutex);

		if(noteOnEnd < noteOnStart) {
			noteOnStart = 0;
		} else {
			outputStore.resize(noteOnEnd - noteOnStart + 1, 0);
		}

		output = outputStore;
		updateSlotOutputs();
	}

	ofEventListeners listeners;

	ofParameter<int> midiDevice;
	ofParameter<int> midiChannel;
	ofParameter<void> panicTrigger;
	ofParameter<int> noteOnStart;
	ofParameter<int> noteOnEnd;

	ofParameter<vector<float>> output;
	ofParameter<vector<float>> pitch;
	ofParameter<vector<float>> velocity;
	ofParameter<vector<int>>   gate;

	ofxMidiIn* midiIn;
	vector<float> outputStore;
	vector<int> activatedNotes;
	vector<int> toShutNotes;
	std::mutex mutex;
	vector<string> midiports;
};

#endif /* midiGateInFast_h */
