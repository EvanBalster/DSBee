#pragma once


#include "universal_sysex.h"


namespace midi2
{
	enum
	{
		CI_VERSION_IMPL = 0x01,

		CI_CHANNEL_ALL = 0x7F,
	};

	/*
		MIDI-CI Unique Identifier: identifies sources & destinations.
			A little-endian, unsigned value.
			Usually either randomly-generated or set to broadcast.
	*/
	struct MUID : public sysex::UInt28
	{
	public:
		using UInt28 = sysex::UInt28;

		enum
		{
			INVALID       = 0xF0000000,
			BROADCAST     = 0x0FFFFFFF,
			SPECIAL_BEGIN = 0x0FFFFF00,
		};

	public:
		using UInt28::value;

	public:
		MUID()                  : UInt28(INVALID) {}
		MUID(uint32_t value)    : UInt28(value) {}

		// Broadcast MUIDs
		static MUID Broadcast()          {return MUID(BROADCAST);}
		bool        broadcast() const    {return value == BROADCAST;}

		// Is this a special MUID?
		bool        special() const    {return value >= SPECIAL_BEGIN;}

		// Validity check
		bool valid() const;

		using UInt28::read;
		using UInt28::write;
	};

	/*
		MIDI-CI identity (used in Discovery messages).
	*/
	struct CI_Identity
	{
		// Peer device identification
		sysex::Data3x7 sysExId  = 0xFF'FF'FF;    // data777
		sysex::Data2x7 family   = 0x00'00;       // data77
		sysex::Data2x7 model    = 0x00'00;       // data77
		sysex::Data4x7 revision = 0x00'00'00'00; // data7777

		bool valid() const                        {return sysExId.valid() && family.valid() && model.valid() && revision.valid();}

		// Read & write
		bool read (SysEx_Reader &reader)          {return reader >> sysExId >> family >> model >> revision;}
		bool write(SysEx_Writer &writer) const    {return writer << sysExId << family << model << revision;}
	};

	/*
		Base type for MIDI-CI messages.
	*/
	struct CI_Message
	{
	public:
		using UInt7  = sysex::UInt7;
		using UInt14 = sysex::UInt14;
		using UInt21 = sysex::UInt21;
		using UInt28 = sysex::UInt28;

		// Types
		enum CI_TYPE : uint8_t
		{
			TYPE_NONE = 0,

			CATEGORY_PROT = 0x10, // Protocol Negotiation
			CATEGORY_PROF = 0x20, // Profile Configuration
			CATEGORY_PROP = 0x30, // Property Exchange
			CATEGORY_MGMT = 0x70, // Management

			PROT_INIT       = 0x10,
			PROT_INIT_REPLY = 0x11,
			PROT_SET        = 0x12,
			PROT_TEST_I2R   = 0x13,
			PROT_TEST_R2I   = 0x14,
			PROT_CONFIRM    = 0x15,

			PROF_INQUIRY       = 0x20,
			PROF_INQUIRY_REPLY = 0x21,
			PROF_SET_ON        = 0x22,
			PROF_SET_OFF       = 0x23,
			PROF_ENABLED       = 0x24,
			PROF_DISABLED      = 0x25,
			PROF_SPECIFIC      = 0x2F,

			PROP_CAPS_INQUIRY = 0x30,
			PROP_CAPS_REPLY   = 0x31,
			PROP_HAS_INQUIRY  = 0x32,
			PROP_HAS_REPLY    = 0x33,
			PROP_GET_INQUIRY  = 0x34,
			PROP_GET_REPLY    = 0x35,
			PROP_SET_INQUIRY  = 0x36,
			PROP_SET_REPLY    = 0x37,
			PROP_SUBSCRIPTION       = 0x38,
			PROP_SUBSCRIPTION_REPLY = 0x39,
			PROP_NOTIFY             = 0x3F,

			MGMT_DISCOVERY       = 0x70,
			MGMT_DISCOVERY_REPLY = 0x71,
			MGMT_INVALIDATE      = 0x72,
			MGMT_NAK             = 0x7F,
		};

	public:
		/*	
			Every MIDI-CI message includes a source and destination MUID.
		*/
		struct Addressing
		{
		public:
			// Source & destination.  By default these are both invalid.
			MUID    source;
			MUID    destination;

		public:
			// Default constructor (invalid)
			Addressing()                                    {}
			Addressing(MUID _source, MUID _destination)     : source(_source), destination(_destination) {}

			// Validity
			bool valid() const    {return source.valid() && destination.valid();}

			// Read & write
			bool read (SysEx_Reader &reader)          {return (reader >> source.value >> destination.value) && valid();}
			bool write(SysEx_Writer &writer) const    {return (writer << source.value << destination.value);}
		};

		/*
			Base type for MIDI-CI messages.
		*/
		struct Base : public UniversalSysEx::Base
		{
		public:
			using UniversalSysEx::Base::sysExId;
			using UniversalSysEx::Base::deviceId;
			using UniversalSysEx::Base::subId1;
			using UniversalSysEx::Base::subId2;

