#pragma once


#include <string>
#include <unordered_map>


namespace dsbee
{
	using index_t = ptrdiff_t;

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
	*/
	class Processor
	{
	public:
		// virtual void process(Buses buses); // PLANNED

		virtual void start(AudioInfo info) = 0;

		virtual void process(float *output, index_t count) = 0;
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

		void process(float *output, index_t count) override
		{
			for (index_t i = 0; i < count; ++i)
			{
				output[i] = makeSample();
			}
		}
	};
}