/// @file macs_buffer.cpp
/// @brief Классы для работы с буфером.
/// @copyright AstroSoft Ltd, 2016

#include <string.h>

#include "macs_buffer.hpp"

namespace utils {
bool Buf::operator == (const Buf &buf) const
{
	if (Len() != buf.Len())
		return false;
	else
		return memcmp(Data(), buf.Data(), Len()) == 0;
}

bool Buf::Alloc(size_t size, size_t alignment)
{
	if (m_state.Check(BS_DYN))
	{
		if (size > m_size || !m_state.Check(BS_OWN))
		{
			FreeMem();
			size_t sizew = size / (sizeof(uint32_t)/sizeof(char));
			if (size % (sizeof(uint32_t)/sizeof(char)))
				sizew++;
			--alignment;
			uint32_t mem_addr = (uint32_t)(new uint32_t[sizew + alignment]);
			m_mem = (byte*)(int *)mem_addr;
			int mem_addr_al = (mem_addr + alignment) & ~ (alignment);
			//int *mem_addr_al_ptr = (int *)mem_addr_al;

			m_mem_aligned = (byte *)(int *)mem_addr_al;
			m_size = size;
			m_state.Add(BS_OWN);
		}
	}
	else
	{
		_ASSERT(size <= m_size);
	}
	Reset();
	return true;
}

void Buf::Free()
{
	FreeMem();
	if (m_state.Check(BS_DYN))
	{
		m_mem = m_mem_aligned = nullptr;
		m_size = 0;
		m_state.Add(BS_OWN);
	}
	Reset();
}

void Buf::Trans(Buf &src, size_t len)
{
	_ASSERT(len <= src.Len());

	Add(src.Data(), len);
	src.m_len -= len;
	src.m_beg += len;
}
 
void Buf::Read(byte *ptr, size_t len, bool move_pos)
{
	if (len)
	{
		_ASSERT(ptr);
		_ASSERT(m_len >= len);

		memcpy(ptr, Data(), len);
		if ( move_pos ) {
			m_len -= len;
			m_beg += len;
		}
	}
}

void Buf::Add(const byte *ptr, size_t len)
{
	if (len)
	{
		_ASSERT(ptr);
		_ASSERT(Len() + len <= Size());

		memcpy(Data() + m_len, ptr, len);
		m_len += len;
	}
}

void Buf::Copy(const byte *ptr, size_t len)
{
	if (len)
		_ASSERT(ptr);

	Alloc(len);
	memcpy(Data(), ptr, len);
	m_len = len;
}

void Buf::Grab(byte *ptr, size_t len, bool dupe)
{
	if (len)
		_ASSERT(ptr);

	if (m_state.Check(BS_DYN))
	{
		FreeMem();
		m_mem = m_mem_aligned = ptr;
		m_size = m_len = len;
		m_beg = 0;
		dupe ? m_state.Rem(BS_OWN) : m_state.Add(BS_OWN);
	}
	else
	{
		Copy(ptr, len);
		if (!dupe)
			delete ptr;
	}
}

void Buf::Grab(Buf &buf, bool dupe)
{
	if (m_state.Check(BS_DYN) && buf.m_state.Check(BS_DYN))
	{
		FreeMem();
		m_mem = buf.m_mem;
		m_mem_aligned = buf.m_mem_aligned;
		m_size = buf.m_size;
		m_len = buf.m_len;
		m_beg = buf.m_beg;

		if (!dupe && buf.m_state.Check(BS_OWN))
		{
			buf.m_state.Rem(BS_OWN);
			m_state.Add(BS_OWN);
		}
		else
			m_state.Rem(BS_OWN);
	}
	else
	{
		Copy(buf);
		if (!dupe)
			buf.Free();
	}
}
}
