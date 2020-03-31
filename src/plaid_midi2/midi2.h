#pragma once


#include <algorithm>
#include <cstdint>


namespace midi2
{
	class UMP;

	using MidiPacket = UMP;

	/*
		Structure for holding Universal MIDI Packets.
			
			This structure is 4 words (128 bits) in size, but 
	*/
	class UMP
	{
	public:
		using word_t = uint32_t;

		using group_t = uint8_t;

		class Utility;
		class Data_8_Byte;
		class Data_16_Byte;
		class System;
		class ChannelVoice;
		class Midi1_ChannelVoice;
		class Midi2_ChannelVoice;

		using Data8  = Data_8_Byte;
		using Data16 = Data_16_Byte;
		using CV1    = Midi1_ChannelVoice;
		using Midi1  = Midi1_ChannelVoice;
		using CV2    = Midi2_ChannelVoice;
		using Voice  = Midi2_ChannelVoice;


	public:
		word_t words[4];


	public:
		enum MESSAGE_TYPE
		{
			UTILITY      = 0,
			SYSTEM       = 1,
			MIDI1_VOICE  = 2, MIDI1_CHANNEL_VOICE = 2,
			DATA_8_BYTE  = 3,
			MIDI2_VOICE  = 4, MIDI2_CHANNEL_VOICE = 4,
			DATA_16_BYTE = 5,

			UNKNOWN  = 254,
			RESERVED = 255,
		};

		typedef MESSAGE_TYPE MT;


		/*
			A protocol type.
		*/
		class Protocol
		{
		public:
			enum PROTOCOL_TYPE
			{
				MIDI_1_0 = 0x01,
				MIDI_2_0 = 0x02,
			};

			enum EXTENSIONS
			{
				EXT_JR_TIMESTAMPS = 0x01,
				EXT_MIDI1_SIZE    = 0x02, // Use 96- and 64-bit message types in 
			};

		public:
			uint8_t type;
			uint8_t version;
			uint8_t extensions;


		public:
			/*
				Default constructor defines the MIDI 1.0 protocol.
					All UMP devices shall start up using MIDI 1.0 protocol, no extensions.
			*/
			Protocol()                                                              : type(MIDI_1_0), version(0), extensions(0) {}
			Protocol(PROTOCOL_TYPE _type, uint8_t _version, uint8_t _extensions)    : type(_type), version(_version), extensions(_extensions) {}

			/*
			*/
			static Protocol Midi_1_0(bool jr_timestamps = false, bool size_of_packets = false)    {return Protocol(MIDI_1_0, 0, uint8_t(size_of_packets<<1u) | uint8_t(jr_timestamps));}
			static Protocol Midi_2_0(bool jr_timestamps = false)                                  {return Protocol(MIDI_2_0, 0, jr_timestamps);}

			/*
				Check protocol
			*/
			bool is_known() const    {return type == MIDI_1_0 || type == MIDI_2_0;}
			bool is_midi1() const    {return type == MIDI_1_0;}
			bool is_midi2() const    {return type == MIDI_2_0;}

			/*
				Get the version number of a MIDI protocol (0.0 for non-midi protocols)
			*/
			uint8_t midi_version_major   () const    {if (type <= 2) return type;}
			uint8_t midi_version_minor   () const    {return 0;}
			uint8_t midi_version_revision() const    {return 0;}

			/*
				Check feature support
			*/
			bool has_large_packets() const    {return type == MIDI_2_0 || (type == MIDI_1_0 && (extensions & EXT_MIDI1_SIZE));}

			bool has_utility      () const    {return type == MIDI_1_0 || type == MIDI_2_0;}
			bool has_jr_timestamps() const    {return has_utility() && (extensions & EXT_JR_TIMESTAMPS);}
			bool has_midi1_voice  () const    {return type == MIDI_1_0;}
			bool has_midi2_voice  () const    {return type == MIDI_2_0;}
			bool has_data_8_byte  () const    {return type == MIDI_1_0 || type == MIDI_2_0;}
			bool has_data_16_byte () const    {return has_large_packets();}
		};


	public:
		/*
			Default constructor: a NOOP message on group 0
		*/
		UMP() :   words{0,0,0,0} {}

		/*
			Construct a MIDI++ event manually.
		*/
		UMP(uint32_t word_0, uint32_t word_1 = 0, uint32_t word_2 = 0, uint32_t word_3 = 0)
		{
			words[0] = word_0;
			words[1] = word_1;
			words[2] = word_2;
			words[3] = word_3;
		}

