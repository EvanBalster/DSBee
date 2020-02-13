#include <dsbee/dsbee.h>

#include "utility.h" // useful stuff


using namespace dsbee;

// Magic variables corresponding to mouse position
extern float MOUSE_X, MOUSE_Y;



class Synth_Example : public Synth_OneByOne
{
public:
	// Our phase value stays between 0 and 1
	float phase  = 0.0f;

	// The number of samples per second.
	float sampleRate = 48000.f;

	float filter = 0.;

	// This is called at the start of our program. It's getting the sample rate from the audio card
	void start(AudioInfo info) override
	{
		sampleRate = info.sampleRate;
	}

	// This is called once per audio sample.
	float makeSample() override
	{
		// Pick a frequency
		float frequency = (-5.f + 400.f * MOUSE_X);

		float amplitude = 1.0f;

		float filter_alpha = std::pow(.001f, 1.0f - MOUSE_Y);

		// Move our phase ahead proportional to frequency.
		//    Wrap it around so it stays between 0 and 1.
		//    (This keeps the float from getting large and imprecise)
		phase += frequency / sampleRate;
		phase = Wrap0to1(phase);

		// Make a sine wave based on the phase.
		float sine = std::sin(phase * TWO_PI);

		// Make square wave.
		float square = ((phase <= .5f) ? (1.0f) : (-1.0f));

		// Make saw tooth wave
		float sawtooth = (2.0f * phase - 1.0f);

		// Use a utility function to make some white noise
		float whiteNoise = WhiteNoise();

		// Mix the synth and white noise together
		float mix =
			+ 0.50f * sawtooth
			+ 0.50f * whiteNoise;

		// A very simple filter
		filter += (mix - filter) * filter_alpha;

		return filter;
	}
};


Processor *dsbee::GetProcessor()    {return new Synth_Example();}