			UInt7      ci_version = CI_VERSION_IMPL; // MIDI-CI message version/format
			Addressing addressing;                   // Source & destination MUID

		public:
			Base() {}
			Base(
				UInt7 ci_type, UInt7 ci_channel,
				MUID   source, MUID  destination);

			/*
				Is the base part of this MIDI-CI message valid?
			*/
			bool valid() const;

			// CI-specific interpretations for universal SysEx fields
			uint8_t category      () const    {return this->subId2 & 0xF0;}
			UInt7   ci_type       () const    {return this->subId2;}
			UInt7  &ci_type       ()          {return this->subId2;}
			UInt7   ci_channel    () const    {return this->deviceId;}
			UInt7  &ci_channel    ()          {return this->deviceId;}
			bool    ci_channel_all() const    {return this->deviceId == CI_CHANNEL_ALL;}

			// Access MUIDs
			MUID  source()      const    {return addressing.source;}
			MUID &source()               {return addressing.source;}
			MUID  destination() const    {return addressing.destination;}
			MUID &destination()          {return addressing.destination;}

			// Read & write
			bool read (SysEx_Reader &reader);
			bool write(SysEx_Writer &writer) const;
		};

		/*
			NAK message.
		*/
		struct NAK : public Base
		{
		public:
			NAK() {}
			NAK(UInt7 ci_channel, MUID source, MUID desination)    : Base(MGMT_NAK, ci_channel, source, desination) {}

			bool valid() const    {return Base::valid() && ci_type() == MGMT_NAK;}

			using Base::read;
			using Base::write;
		};

		/*
			Discovery or Reply to Discovery message.
		*/
		struct Discovery : public Base
		{
		public:
			CI_Identity identity;

		public:
			Discovery(bool is_reply, MUID source, MUID desination, CI_Identity _identity) :
				Base(is_reply ? MGMT_DISCOVERY_REPLY : MGMT_DISCOVERY, CI_CHANNEL_ALL, source, desination),
				identity(_identity) {}

			bool valid() const    {return Base::valid() && ci_channel() == CI_CHANNEL_ALL && identity.valid();}

			using Base::read;
			using Base::write;
		};

		/*
			Protocol negotiation messages always include an authority level...
		*/
		struct ProtocolNegotiation : public Base
		{
		public:
			sysex::UInt7 authority_level = 0xFF;

		public:
			bool valid() const                        {return Base::valid() && authority_level.valid();}

			// Read & write
			bool read (SysEx_Reader &reader)          {return Base::read (reader) & (reader >> authority_level);}
			bool write(SysEx_Writer &writer) const    {return Base::write(writer) & (writer << authority_level);}
		};


		/*
			Prope
		*/
		struct PropertyChunk : public Base
		{
		public:
			sysex::UInt7 authority_level = 0xFF;

		public:
			bool valid() const                        {return Base::valid() && authority_level.valid();}

			// Read & write
			bool read (SysEx_Reader &reader)          {return Base::read (reader) & (reader >> authority_level);}
			bool write(SysEx_Writer &writer) const    {return Base::write(writer) & (writer << authority_level);}
		};
	};
	
}

/*
	********************************************************************
	***********        IMPLEMENTATION      *****************************
	********************************************************************
*/

namespace midi2
{
	inline bool MUID::valid() const
	{
		// TODO how should receivers react to reserved MUIDs?
		return UInt28::valid()
			&& !(value & INVALID)
			&&  (value < SPECIAL_BEGIN || value == BROADCAST);
	}

	inline CI_Message::Base::Base(UInt7 ci_type, UInt7 ci_channel, MUID source, MUID destination) :
		UniversalSysEx::Base(
			sysex::SYSEX_ID_UNIVERSAL,
			ci_channel,
			sysex::SYSEX_SUBID1_MIDI_CI,
			ci_type),
		addressing(source, destination)
	{

	}

	inline bool CI_Message::Base::valid() const
	{
		return UniversalSysEx::Base::valid()
			&& sysExId == sysex::SYSEX_ID_UNIVERSAL
			&& subId1  == sysex::SYSEX_SUBID1_MIDI_CI
			&& (this->deviceId < 16 || this->deviceId == CI_CHANNEL_ALL)
			&& ci_version == CI_VERSION_IMPL //.valid()  // TODO specs are unclear about what to do here
			&& addressing.valid();
	}

	inline bool CI_Message::Base::read(SysEx_Reader &reader)
	{
		using namespace sysex;
		return UniversalSysEx::Base::read(reader)
			&  (reader >> ci_version)
			&  addressing.read(reader)
			&& valid();
	}
	inline bool CI_Message::Base::write(SysEx_Writer &writer) const
	{
		using namespace sysex;
		return UniversalSysEx::Base::write(writer)
			&  (writer << ci_version)
			&  addressing.write(writer);
	}
}