		/*
			NOOP messages
		*/
		static UMP NOOP()         {return UMP(0);}

		bool   is_noop() const    {return (words[0] & 0xF0F00000) == 0;}


		/*
			All message types:
				Query the group (0-15).
				Query the message type (0-15).
				Query the message size in words (1-4)
				Identify the message type in the current protocol (MESSAGE_TYPE)
		*/
		uint8_t      group()       const    {return (words[0]>>24) & 0xF;}
		uint8_t      messageType() const    {return (words[0]>>28) & 0xF;}
		uint32_t     messageSize() const;

		MESSAGE_TYPE identify(const Protocol &protocol) const;


		/*
			Check for standard message types
		*/
		bool   is_utility     (const Protocol &protocol) const    {return identify(protocol) == UTILITY;}
		bool   is_system      (const Protocol &protocol) const    {return identify(protocol) == SYSTEM;}
		bool   is_data_8_byte (const Protocol &protocol) const    {return identify(protocol) == DATA_8_BYTE;}
		bool   is_data_16_byte(const Protocol &protocol) const    {return identify(protocol) == DATA_16_BYTE;}
		bool   is_midi1_voice (const Protocol &protocol) const    {return identify(protocol) == MIDI1_VOICE;}
		bool   is_midi2_voice (const Protocol &protocol) const    {return identify(protocol) == MIDI2_VOICE;}
	};


	class UMP::Utility : public UMP
	{
	public:
		typedef uint16_t jr_time_t;

		// Utility opcodes, used with MT_UTILITY
		enum UTILITY_OP
		{
			NOOP         = 0x0,
			JR_CLOCK     = 0x1,
			JR_TIMESTAMP = 0x2,
		};


	public:
		/*
			Construct JR timing messages.
				gpchan_t -- only the group is considered.
				jr_time  -- 16-bit looping timestamp in units of 1/31250 (32/1000000) of a second.
		*/
		static UMP JR_Clock    (group_t, jr_time_t);
		static UMP JR_TimeStamp(group_t, jr_time_t);
	};

	class UMP::Data_8_Byte : public UMP
	{
		// Data opcodes, used with MT_DATA and MT_DATA_EXT
		enum STATUS
		{
			// 7-bit data messages, used with MT_DATA
			SYSEX7_COMPLETE = 0x0,
			SYSEX7_BEGIN    = 0x1,
			SYSEX7_CONTINUE = 0x2,
			SYSEX7_END      = 0x3,

			// 8-bit data messages, used with MT_DATA_EXT
			SYSEX8_COMPLETE = 0x4,
			SYSEX8_BEGIN    = 0x5,
			SYSEX8_CONTINUE = 0x6,
			SYSEX8_END      = 0x7,

			MIXED_DATA_HEADER  = 0x8,
			MIXED_DATA_PAYLOAD = 0x9,
		};
	};


	class UMP::System : public UMP
	{
	public:
		// System opcodes, used with MT_SYSTEM
		enum STATUS
		{
			// System Common messages
			TIME_CODE     = 0xF1, MTC      = 0xF1,
			SONG_POSITION = 0xF2,
			SONG_SELECT   = 0xF3,
			TUNE_REQUEST  = 0xF6,

			// System Realtime messages
			TIMING_CLOCK   = 0xF8,
			START          = 0xFA,
			CONTINUE       = 0xFB,
			STOP           = 0xFC,
			ACTIVE_SENSING = 0xFE,
			RESET          = 0xFF,
		};
	};

	/*
		Casting for Channel Voice messages (both MIDI 1.0 and MIDI 2.0).
	*/
	class UMP::ChannelVoice : public UMP
	{
	public:
		typedef uint8_t grpchan_t;
		typedef uint8_t notenum_t;

		// Channel Voice opcodes, used with MT_MIDI_VOICE or MT_HD_VOICE
		enum OPCODE
		{
			NOTE_OFF        = 0x8,
			NOTE_ON         = 0x9,
			CHAN_PITCH_BEND = 0xE, PITCH_BEND     = 0xE,
			CHAN_CONTROL    = 0xB, CHAN_CC = 0xB, CC = 0xB,
			CHAN_PRESSURE   = 0xD,
			NOTE_PRESSURE   = 0xA,
			PROGRAM_CHANGE  = 0xC, PROGRAM = 0xC,

