#pragma once


#include <cstdint>


namespace midi2
{
	namespace sysex
	{
		using byte_t = uint8_t;

		/*
			Template for integral fields used in system exclusive messages.
			Arguments:
				T         -- integer representation
				ByteMask  -- mask of valid bits in original bytes; used to catch 7-bit errors
				FieldMask -- mask of valid bits, prior to shift (in integer representation)
				Shifts    -- shift used when reading or writing
		*/
		template<bool IsSysex7, typename T, T FieldMask, T ... Shifts>
		struct IntegralField_
		{
		public:
			static const byte_t BYTE_MASK = (IsSysex7 ? 0x7F : 0xFF);
			static const T      FIELD_MASK = FieldMask;
			static const size_t BYTE_SIZE = sizeof...(Shifts);
			static const bool   IS_7_BIT  = IsSysex7;

			static_assert(BYTE_SIZE > 0, "Integral Field must serialize to at least 1 byte.");

		public:
			T value;

		public:
			// Assign from value type
			IntegralField_           (const T v)    : value(v) {}
			IntegralField_& operator=(const T v)    {value = v; return *this;}

			// Convert to value type
			operator T() const    {return value;}

			// Is the value valid?
			bool valid() const    {return (value&FIELD_MASK) == value;}

			/*
				Read and write operations.
			*/
			bool write(byte_t *data) const
			{
				static const T sh[] = {Shifts...};
				T safe = value & FIELD_MASK;
				for (size_t i = 0; i < BYTE_SIZE; ++i) data[i] = (safe >> sh[i]);
				return value == safe;
			}
			bool read(const byte_t *data)
			{
				static const T sh[] = {Shifts...};
				value = 0;
				byte_t verify = 0;
				for (size_t i = 0; i < BYTE_SIZE; ++i)
				{
					verify |= data[i];
					value |= T(data[i]&BYTE_MASK) << sh[i];
				}
				return !(verify & ~BYTE_MASK) && (value & FIELD_MASK) == value;
			}

			bool read_noByteCheck(const byte_t *data)
			{
				static const T sh[] = {Shifts...};
				value = 0;
				for (size_t i = 0; i < BYTE_SIZE; ++i) value |= T(data[i]&BYTE_MASK) << sh[i];
				return (value & FIELD_MASK) == value;
			}

#if 0
			/*
				Various operators
			*/
			operator bool()  const    {return value;}
			bool operator!() const    {return !value;}

#define MIDI_BINARY_OPERATOR(OP) IntegralField_  operator OP (T v) const    {return value OP  v;} \
/* ... */                        IntegralField_& operator OP=(T v) const    {value OP= v; return *this;}
#define MIDI_UNARY_OPERATOR(OP)  IntegralField_  operator OP ()    const    {return OP value;}
#define MIDI_COMPARATOR(OP)      bool            operator OP (T v) const    {return value OP v;}

			MIDI_UNARY_OPERATOR(~)
			MIDI_BINARY_OPERATOR(&)
			MIDI_BINARY_OPERATOR(|)
			MIDI_BINARY_OPERATOR(^)
			MIDI_BINARY_OPERATOR(+)
			MIDI_BINARY_OPERATOR(-)
			MIDI_BINARY_OPERATOR(*)
			MIDI_BINARY_OPERATOR(/)
			MIDI_COMPARATOR(==)
			MIDI_COMPARATOR(!=)
			MIDI_COMPARATOR(<)
			MIDI_COMPARATOR(<=)
			MIDI_COMPARATOR(>)
			MIDI_COMPARATOR(>=)

#undef MIDI_BINARY_OPERATOR
#undef MIDI_UNARY_OPERATOR
#undef MIDI_COMPARATOR
#endif

			// Fault-tolerant read
			/*T read(const byte_t *data)
			{
				static const T sh[] = {Shifts...};
				T val = 0;
				for (size_t i = 0; i < BYTE_SIZE; ++i) val |= T(data[i]) << sh[i];
				return val;
			}*/
		};


		using UInt28  = IntegralField_<1, uint32_t, 0x0FFFFFFF, 0, 7, 14, 21>;
		using UInt21  = IntegralField_<1, uint32_t,   0x1FFFFF, 0, 7, 14>;
		using UInt14  = IntegralField_<1, uint32_t,     0x3FFF, 0, 7>;
		using UInt7   = IntegralField_<1, uint8_t,        0x7F, 0>;
		using Data1x7 = IntegralField_<1, uint8_t,        0x7F, 0>;
		using Data2x7 = IntegralField_<1, uint16_t,     0x7F7F, 0, 8>;
		using Data3x7 = IntegralField_<1, uint32_t,   0x7F7F7F, 0, 8, 16>;
		using Data4x7 = IntegralField_<1, uint32_t, 0x7F7F7F7F, 0, 8, 16, 24>;
	}
}