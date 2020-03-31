#pragma once


#include <string>
#include <unordered_map>

#include <plaid_midi2/midi2.h>


namespace dsbee
{
	using index_t = ptrdiff_t;

	using namespace midi2;

#if 0
	static inline const float PI = 3.1415926535f;


	/*
		Used to indicate the 
	*/
	struct index_range_t
	{
		index_t begin, end;
	};

	class SignalSlice
	{
		index_range_t range;

		virtual void* out_of_bounds(index_t begin, index_t end);
	};

	template<typename T>
	class Signal
	{
		T      *ptr;
		index_t begin, end; // available signal

		virtual T* out_of_bounds(index_t i) = 0;

		void operator[](index_t i)
		{
			if (i >= begin && i < end) return ptr[i];
		}
	};

	template<typename T>
	using SignalIn = Signal<const T>;


	class Buses
	{

	};
#endif

	/*
		This will be provided to the Processor when it starts working.
	*/
	struct AudioInfo
	{
		float sampleRate;
	};

	/*
		The base class for DSBee processors.
	*/
	class Processor
	{
	public:
		// virtual void process(Buses buses); // PLANNED

		virtual void start(AudioInfo info) = 0;

		virtual void process(const float *input, float *output, index_t count) = 0;

		virtual void midiIn(const UMP &event)    {}
	};

	Processor *GetProcessor();


	/*
		A very simple synth that produces samples one at a time.
	*/
	class Synth_OneByOne : public Processor
	{
	public:
		/*
			Override this method.
		*/
		virtual float makeSample() = 0;

		void process(const float *input, float *output, index_t count) override
		{
			for (index_t i = 0; i < count; ++i)
			{
				output[i] = makeSample();
			}
		}
	};

	/*
		A very simple effect that produces samples one at a time.
	*/
	class Effect_OneByOne : public Processor
	{
	public:
		/*
			Override this method.
		*/
		virtual float processSample(const float input) = 0;

		void process(const float *input, float *output, index_t count) override
		{
			for (index_t i = 0; i < count; ++i)
			{
				output[i] = processSample(input[i]);
			}
		}
	};

	/*
		A chain of processors.
	*/
	class Chain : public Processor
	{
	private:
		std::vector<Processor*> processors;
		std::vector<float>      temporary[2];

	public:
		// virtual void process(Buses buses); // PLANNED

		// Zero-length chain
		Chain() {}

		// Add processors
		void add(Processor *processor)
		{
			processors.push_back(processor);
		}

		// Construct from an array
		template<size_t Count>
		Chain(Processor* (&array_of_processors)[Count])
		{
			for (Processor *processor : array_of_processors) add(processor);
		}

		~Chain()
		{
			// Delete all processors in the chain
			while (processors.size())
			{
				delete processors.back();
				processors.pop_back();
			}
		}

		void start(AudioInfo info) override
		{
			for (size_t i = 0; i < processors.size(); ++i)
			{
				processors[i]->start(info);
			}
		}

		void process(const float *input, float *output, index_t count) override
		{
			// Make our temporary buffers the same size as the
			temporary[0].resize(count);
			temporary[1].resize(count);
			bool whichTemporary = false;

			if (!processors.size())
			{
				// A zero-length chain should just copy input to output.
				for (size_t i = 0; i < count; ++i) output[i] = input[i];

				return;
			}

			for (size_t i = 0; i < processors.size(); ++i)
			{
				// Last stage of processing?
				bool first = (i == 0), last = (i+1 == processors.size());

				// Decide the input and output buffers for this step.
				const float *stage_input  = (first ? input  : temporary[whichTemporary].data());
				whichTemporary = !whichTemporary;
				float       *stage_output = (last  ? output : temporary[whichTemporary].data());

				// Run the sub-process.
				processors[i]->process(stage_input, stage_output, count);
			}
		}

		void midiIn(const UMP &event) override
		{
			for (size_t i = 0; i < processors.size(); ++i)
			{
				processors[i]->midiIn(event);
			}
		}
	};
}