			// MIDI 2.0 Channel Voice messages
			CHAN_REGISTERED_CONTROL          = 0x2, CHAN_RC = 0x2, RC   = 0x2,
			CHAN_ASSIGNABLE_CONTROL          = 0x3, CHAN_AC = 0x3, AC   = 0x3,
			CHAN_RELATIVE_REGISTERED_CONTROL = 0x4, REL_RC  = 0x2,
			CHAN_RELATIVE_ASSIGNABLE_CONTROL = 0x5, REL_AC  = 0x3,
			NOTE_PITCH_BEND                  = 0x6,
			NOTE_REGISTERED_CONTROL          = 0x0, NOTE_RC = 0x0, PNRC = 0x0,
			NOTE_ASSIGNABLE_CONTROL          = 0x1, NOTE_AC = 0x1, PNAC = 0x1,
			NOTE_MANAGEMENT                  = 0xF,
		};

	public:
		/*
			Get common fields.
				Query the opcode (0-15).
				Query the channel index (0-15).
				Query the combined group/channel index (0-255).
		*/
		uint8_t opcode()          const    {return (words[0]>>20) & 0xF;}
		uint8_t channel()         const    {return (words[0]>>16) & 0xF;}
		uint8_t groupAndChannel() const    {return (group()<<4u) & channel();}

		/*
			Classify opcode.
				perNote    : note on/off, poly pressure, per-note RC/AC/pitchbend and per-note management.
				controller : all controllers, pitch bend and pressure
				parameter  : channel registered & assignable controllers (aka registered/non-registered parameters)
		*/
		bool is_perNote   () const    {bool isPerNote[16] = {1,1,0,0,0,0,1,0,1,1,1,0,0,0,0,1}; return isPerNote[opcode()];}
		bool is_controller() const    {bool isControl[16] = {1,1,1,1,1,1,1,0,0,0,1,1,0,1,1,0}; return isControl[opcode()];}
		bool is_parameter () const    {bool isParam  [16] = {0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,0}; return isParam  [opcode()];}
		bool is_pressure  () const    {return opcode() == CHAN_PRESSURE || opcode() == NOTE_PRESSURE;}
		bool is_noteOn    () const    {return opcode() == NOTE_ON;}
		bool is_noteOff   () const    {return opcode() == NOTE_OFF;}
		bool is_noteOnOff () const    {return is_noteOn() || is_noteOff();}
		bool is_cc        () const    {return opcode() == CHAN_CC;}

		/*
			Get opcode-specific fields.
				noteNumber  : MIDI Note Number (0-127)
				cc_index    : Control Change index (0-127)
				param_bank  : Parameter bank, a.k.a. RPN/NRPN index MSB
				param_index : Parameter index, a.k.a. RPN/NRPN index LSB
		*/
		uint8_t noteNumber () const    {return is_perNote()   ? (words[0]>>8)&0x7F : 0xFF;}
		uint8_t cc_index   () const    {return is_cc()        ? (words[0]>>8)&0x7F : 0xFF;}
		uint8_t param_bank () const    {return is_parameter() ? (words[0]>>8)&0x7F : 0xFF;}
		uint8_t param_index() const    {return is_parameter() ? (words[0]   )&0x7F : 0xFF;}
	};

	/*
		Casting for MIDI 1.0 Channel Voice Messages.
	*/
	class UMP::Midi1_ChannelVoice : public UMP::ChannelVoice
	{
	public:
		using OPCODE = ChannelVoice::OPCODE;

		using velocity7_t    = uint8_t;
		using pressure7_t    = uint8_t;
		using index7_t       = uint8_t;
		using value7_t       = uint8_t;
		using pitchBend14_t =  int16_t;


	public:
		/*
			Construct channel voice messages.
				grpChan is a combined group/channel index (0-255).
		*/
		static UMP Note_On       (grpchan_t, notenum_t, velocity7_t = 0x40);
		static UMP Note_Off      (grpchan_t, notenum_t, velocity7_t = 0x40);
		static UMP Chan_Pressure (grpchan_t,            pressure7_t);
		static UMP Note_Pressure (grpchan_t, notenum_t, pressure7_t);
		static UMP Chan_CC       (grpchan_t, index7_t,  value7_t);
		static UMP Chan_PitchBend(grpchan_t,            pitchBend14_t = 0);

		/*
			Get common fields.
		*/
		uint8_t data_1() const    {return (words[0]>>20) & 0xF;}
		uint8_t data_2() const    {return (words[0]>>16) & 0xF;}


