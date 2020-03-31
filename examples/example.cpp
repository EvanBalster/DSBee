#include <dsbee/dsbee.h>

#include <iostream>

#include "utility.h" // useful stuff


using namespace dsbee;

// Magic variables corresponding to mouse position
extern float MOUSE_X, MOUSE_Y;


/*
	A nice base class for oscillators of all kinds
*/
class Oscillator : public Synth_OneByOne
{
public:
	// Our phase value stays between 0 and 1
	float phase  = 0.0f;

	float sampleRate = 48000.f;

	float last_midi_note = -1.0f;

	// This is called at the start of our program. It's getting the sample rate from the audio card
	void start(AudioInfo info) override
	{
		sampleRate = info.sampleRate;

		// Reset our phase.
		phase = 0.0f;
	}

	void midiIn(const UMP &event) override
	{
		// Does this Unviersal MIDI Packet contain a MIDI 1.0 voice message?
		if (event.messageType() == UMP::MIDI1_CHANNEL_VOICE)
		{
			auto &midi = (const UMP::Midi1_ChannelVoice&) event;

			// Note on?
			if (midi.opcode() == UMP::ChannelVoice::NOTE_ON)
			{
				last_midi_note = midi.noteNumber();
			}
		}
	}

	// A rule for our frequency control.
	float pickFrequency()
	{
		// MIDI note from controller if any, else from mouse
		float midi_note = last_midi_note;
		if (last_midi_note >= 0.0f) midi_note = last_midi_note;
		else                        midi_note = 36.f + 60.f * MOUSE_X;

		// Pick a frequency, calculating it from a MIDI note.
		return MidiFrequency(midi_note);
	}

	// Call this once per sample to advance the oscillator
	void advance(float frequency)
	{
		// Move our phase ahead proportional to frequency.
		//    Wrap it around so it stays between 0 and 1.
		//    (This keeps the float from getting large and imprecise)
		phase += frequency / sampleRate;
		phase = Wrap0to1(phase);
	}
};

/*
	A sine wave.
*/
class Osc_Sine : public Oscillator
{
public:
	// This is called once per audio sample.
	float makeSample() override
	{
		// Parameters for this oscillator
		float amp       = 1.0f;
		float frequency = pickFrequency();

		// Run the oscillator
		advance(frequency);

		// Final result
		return amp * std::sin(phase * TWO_PI);
	}
};

/*
	A square wave.
*/
class Osc_Square : public Oscillator
{
public:
	// This is called once per audio sample.
	float makeSample() override
	{
		// Parameters for this oscillator
		float amp       = 1.0f;
		float frequency = pickFrequency();

		// Run the oscillator
		advance(frequency);

		// Final result
		return amp * ((phase <= .5f) ? (1.0f) : (-1.0f));
	}
};

/*
	A saw wave.
*/
class Osc_Sawtooth : public Oscillator
{
public:
	// This is called once per audio sample.
	float makeSample() override
	{
		// Parameters for this oscillator
		float amp       = 1.0f;
		float frequency = pickFrequency();

		// Run the oscillator
		advance(frequency);

		// Final result
		return amp * (2.0f * phase - 1.0f);
	}
};

/*
	White noise.
*/
class Synth_Noise : public Synth_OneByOne
{
	void start(AudioInfo info) override {}

	float makeSample() override
	{
		return WhiteNoise();
	}
};


/*
	A simple filter.
*/
class Simple_Filter : public Effect_OneByOne
{
public:
	// The number of samples per second.
	float sampleRate = 48000.f;

	// Filter configuration
	float alpha;

	// Filter state:
	float in [2]; // The last two inputs
	float out[2]; // The last two outputs

	// This is called at the start of our program. It's getting the sample rate from the audio card
	void start(AudioInfo info) override
	{
		sampleRate = info.sampleRate;

		// Reset our filter state (pretend everything that came before was silence)
		in [0] = in [1] = 0.f;
		out[0] = out[1] = 0.f;

		// By default, set alpha to 1.
		alpha = 1.0f;
	}

	// This is called once per audio sample.
	float processSample(const float input) override
	{
		// Control our filter(s).
		alpha = std::pow(.01f, 1.0f - MOUSE_Y);


		// The current input.
		in[0] = input;

		// Average the last two inputs.
		//   (this cancels out the nyquist frequency, and makes our
		//    one-pole filter into a nice butterworth filter!)
		float last_two_avg = .5 * (in[0] + in[1]);

		// Now apply the familiar step from our one-pole filter.
		out[0] = out[1] + alpha * (last_two_avg - out[1]);

		// Next sample, these will become the previous input and output.
		in [1] = in [0];
		out[1] = out[0];

		// The current output
		return out[0];
	}
};


Processor *dsbee::GetProcessor()
{
	Processor *chain[] =
	{
		// A synth...
		new Osc_Sawtooth(),

		// ... then three simple filters
		new Simple_Filter(),
		new Simple_Filter(),
		new Simple_Filter(),
	};
	return new Chain(chain);
}