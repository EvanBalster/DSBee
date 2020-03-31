#pragma once


#include "sysex.h"


namespace midi2
{
	struct UniversalSysEx
	{
	public:
		using UInt7  = sysex::UInt7;
		using UInt14 = sysex::UInt14;
		using UInt21 = sysex::UInt21;
		using UInt28 = sysex::UInt28;

		/*
			Base type for Universal SysEx messages.
		*/
		struct Base
		{
		public:
			UInt7
				sysExId  = 0xFF,
				deviceId = 0xFF,
				subId1   = 0xFF,
				subId2   = 0xFF;

			bool valid() const    {return sysExId.valid() && deviceId.valid() && subId1.valid() && subId2.valid();}

		public:
			Base() {}
			Base(
				sysex::UInt7 _sysExId, sysex::UInt7 _deviceId,
				sysex::UInt7 _subId1,  sysex::UInt7 _subId2) :
				sysExId(_sysExId), deviceId(_deviceId),
				subId1 (_subId1),  subId2  (_subId2) {}

			// Read & write
			bool read (SysEx_Reader &reader)          {return (reader >> sysExId >> deviceId >> subId1 >> subId2) && valid();}
			bool write(SysEx_Writer &writer) const    {return (writer << sysExId << deviceId << subId1 << subId2);}
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
}