		/*
			Get type-specific fields
		*/
		uint8_t velocity() const    {return is_noteOnOff() ? data_2() : 0xFF;}
		uint8_t cc_value() const    {return is_cc       () ? data_2() : 0xFF;}
		uint8_t pressure() const    {switch (opcode()) {case CHAN_PRESSURE: return data_1(); case NOTE_PRESSURE: return data_2();} return 0xFF;}


	public:
		static word_t _word(uint8_t grpChan, OPCODE opcode, uint8_t data1, uint8_t data2 = 0);
	};

	/*
		Casting for MIDI 2.0 Channel Voice Messages.
	*/
	class UMP::Midi2_ChannelVoice : public UMP::ChannelVoice
	{
	public:
		using OPCODE = ChannelVoice::OPCODE;

		using articulation_type_t = uint8_t;
		using articulation16_t    = uint16_t;
		using velocity16_t        = uint16_t;
		using value32_t           = uint32_t;
		using pressure32_t        = uint32_t;
		using pitchBend32_t       = int32_t;
		using index7_t            = uint8_t;
		using index8_t            = uint8_t;


	public:
		/*
			Construct channel voice messages.
				grpChan is a combined group/channel index (0-255).
		*/
		static UMP Note_On       (grpchan_t, notenum_t, velocity16_t=0x7FFF, articulation_type_t=0, articulation16_t=0);
		static UMP Note_Off      (grpchan_t, notenum_t, velocity16_t=0x7FFF, articulation_type_t=0, articulation16_t=0);
		static UMP Chan_Pressure (grpchan_t,                                   pressure32_t);
		static UMP Note_Pressure (grpchan_t, notenum_t,                        pressure32_t);
		static UMP Chan_CC       (grpchan_t,            index7_t index,        value32_t);
		static UMP Chan_RC       (grpchan_t, index7_t idxMSB, index7_t idxLSB, value32_t);
		static UMP Chan_AC       (grpchan_t, index7_t idxMSB, index7_t idxLSB, value32_t);
		static UMP Note_RC       (grpchan_t, notenum_t, index8_t index,        value32_t);
		static UMP Note_AC       (grpchan_t, notenum_t, index8_t index,        value32_t);
		static UMP Chan_PitchBend(grpchan_t,                                   pitchBend32_t = 0);
		static UMP Note_PitchBend(grpchan_t, notenum_t,                        pitchBend32_t = 0);
		static UMP Note_Manage   (grpchan_t, notenum_t, uint8_t options);

		/*
			Shorthand per-note management messages:
				Detach note controllers from previous notes
				Reset note controllers
		*/
		static UMP Note_Detach    (grpchan_t, notenum_t);
		static UMP Note_Reset     (grpchan_t, notenum_t);

		/*
			Floating-point message functions
		*/
		static UMP Chan_PitchBend(grpchan_t,            float bend_value, float bend_range);
		static UMP Note_PitchBend(grpchan_t, notenum_t, float bend_value, float bend_range);


	public:
		// Get the first word of a 
		static word_t _word0_16(uint8_t grpChan, OPCODE opcode, uint16_t indexBits);
		static word_t _word0   (uint8_t grpChan, OPCODE opcode, uint8_t indexByte_1, uint8_t indexByte_2);
	};
}

/*
	********************************************************************
	***********        IMPLEMENTATION      *****************************
	********************************************************************
*/

namespace midi2
{
	inline uint32_t UMP::messageSize() const
	{
		static const uint8_t MT_WORD_SIZE[16] =
			{1, 1, 1, 2, 2, 4, 1, 1, 2, 2, 2, 3, 3, 4, 4, 4};

		return MT_WORD_SIZE[messageType()];
	}

	inline UMP::MESSAGE_TYPE UMP::identify(const Protocol &protocol) const
	{
		static const MESSAGE_TYPE R = RESERVED;
		static const MESSAGE_TYPE
			MT_IN_MIDI1  [] = {UTILITY, SYSTEM, MIDI1_CHANNEL_VOICE, DATA_8_BYTE, R,R,R,R, R,R,R,R, R,R,R,R},
			MT_IN_MIDI1_S[] = {UTILITY, SYSTEM, MIDI1_CHANNEL_VOICE, DATA_8_BYTE, R, DATA_16_BYTE, R,R, R,R,R,R, R,R,R,R},
			MT_IN_MIDI2  [] = {UTILITY, SYSTEM, R, DATA_8_BYTE, MIDI2_CHANNEL_VOICE, DATA_16_BYTE, R,R, R,R,R,R, R,R,R,R};

		switch (protocol.type)
		{
		case Protocol::MIDI_1_0: return (protocol.has_large_packets() ? MT_IN_MIDI1_S : MT_IN_MIDI1)[messageType()];
		case Protocol::MIDI_2_0: return MT_IN_MIDI2                                                 [messageType()];
		default: return RESERVED;
		}
	}


