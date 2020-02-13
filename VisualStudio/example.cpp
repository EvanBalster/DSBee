#include <dsbee/dsbee.h>

using namespace dsbee;


static const float PI = 3.1415926535f;

static const float TWO_PI = 2.f * PI;


class Synth_Example : public Synth_OneByOne
{
public:
	float phase = 0.0f;

	float sampleRate = 48000.f;

	void start(AudioInfo info) override
	{
		sampleRate = info.sampleRate;
	}

	float makeSample() override
	{
		phase += TWO_PI * (2000.f / sampleRate);

		return std::sin(phase);
	}
};


Processor *dsbee::GetProcessor()
{
	return new Synth_Example();
}