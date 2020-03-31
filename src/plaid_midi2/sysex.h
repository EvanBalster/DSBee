#pragma once



#include <cstdint>

#include "sysex_fields.h"



namespace midi2
{
	namespace sysex
	{
		enum
		{
			SYSEX_ID_UNIVERSAL   = 0x7E,

			SYSEX_SUBID1_MIDI_CI = 0x0D,
		};
	}

	/*
		A buffer of bytes for writing into.
	*/
	struct Byte_Buffer
	{
		uint8_t *bytes;
		size_t   capacity;
	};

	/*
		This structure is used to represent SysEx-7 and SysEx-8 messages.
	*/
	struct SysEx_Message
	{
	public:
		const uint8_t *bytes;  // Byte vector, not including MIDI 1.0's F0 and F7
		size_t         length; // Length in bytes.
	};

	/*
		Base class for SysEx reader and writer.
	*/
	class SysEx_IO
	{
	public:
		using byte_t = uint8_t;

		enum READ_FLAGS
		{
			DEFAULT       = 0x00,
			SYSEX7        = 0x80,            // High bit produces an error.
			SYSEX8        = 0x80,            // (no effect)
			//LITTLE_ENDIAN = 0x00, LE = 0x00, // (no effect)
			//BIG_ENDIAN    = 0x01, BE = 0x01, // Currently unsupported
		};

		enum FAIL_STATE
		{
			FAIL_OVERRUN = 0x01, // Overran end of message buffer.
			FAIL_INVALID = 0x40, // Encountered an invalid value of some kind.
			FAIL_7BIT    = 0x80, // Encountered a set high bit in 7-bit data.
		};

	public:
		// Common fields
		uint32_t      fail_state = 0;
		const byte_t *fail_pos   = nullptr;

	public:
		/*
			Check for fail conditions.
				fail -- an attempt to read some data failed (inspect fail_state)
		*/
		bool failed() const    {return fail_state != 0;}

		/*
			Register a failure.
		*/
		void fail_at(const byte_t *pos, FAIL_STATE type, const char *description)
		{
			fail_state |= type;
			fail_pos = pos;
		}

	public:
		/*
			Read and write common data formats.
		*/
		/*static uint32_t Read_u28  (const byte_t *data);
		static bool    Write_u28  (      byte_t *data, uint32_t val);
		static uint32_t Read_u21  (const byte_t *data);
		static bool    Write_u21  (      byte_t *data, uint32_t val);
		static uint16_t Read_u14  (const byte_t *data);
		static bool    Write_u14  (      byte_t *data, uint16_t val);
		static uint8_t  Read_u7   (const byte_t *data);
		static bool    Write_u7   (      byte_t *data, uint8_t  val);
		static uint16_t Read_d77  (const byte_t *data);
		static bool    Write_d77  (      byte_t *data, uint16_t val);
		static uint32_t Read_d777 (const byte_t *data);
		static bool    Write_d777 (      byte_t *data, uint32_t val);
		static uint32_t Read_d7777(const byte_t *data);
		static bool    Write_d7777(      byte_t *data, uint32_t val);*/
	};

	/*
		Class for reading System Exclusive messages.
	*/
	class SysEx_Reader : public SysEx_IO
	{
	public:
		const uint8_t *pos, *end;

	public:
		SysEx_Reader(const SysEx_Message &message)    : pos(message.bytes), end(message.bytes + message.length) {}

		/*
			Check for stop conditions
				eof  -- we have reached
				
		*/
		bool eof () const    {return pos >= end;}

		/*
			Read bytes.
				read<N> or read(n) returns a pointer to the next N bytes (if available).
				read<N>(v) or read(v,n) copies the bytes into v, returning whether successful.
			READ_FLAGS control diagnostic options:
				SYSEX7 : error if the high bit is set.  (use read7 as shorthand for this)
		*/
		template<size_t N, int FLAGS=0> const uint8_t* read ();
		template<          int FLAGS=0> const uint8_t* read (            size_t n);
		template<size_t N, int FLAGS=0> const uint8_t* read7()                        {return read<N,FLAGS|SYSEX7>();}
		template<          int FLAGS=0> const uint8_t* read7(            size_t n)    {return read<  FLAGS|SYSEX7>(n);}
		template<size_t N, int FLAGS=0> bool           read (uint8_t *v);
		template<          int FLAGS=0> bool           read (uint8_t *v, size_t n);
		template<size_t N, int FLAGS=0> bool           read7(uint8_t *v)              {return pull<N,FLAGS|SYSEX7>(v);}
		template<          int FLAGS=0> bool           read7(uint8_t *v, size_t n)    {return pull<  FLAGS|SYSEX7>(v,n);}

