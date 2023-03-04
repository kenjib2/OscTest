#include <vector>
#include <string.h>

#include "daisy_pod.h"
#include "daisysp.h"


using namespace daisy;
using namespace daisysp;
using namespace std;


const float INITIAL_OSC_COUNT = 30;
const float OSC_FREQ = 220.0f;
const float OSC_AMP = 0.1f;
const int SAMPLES_PER_BLOCK = 48;
const int MONITOR_CYCLES = 100;
const int NUM_WAVEFORMS = 8;
const int LOAD_LIMIT = 95;

DaisyPod hw;
vector<Oscillator> oscillators;
CpuLoadMeter cpuLoadMeter;
int monitorCycle = 0;
int waveforms[NUM_WAVEFORMS] = { 
					Oscillator::WAVE_TRI,
				    Oscillator::WAVE_SQUARE,
				    Oscillator::WAVE_SIN,
				    Oscillator::WAVE_SAW,
				    Oscillator::WAVE_RAMP,
				    Oscillator::WAVE_POLYBLEP_TRI,
				    Oscillator::WAVE_POLYBLEP_SQUARE,
				    Oscillator::WAVE_POLYBLEP_SAW
				    };
string waveformNames[NUM_WAVEFORMS] = {
						"WAVE_TRI",
						"WAVE_SQUARE",
						"WAVE_SIN",
						"WAVE_SAW",
						"WAVE_RAMP",
						"WAVE_POLYBLEP_TRI",
						"WAVE_POLYBLEP_SQUARE",
						"WAVE_POLYBLEP_SAW"
					};
int currentWaveform;


void AddOscillator(int waveform) {
	Oscillator oscillator;
	oscillator.Init(hw.AudioSampleRate());
	oscillator.SetWaveform(waveform);
	oscillator.SetFreq(OSC_FREQ);
	oscillator.SetAmp(OSC_AMP);

	oscillators.push_back(oscillator);
}


void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
	cpuLoadMeter.OnBlockStart();

	hw.ProcessAllControls();
	for (size_t i = 0; i < size; i++)
	{
		float sample = 0.0f;
		for (size_t i = 0; i < oscillators.size(); i++) {
			sample = oscillators[i].Process();
		}
		out[0][i] = sample;
		out[1][i] = sample;
	}

	cpuLoadMeter.OnBlockEnd();

	monitorCycle++;	
	if (monitorCycle >= MONITOR_CYCLES && currentWaveform < NUM_WAVEFORMS) {
		int max = cpuLoadMeter.GetMaxCpuLoad() * 100;
		cpuLoadMeter.Reset();
		if (max < LOAD_LIMIT) {
			hw.seed.Print(".");
			AddOscillator(waveforms[currentWaveform]);
		} else {
			hw.seed.PrintLine("\n%s: %d%% capacity reached at %d oscillators.",
					waveformNames[currentWaveform].c_str(),
					max,
					(int)oscillators.size()
					);
			oscillators.clear();
			currentWaveform++;
			for (int i = 0; i < INITIAL_OSC_COUNT; i++) {
				AddOscillator(waveforms[currentWaveform]);
			}
		}
		monitorCycle = 0;
	}
}


int main(void)
{
	hw.Init();
	hw.SetAudioBlockSize(SAMPLES_PER_BLOCK); // number of samples handled per callback
	hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    hw.seed.StartLog(true);

	cpuLoadMeter.Init(hw.AudioSampleRate(), hw.AudioBlockSize());

	currentWaveform = 0;
	for (int i = 0; i < INITIAL_OSC_COUNT; i++) {
		AddOscillator(waveforms[currentWaveform]);
	}

	hw.StartAdc();
	hw.StartAudio(AudioCallback);
	while(1) {}
}