	inline UMP UMP::Utility::JR_Clock(group_t group, jr_time_t time)
	{
		uint32_t status =
			(uint32_t(UMP::UTILITY) << 28) |
			(uint32_t(group&0xF)    << 24) |
			(uint32_t(JR_CLOCK)     << 20) |
			//(uint32_t(grpChan&0x0F)  << 16) |
			uint32_t(time);
		return UMP(status);
	}
	inline UMP UMP::Utility::JR_TimeStamp(group_t group, jr_time_t time)
	{
		uint32_t status =
			(uint32_t(UMP::UTILITY) << 28) |
			(uint32_t(group&0xF)    << 24) |
			(uint32_t(JR_TIMESTAMP) << 20) |
			//(uint32_t(grpChan&0x0F)    << 16) |
			uint32_t(time);
		return UMP(status);
	}


	inline uint32_t UMP::CV1::_word(uint8_t grpChan, OPCODE opcode, uint8_t data1, uint8_t data2)
	{
		return (uint32_t(UMP::MIDI1_VOICE) << 28)
			|  (uint32_t(grpChan&0xF0)     << 20)
			| ((uint32_t(opcode)&0x0F)     << 20)
			|  (uint32_t(grpChan&0x0F)     << 16)
			|  (uint32_t(data1  &0x7F)     <<  8)
			|   uint32_t(data2  &0x7F);
	}

	inline UMP UMP::CV1::Note_On(grpchan_t grpChan, notenum_t note, velocity7_t velocity)
	{
		return UMP(_word(grpChan, NOTE_ON, note&0x7F, velocity&0x7F));
	}
	inline UMP UMP::CV1::Note_Off(grpchan_t grpChan, notenum_t note, velocity7_t velocity)
	{
		return UMP(_word(grpChan, NOTE_OFF, note&0x7F, velocity&0x7F));
	}
	inline UMP UMP::CV1::Chan_Pressure(grpchan_t grpChan, pressure7_t pressure)
	{
		return UMP(_word(grpChan, CHAN_PRESSURE, pressure&0x7F, 0));
	}
	inline UMP UMP::CV1::Note_Pressure(grpchan_t grpChan, notenum_t note, pressure7_t pressure)
	{
		return UMP(_word(grpChan, NOTE_PRESSURE, note&0x7F, pressure&0x7F));
	}
	inline UMP UMP::CV1::Chan_CC(grpchan_t grpChan, index7_t index, value7_t value)
	{
		return UMP(_word(grpChan, CHAN_CONTROL, index&0x7F, value&0x7F));
	}
	inline UMP UMP::CV1::Chan_PitchBend(grpchan_t grpChan, pitchBend14_t bend)
	{
		return UMP(_word(grpChan, CHAN_CONTROL, bend&0x7F, ((bend^0x2000)>>7)&0x7F));
	}


	inline uint32_t UMP::CV2::_word0_16(uint8_t grpChan, OPCODE opcode, uint16_t indexBits)
	{
		return (uint32_t(UMP::MIDI2_VOICE) << 28)
			|  (uint32_t(grpChan&0xF0)     << 20)
			| ((uint32_t(opcode)&0x0F)     << 20)
			|  (uint32_t(grpChan&0x0F)     << 16)
			|   uint32_t(indexBits);
	}
	inline uint32_t UMP::CV2::_word0(uint8_t grpChan, OPCODE opcode, uint8_t index_1, uint8_t index_2)
	{
		return _word0_16(grpChan, opcode, (index_1 << uint16_t(8)) | uint16_t(index_2));
	}