		/*
			Read common number formats:
				u## : unsigned numbers encoded as little-endian SysEx-7 bytes.
				d## : multi-byte data stored as a little-endian unsigned integer.  Bits 7, 14, 21 etc are 0.
		*/
		/*bool read_u28  (uint32_t &v)    {if (auto p = read7<4>()) {v = SysEx_IO::Read_u28(p);   return 1;} else return 0;}
		bool read_u21  (uint32_t &v)    {if (auto p = read7<3>()) {v = SysEx_IO::Read_u28(p);   return 1;} else return 0;}
		bool read_u14  (uint16_t &v)    {if (auto p = read7<2>()) {v = SysEx_IO::Read_u28(p);   return 1;} else return 0;}
		bool read_u7   (uint8_t  &v)    {if (auto p = read7<1>()) {v = SysEx_IO::Read_u28(p);   return 1;} else return 0;}
		bool read_d77  (uint16_t &v)    {if (auto p = read7<2>()) {v = SysEx_IO::Read_d77(p);   return 1;} else return 0;}
		bool read_d777 (uint32_t &v)    {if (auto p = read7<3>()) {v = SysEx_IO::Read_d777(p);  return 1;} else return 0;}
		bool read_d7777(uint32_t &v)    {if (auto p = read7<4>()) {v = SysEx_IO::Read_d7777(p); return 1;} else return 0;}*/

		/*
			Read a SysEx field.
		*/
		template<typename SysExField>
		bool read(SysExField &field);

		/*
			iostream-style reading.
		*/
		class Operation;

		template<typename SysExField>
		Operation operator>>(SysExField &field)    {Operation op = {*this,false}; op >> field; return op;}

	public:
		// Helper for iostream-style reading.
		class Operation
		{
		public:
			SysEx_Reader &reader;
			bool          fail;

		public:
			// Conversion to bool indicates whether successful.
			operator bool() const    {return !fail;}

			// iostream-style writing.
			template<typename SysExField>
			Operation &operator>>(const SysExField &field)    {fail &= !reader.read(field); return *this;}
		};
	};

	/*
		Class for writing System Exclusive messages.
	*/
	class SysEx_Writer : public SysEx_IO
	{
	public:
		uint8_t *pos, *limit;

	public:
		SysEx_Writer(const Byte_Buffer &buffer)    : pos(buffer.bytes), limit(buffer.bytes + buffer.capacity) {}

		/*
			Check for stop conditions.
				full -- we have used up the buffer.
				fail -- an attempt to write some data failed (inspect fail_state)
		*/
		bool full() const    {return pos >= limit;}

		/*
			Write data to the buffer.
				writebuf<N> and writebuf(n) fetch the next n bytes for writing into.
				write<N>(v) and write(v,n) copy the bytes from v, returning whether successful.
			FLAGS (from SysEx_IO) control diagnostic options:
				SYSEX7 : error if the high bit is set. (use write7 as shorthand for this)
		*/
		template<size_t N>              uint8_t* writebuf();
		/* ... */                       uint8_t* writebuf(                size_t n);
		template<size_t N, int FLAGS=0> bool     write (const uint8_t *v);
		template<          int FLAGS=0> bool     write (const uint8_t *v, size_t n);
		template<size_t N, int FLAGS=0> bool     write7(const uint8_t *v)              {write<N,FLAGS|>(data,n);}
		template<          int FLAGS=0> bool     write7(const uint8_t *v, size_t n)    {write<  FLAGS|>(data,n);}

		/*
			Write common number formats:
				u## : unsigned numbers encoded as little-endian SysEx-7 bytes.
				d## : multi-byte data stored as a little-endian unsigned integer.  Bits 7, 14, 21 etc are 0.
		*/
		/*bool write_u28  (uint32_t v)    {return write_<4>(v, Write_u28);}
		bool write_u21  (uint32_t v)    {return write_<3>(v, Write_u21);}
		bool write_u14  (uint16_t v)    {return write_<2>(v, Write_u14);}
		bool write_u7   (uint8_t  v)    {return write_<1>(v, Write_u7);}
		bool write_d77  (uint16_t v)    {return write_<2>(v, Write_d77);}
		bool write_d777 (uint32_t v)    {return write_<3>(v, Write_d777);}
		bool write_d7777(uint32_t v)    {return write_<4>(v, Write_d7777);}
		template<size_t N, typename T> bool write_(T val, bool (*write_func)(byte_t*,T));*/

