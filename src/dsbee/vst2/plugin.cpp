#define _CRT_SECURE_NO_WARNINGS 1

#include <algorithm>
#include <stdio.h>
#include <string.h>

#include "plugin.h"

#include <dsbee/dsbee.h>
#include <plaid_midi2/midi2.h>

//----------------------------------------------------------------------------- 
DSBeeProgram::DSBeeProgram ()
{
	// default Program Values

	name = "Default";
}

//-----------------------------------------------------------------------------
DSBeeEffect::DSBeeEffect (audioMasterCallback audioMaster)
	: AudioEffectX (audioMaster, kNumPrograms, kNumParams)
{
	programs = new DSBeeProgram[numPrograms];
	program = programs[0];

	if (programs)
		setProgram (0);

	setNumInputs (1);	// mono input
	setNumOutputs (1);	// mono output

	processor = dsbee::GetProcessor();

	setUniqueID ('iDSB');	// this should be unique, use the Steinberg web page for plugin Id registration

	resume ();		// flush buffer
}

//------------------------------------------------------------------------
DSBeeEffect::~DSBeeEffect ()
{
	if (buffer)
		delete[] buffer;
	if (programs)
		delete[] programs;
}

VstInt32 DSBeeEffect::canDo(char* text)
{
	if (strcmp(text, "receiveVstMidiEvent")) return 1;

	return 0;
}

VstInt32 DSBeeEffect::processEvents(VstEvents* events)
{
	using namespace midi2;
	for (VstInt32 i = 0; i < events->numEvents; ++i)
	{
		const VstEvent *event = events->events[i];
		switch (event->type)
		{
		case kVstMidiType:
			{
				auto midiEvent = (const VstMidiEvent*) event;

				auto midiBytes = (const uint8_t*) midiEvent->midiData;

				UMP packet = UMP(
					(uint32_t(UMP::MIDI1_CHANNEL_VOICE) << 28) |
					(uint32_t(midiBytes[0]) << 16) |
					(uint32_t(midiBytes[1]) << 8) |
					(uint32_t(midiBytes[2])));

				UMP::CV1 &midi1Packet = reinterpret_cast<UMP::CV1&>(packet);

				processor->midiIn(midi1Packet);
			}
			break;
		case kVstSysExType:
			break;
		}
	}

	return 1;
}

//------------------------------------------------------------------------
void DSBeeEffect::setProgram (VstInt32 program)
{
	DSBeeProgram* ap = &programs[std::min<VstInt32>(program, kNumPrograms-1)];

	curProgram = program;
	setParameter (kPadX, ap->pad_x);	
	setParameter (kPadY, ap->pad_y);
	setParameter (kAmp,  ap->amp);
}

//------------------------------------------------------------------------
void DSBeeEffect::setProgramName (char *name)
{
	programs[curProgram].name = name;
}

//------------------------------------------------------------------------
void DSBeeEffect::getProgramName (char *name)
{
	if (!strcmp (programs[curProgram].name.c_str(), "Init"))
		sprintf (name, "%s %d", programs[curProgram].name.c_str(), curProgram + 1);
	else
		strcpy (name, programs[curProgram].name.c_str());
}

//-----------------------------------------------------------------------------------------
bool DSBeeEffect::getProgramNameIndexed (VstInt32 category, VstInt32 index, char* text)
{
	if (index < kNumPrograms)
	{
		strcpy (text, programs[index].name.c_str());
		return true;
	}
	return false;
}

//------------------------------------------------------------------------
void DSBeeEffect::resume ()
{
	dsbee::AudioInfo info;
	info.sampleRate = this->sampleRate;

	processor->start(info);

	//memset (buffer, 0, size * sizeof (float));
	AudioEffectX::resume ();
}

//------------------------------------------------------------------------
void DSBeeEffect::setParameter (VstInt32 index, float value)
{
	DSBeeProgram* ap = &programs[curProgram];

	switch (index)
	{
	case kAmp  : program.amp   = value; break;
	case kPadX : program.pad_x = value; break;
	case kPadY : program.pad_y = value; break;
	}
}

//------------------------------------------------------------------------
float DSBeeEffect::getParameter (VstInt32 index)
{
	float v = 0;

	switch (index)
	{
	case kAmp  : v = program.amp; break;
	case kPadX : v = program.pad_x; break;
	case kPadY : v = program.pad_y; break;
	}
	return v;
}

//------------------------------------------------------------------------
void DSBeeEffect::getParameterName (VstInt32 index, char *label)
{
	switch (index)
	{
	case kAmp  : strcpy (label, "Amp");   break;
	case kPadX : strcpy (label, "Pad X"); break;
	case kPadY : strcpy (label, "Pad Y"); break;
	}
}

//------------------------------------------------------------------------
void DSBeeEffect::getParameterDisplay (VstInt32 index, char *text)
{
	switch (index)
	{
	case kAmp  :    dB2string (program.amp, text, kVstMaxParamStrLen);			break;
	case kPadX : float2string (program.pad_x, text, kVstMaxParamStrLen);	break;
	case kPadY : float2string (program.pad_y, text, kVstMaxParamStrLen);			break;
	}
}

//------------------------------------------------------------------------
void DSBeeEffect::getParameterLabel (VstInt32 index, char *label)
{
	switch (index)
	{
	case kAmp  : strcpy (label, "dB");	break;
	case kPadX : strcpy (label, "pos");	break;
	case kPadY : strcpy (label, "pos"); break;
	}
}

//------------------------------------------------------------------------
bool DSBeeEffect::getEffectName (char* name)
{
	strcpy (name, "DSBee Effect");
	return true;
}

//------------------------------------------------------------------------
bool DSBeeEffect::getProductString (char* text)
{
	strcpy (text, "DSBee Plugin");
	return true;
}

//------------------------------------------------------------------------
bool DSBeeEffect::getVendorString (char* text)
{
	strcpy (text, "imitone team");
	return true;
}

float MOUSE_X = 0.f, MOUSE_Y = 0.f;

//---------------------------------------------------------------------------
void DSBeeEffect::processReplacing (float** inputs, float** outputs, VstInt32 sampleFrames)
{
	float* in1 = inputs[0];
	float* out1 = outputs[0];
	//float* out2 = outputs[1];

	MOUSE_X = program.pad_x;
	MOUSE_Y = program.pad_y;

	processor->process(in1, out1, sampleFrames);

	/*while (--sampleFrames >= 0)
	{
		float x = *in++;
		buffer[cursor++] = x + y * fFeedBack;
		if (cursor >= delay)
			cursor = 0;
		*out1++ = y;
		if (out2)
			*out2++ = y;
	}*/
}

AudioEffect* createEffectInstance (audioMasterCallback audioMaster)
{
	return new DSBeeEffect (audioMaster);
}
