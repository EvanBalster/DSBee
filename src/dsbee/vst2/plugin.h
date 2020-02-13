#pragma once


#include <string>

#include <dsbee/dsbee.h>

#include "public.sdk/source/vst2.x/audioeffectx.h"

enum
{
	// Global
	kNumPrograms = 16,

	// Parameters Tags
	kAmp  = 0,
	kPadX = 1,
	kPadY = 2,

	kNumParams
};

class DSBeeEffect;

//------------------------------------------------------------------------
class DSBeeProgram
{
	friend class DSBeeEffect;
public:
	DSBeeProgram ();
	~DSBeeProgram () {}

private:
	float amp   = 0.5f;
	float pad_x = 0.5f;
	float pad_y = 0.5f;

	std::string name;
};

//------------------------------------------------------------------------
class DSBeeEffect : public AudioEffectX
{
public:
	DSBeeEffect (audioMasterCallback audioMaster);
	~DSBeeEffect ();

	//---from AudioEffect-----------------------
	virtual void processReplacing (float** inputs, float** outputs, VstInt32 sampleFrames);

	virtual void setProgram (VstInt32 program);
	virtual void setProgramName (char* name);
	virtual void getProgramName (char* name);
	virtual bool getProgramNameIndexed (VstInt32 category, VstInt32 index, char* text);

	virtual void setParameter (VstInt32 index, float value);
	virtual float getParameter (VstInt32 index);
	virtual void getParameterLabel (VstInt32 index, char* label);
	virtual void getParameterDisplay (VstInt32 index, char* text);
	virtual void getParameterName (VstInt32 index, char* text);

	virtual void resume ();

	virtual bool getEffectName (char* name);
	virtual bool getVendorString (char* text);
	virtual bool getProductString (char* text);
	virtual VstInt32 getVendorVersion () { return 1000; }

	virtual VstPlugCategory getPlugCategory () { return kPlugCategEffect; }

protected:
	//void setDelay (float delay);

	DSBeeProgram* programs;
	VstInt32 program_count;

	float* buffer;

	DSBeeProgram program;

	dsbee::Processor *processor;

	//long delay;
	//long size;
	//long cursor;
};