		/*
			Write a sysex field.
		*/
		template<typename SysExField>
		bool write(const SysExField&);

		/*
			iostream-style writing.
		*/
		class Operation;

		template<typename SysExField>
		Operation operator<<(const SysExField &field)    {Operation op = {*this,false}; op << field; return op;}

	public:
		// Helper for iostream-style writing.
		class Operation
		{
		public:
			SysEx_Writer &writer;
			bool          fail;

		public:
			// Conversion to bool indicates whether successful.
			operator bool() const    {return !fail;}

			// iostream-style writing.
			template<typename SysExField>
			Operation &operator<<(const SysExField &field)    {fail &= !writer.write(field); return *this;}
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
	/*inline bool SysEx_IO::Write_u28(byte_t *data, uint32_t val)
	{
		data[0] = (val >> 21) & 0x7F;
		data[1] = (val >> 14) & 0x7F;
		data[2] = (val >>  7) & 0x7F;
		data[3] = (val      ) & 0x7F;
		return !(val & 0xF0000000);
	}
	inline uint32_t SysEx_IO::Read_u28(const byte_t *data)
	{
		return
			(uint32_t(data[0]&0x7F) << 21) |
			(uint32_t(data[1]&0x7F) << 14) |
			(uint32_t(data[2]&0x7F) <<  7) |
			(uint32_t(data[3]&0x7F) );
	}
	inline bool SysEx_IO::Write_u21(byte_t *data, uint32_t val)
	{
		data[0] = (val >> 14) & 0x7F;
		data[1] = (val >>  7) & 0x7F;
		data[2] = (val      ) & 0x7F;
		return !(val & 0xE000);
	}
	inline uint32_t SysEx_IO::Read_u21(const byte_t *data)
	{
		return
			(uint32_t(data[0]&0x7F) << 14) |
			(uint32_t(data[1]&0x7F) <<  7) |
			(uint32_t(data[2]&0x7F) );
	}
	inline bool SysEx_IO::Write_u14(byte_t *data, uint16_t val)
	{
		data[0] = (val >> 7) & 0x7F;
		data[1] = (val     ) & 0x7F;
		return !(val & 0xC0);
	}
	inline uint16_t SysEx_IO::Read_u14(const byte_t *data)
	{
		return
			(uint16_t(data[0]&0x7F) << 7) |
			(uint16_t(data[1]&0x7F) );
	}
	inline bool SysEx_IO::Write_u7(byte_t *data, uint8_t val)
	{
		data[0] = val & 0x7F;
		return !(val & 0x80);
	}
	inline uint8_t SysEx_IO::Read_u7(const byte_t *data)
	{
		return uint8_t(data[0]&0x7F);
	}
	inline bool SysEx_IO::Write_d77(byte_t *data, uint16_t val)
	{
		data[0] = (val >> 8) & 0x7F;
		data[1] = (val     ) & 0x7F;
		return !(val & 0x8080);
	}
	inline uint16_t SysEx_IO::Read_d77(const byte_t *data)
	{
		return
			(uint16_t(data[0]&0x7F) << 8) |
			(uint16_t(data[1]&0x7F) );
	}
	inline bool SysEx_IO::Write_d777(byte_t *data, uint32_t val)
	{
		data[0] = (val >> 16) & 0x7F;
		data[1] = (val >>  8) & 0x7F;
		data[2] = (val      ) & 0x7F;
		return !(val & 0xFF808080);
	}
	inline uint32_t SysEx_IO::Read_d777(const byte_t *data)
	{
		return
			(uint32_t(data[0]&0x7F) << 16) |
			(uint32_t(data[1]&0x7F) <<  8) |
			(uint32_t(data[2]&0x7F) );
	}
	inline bool SysEx_IO::Write_d7777(byte_t *data, uint32_t val)
	{
		data[0] = (val >> 24) & 0x7F;
		data[1] = (val >> 16) & 0x7F;
		data[2] = (val >>  8) & 0x7F;
		data[3] = (val      ) & 0x7F;
		return !(val & 0x80808080);
	}
	inline uint32_t SysEx_IO::Read_d7777(const byte_t *data)
	{
		return
			(uint32_t(data[0]&0x7F) << 24) |
			(uint32_t(data[1]&0x7F) << 16) |
			(uint32_t(data[2]&0x7F) <<  8) |
			(uint32_t(data[3]&0x7F) );
	}*/