	inline UMP UMP::CV2::Note_On(
		uint8_t grpChan, notenum_t note, uint16_t vel, uint8_t attType, uint16_t attrib)
	{
		return UMP(
			_word0(grpChan, NOTE_ON, note, attType),
			(uint32_t(vel)<<16) | uint32_t(attrib)
		);
	}
	inline UMP UMP::CV2::Note_Off(
		uint8_t grpChan, notenum_t note, uint16_t vel, uint8_t attType, uint16_t attrib)
	{
		return UMP(
			_word0(grpChan, NOTE_OFF, note, attType),
			(uint32_t(vel)<<16) | uint32_t(attrib)
		);
	}

	inline UMP UMP::CV2::Note_Manage(grpchan_t grpChan, notenum_t note, uint8_t options)
	{
		return UMP(_word0(grpChan, NOTE_MANAGEMENT, note, options), 0);
	}

	inline UMP UMP::CV2::Chan_Pressure(uint8_t grpChan, uint32_t z)
	{
		return UMP(
			_word0(grpChan, CHAN_PRESSURE, 0, 0),
			z
		);
	}
	inline UMP UMP::CV2::Note_Pressure(uint8_t grpChan, uint8_t note, uint32_t z)
	{
		return UMP(
			_word0(grpChan, NOTE_PRESSURE, note, 0),
			z
		);
	}

	inline UMP UMP::CV2::Chan_PitchBend(uint8_t grpChan, int32_t value)
	{
		return UMP(
			_word0(grpChan, CHAN_PITCH_BEND, 0, 0),
			uint32_t(value) ^ 0x80000000
		);
	}

	inline UMP UMP::CV2::Note_PitchBend(uint8_t grpChan, uint8_t note, int32_t value)
	{
		return UMP(
			_word0(grpChan, NOTE_PITCH_BEND, 0, 0),
			uint32_t(value) ^ 0x80000000
		);
	}
	/*Midi2Packet Midi2Packet::PitchBend_ST(uint8_t grpChan, float  tones, float range)
	{
	int32_t amt = int32_t(std::min(std::max(((float(0x7FFFFFFF)*tones)/range),
	-float(0x80000000)), float(0x7FFFFFFF)));
	return (grpChan, amt);
	}*/

	inline UMP UMP::CV2::Chan_CC(
		uint8_t grpChan, uint8_t cc, uint32_t value)
	{
		return UMP(
			_word0(grpChan, CHAN_CONTROL, cc&0x7F, 0),
			value
		);
	}

	inline UMP UMP::CV2::Chan_RC(
		uint8_t grpChan, uint8_t indexMSB, uint8_t indexLSB, word_t value)
	{
		return UMP(
			_word0(grpChan, CHAN_REGISTERED_CONTROL,
				indexMSB&0x7F, indexLSB&0x7F),
			value
		);
	}
	inline UMP UMP::CV2::Chan_AC(
		uint8_t grpChan, uint8_t indexMSB, uint8_t indexLSB, word_t value)
	{
		return UMP(
			_word0(grpChan, CHAN_ASSIGNABLE_CONTROL,
				indexMSB&0x7F, indexLSB&0x7F),
			value
		);
	}
	inline UMP UMP::CV2::Note_RC(
		uint8_t grpChan, notenum_t note, uint8_t index, word_t value)
	{
		return UMP(
			_word0(grpChan, NOTE_REGISTERED_CONTROL,
				note, index),
			value
		);
	}
	inline UMP UMP::CV2::Note_AC(
		uint8_t grpChan, notenum_t note, uint8_t index, word_t value)
	{
		return UMP(
			_word0(grpChan, NOTE_ASSIGNABLE_CONTROL,
				note, index),
			value
		);
	}

	inline UMP UMP::CV2::Chan_PitchBend(uint8_t grpChan, float tones, float range)
	{
		return Chan_PitchBend(grpChan,
			std::min<int32_t>(std::max<int32_t>(
				float(0x7FFFFFFF)*tones/range,
				-(1<<31)), 0x7FFFFFFF));
	}

	inline UMP UMP::CV2::Note_PitchBend(uint8_t grpChan, notenum_t note, float tones, float range)
	{
		return Note_PitchBend(grpChan, note,
			std::min<int32_t>(std::max<int32_t>(
				float(0x7FFFFFFF)*tones/range,
				-(1<<31)), 0x7FFFFFFF));
	}


	inline UMP UMP::CV2::Note_Detach(grpchan_t grpChan, notenum_t note)
		{return Note_Manage(grpChan, note, 0x2);}
	inline UMP UMP::CV2::Note_Reset (grpchan_t grpChan, notenum_t note)
		{return Note_Manage(grpChan, note, 0x1);}
}