	template<size_t N, int FLAGS>
	inline const uint8_t* SysEx_Reader::read()
	{
		const uint8_t *p = pos; pos += N;
		if (pos > end)
		{
			fail_at(p, FAIL_OVERRUN, "not enough bytes remaining");
			return nullptr;
		}

		if (FLAGS & SYSEX7) for (size_t i=0; i<N; ++i)
		{
			if (p[i] & 0x80)
			{
				fail_at(p+i, FAIL_7BIT, "illegal high bit during SysEx7 read");
				return nullptr;
			}
		}
		return p;
	}
	template<int FLAGS>
	const uint8_t* SysEx_Reader::read(size_t n)
	{
		const uint8_t *p = pos; pos += n;
		if (pos > end)
		{
			fail_at(p, FAIL_OVERRUN, "not enough bytes remaining");
			return nullptr;
		}

		if (FLAGS & SYSEX7) for (size_t i=0; i<n; ++i)
		{
			if (p[i] & 0x80)
			{
				fail_at(p+i, FAIL_7BIT, "illegal high bit during SysEx7 read");
				return nullptr;
			}
		}
		return p;
	}
	template<size_t N, int FLAGS>
	inline bool SysEx_Reader::read(uint8_t *v)
	{
		if (const uint8_t *p = read<N,FLAGS>())
		{
			for (size_t i=0; i<N; ++i) v[i] = p[i];
			return true;
		}
		else return false;
	}
	
	template<int FLAGS>
	bool SysEx_Reader::read(uint8_t *v, size_t n)
	{
		if (const uint8_t *p = read<FLAGS>(n))
		{
			for (size_t i=0; i<n; ++i) v[i] = p[i];
			return true;
		}
		else return false;
	}

	template<typename SysExField>
	bool SysEx_Reader::read(SysExField &field)
	{
		if (auto w = read<SysExField::IS_7_BIT ? SYSEX7 : SYSEX8>())
		{
			if (field.read_noByteCheck(w)) return 1;
			else fail_at(w, FAIL_INVALID, "illegal value for field");
		}
		return 0;
	}


	template<size_t N>
	inline uint8_t* SysEx_Writer::writebuf()
	{
		const uint8_t *p = pos; pos += N;
		if (pos > limit)
		{
			fail_at(p, FAIL_OVERRUN, "insufficient capacity for writing");
			return nullptr;
		}
		return p;
	}
	inline uint8_t* SysEx_Writer::writebuf(size_t n)
	{
		uint8_t *p = pos; pos += n;
		if (pos > limit)
		{
			fail_at(p, FAIL_OVERRUN, "insufficient capacity for writing");
			return nullptr;
		}
		return p;
	}
	template<size_t N, int FLAGS>
	bool SysEx_Writer::write(const uint8_t *v)
	{
		if (uint8_t *w = writebuf<N>())
		{
			for (size_t i = 0; i < N; ++i)
			{
				w[i] = v[i];
				if (FLAGS & SYSEX7) if (v[i] & 0x80)
				{
					fail_at(w+i, FAIL_7BIT, "illegal high bit during SysEx7 write");
					return false;
				}
			}
			return true;
		}
		else return false;
	}
	template<int FLAGS>
	bool SysEx_Writer::write(const uint8_t *v, size_t n)
	{
		if (uint8_t *w = writebuf(n))
		{
			for (size_t i = 0; i < n; ++i)
			{
				w[i] = v[i];
				if (FLAGS & SYSEX7) if (v[i] & 0x80)
				{
					fail_at(w+i, FAIL_7BIT, "illegal high bit during SysEx7 write");
					return false;
				}
			}
			return true;
		}
		else return false;
	}

	template<typename SysExField>
	bool SysEx_Writer::write(const SysExField &field)
	{
		if (auto w = writebuf<SysExField::BYTE_SIZE>())
		{
			if (field.write(w)) return 1;
			else fail_at(w, FAIL_INVALID, "attempted to write malformed value");
		}
		return 0;
	}

	/*template<size_t N, typename T>
	bool SysEx_Writer::write_(T val, bool (*write_func)(byte_t*,T))
	{
		if (auto w = writebuf<N>())
		{
			if (write_func(w,val)) return 1;
			else fail_at(w, FAIL_INVALID, "attempted to write malformed value");
		}
		return 0;
	}*/

	
}
