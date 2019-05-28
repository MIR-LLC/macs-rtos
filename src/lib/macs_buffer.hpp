/// @file macs_buffer.hpp
/// @brief Классы для работы с буфером.
/// @copyright AstroSoft Ltd, 2016

#pragma once

#include "macs_common.hpp"

namespace utils {

typedef uint8_t byte;

/// @brief Универсальный буфер.
/// @details Универсальный буфер, способный работать в разных режимах для разных реализаций.
class Buf
{
public:
	static const size_t DEF_BUF_SIZE = 64;

protected:
	enum BUF_STATE {
		BS_OWN = (0x01 << 0),
		BS_DYN = (0x01 << 1),
		BS_DYN_OWN = BS_DYN | BS_OWN
	};

	BitMask<BUF_STATE>	m_state;
	size_t				m_len;
	uint				m_beg;
	byte				*m_mem;
	byte				*m_mem_aligned;
	size_t				m_size;

public:
	// cppcheck-suppress uninitMemberVar
	/// @brief Конструктор. Создает пустой буфер, без возможности динамически изменять размер.
	Buf() { Reset(); }

	/// @brief Деструктор
	 ~Buf() { FreeMem(); }

	/// @brief Получить размер доступного для чтения содержимого в буфере (в байтах).
	/// @return Размер доступного для чтения содержимого в буфере (в байтах).
 	inline size_t Len() const { return m_len; }

	/// @brief Получить размер буфера в байтах.
	/// @return Размер буфера в байтах.
	inline size_t Size() const { return m_size; }

	/// @brief Получить размер свободной памяти в буфере (в байтах).
	/// @details Возвращает сумму уже считанных байт в начале буфера и незанятых байт в конце буфера.
	/// @return Размер свободной памяти в буфере (в байтах).
	inline size_t Rest() const { return Size() - Len(); }

	/// @brief Получить указатель на первый байт содержимого буфера.
	/// @return Указатель на первый байт содержимого буфера.
	inline byte *Data() { return &m_mem_aligned[m_beg]; }

	/// @brief Получить указатель на первый байт содержимого буфера.
	/// @return Указатель на первый байт содержимого буфера.
	inline const byte *Data() const { return &((const byte *)m_mem_aligned)[m_beg]; }

	/// @brief Получить значение указанного байта в содержимом буфера.
	/// @param i - индекс запрашиваемого байта.
	/// @return Значение указанного байта в содержимом буфера.
	inline byte operator[] (size_t i) const
	{
		_ASSERT(i < Len());
		return Data()[i];
	}

	/// @brief Оператор сравнения двух буферов.
	/// @details Сравнение осуществляется по содержимому побайтно.
	/// @param buf - буфер, с которым будет производиться сравнение содержимого текущего буфера.
	/// @return true - содержимое обоих буферов одинаковое, false - в противном случае.
	bool operator == (const Buf &buf) const;

	/// @brief Опустошает буфер.
	/// @details Реаллокации памяти не осуществляется, данные не затираются. Буфер помечается как имеющий 0 байтов для чтения.
	inline void Reset() { m_beg = m_len = 0; }


	/// @brief Выделяет память для буфера.
	/// @details На поведение функции влияет флаг m_state.
	/// @details При выставленном флаге BS_DYN возможно динамическое изменение размеров буфера.
	/// @details При текущем размере буфера меньше size функция равносильна вызову Reset().
	/// @details При текущем размере буфера больше size происходит реаллокация большего куска памяти.
	/// @details При отсутствующем флаге BS_DYN вызов данной функции приведет к срабатыванию assert-a.
	/// @param size - Желаемый размер буфера в байтах.
	/// @param alignment - Желаемое выравнивание памяти.
	/// @return true - если аллокация прошла успешно, false - в противном случае.
	bool Alloc(size_t size, size_t alignment = 1);

	/// @brief Освобождает память, занятую буфером.
	void Free();

	/// @brief Увеличивает размер доступного для чтения содержимого в буфере.
	/// @details Функция расширяет доступное для чтения содержимое за счет неиспользуемых байтов, идущих прямо за текущим содержимым в буфере.
	/// @param len - Количество добавляемых к доступному для чтения содержимому байтов.
	inline void AddLen(size_t len)
	{
		_ASSERT(Len() + len <= Size());
		m_len += len;
	}

	/// @brief Считывает первый байт содержимого из буфера.
	/// @return Значение первого байта содержимого из буфера.
	byte ReadByte()
	{
		_ASSERT(m_len >= 1);
		m_len -= 1;
		m_beg += 1;
		return *(Data() - 1);
	}

	/// @brief Считывает двухбайтовое число из содержимого буфера.
	/// @return Двухбайтовое число, считанное из буфера.
	inline int16_t ReadInt16()
	{
		int16_t val;
		Read((byte *)&val, sizeof(val));
		return val;
	}

	/// @brief Считывает четырёхбайтовое число из содержимого буфера.
	/// @return Четырёхбайтовое число, считанное из буфера.
	inline int32_t ReadInt32()
	{
		int32_t val;
		Read((byte *)&val, sizeof(val));
		return val;
	}

	/// @brief Считывает указанное количество байт содержимого из буфера.
	/// @param ptr - Указатель на первый байт внешней памяти.
	/// @param len - Количество байт из буфера, которые следует скопировать во внешнюю память.
	/// @param move_pos - Следует ли менять текущее положение указателя данных в буфере.
	void Read(byte *ptr, size_t len, bool move_pos = true);
	inline void Read(byte *ptr, size_t len) const { ((Buf *) this)->Read(ptr, len, false); }
	inline void Read(byte *ptr, size_t len, bool move_pos) const { _ASSERT(move_pos == false); Read(ptr, len); }

	/// @brief Добавляет в буфер 1 байт данных.
	/// @param val - Значение байта, добавляемого в буфер.
	inline void AddByte(byte val)
	{
		*(Data() + m_len) = val;
		AddLen(1);
	}

	/// @brief Добавляет в буфер двухбайтовое число.
	/// @param val - Двухбайтовое число, добавляемое в буфер.
	inline void AddInt16(int16_t val) { Add((const byte *)&val, sizeof(val)); }

	/// @brief Добавляет в буфер четырёхбайтовое число.
	/// @param val - Четырёхбайтовое число, добавляемое в буфер.
	inline void AddInt32(int32_t val) { Add((const byte *)&val, sizeof(val)); }

	/// @brief Добавляет в буфер данные из указанной внешней памяти.
	/// @param ptr - Указатель на первый байт внешней памяти.
	/// @param len - Количество байт из внешней памяти, которые следует добавить в буфер.
	void Add(const byte *ptr, size_t len); 

	/// @brief Добавляет в буфер данные из другого буфера.
	/// @param buf - Буфер-источник.
	void Add(const Buf & buf) { Add(buf.Data(), buf.Len()); }

	/// @brief Копирует содержимое буфера из указанной внешней памяти.
	/// @param ptr - Указатель на первый байт внешней памяти.
	/// @param len - Количество байт из буфера, которые следует скопировать из внешней памяти.
	void Copy(const byte *ptr, size_t len);

	/// @brief Копирует содержимое буфера в другой буфер.
	/// @param buf - [Буфер, из которого будет скопировано содержимое в данный буфера](@ref macs::Buf).
	inline void Copy(const Buf &buf) { Copy(buf.Data(), buf.Len()); }

	/// @brief Заменяет содержимое буфера содержимым из указанной внешней памяти.
	/// @param ptr - Указатель на первый байт внешней памяти.
	/// @param len - Количество байт из внешней памяти, которые следует взять.
	/// @param dupe - Флаг, указывающий, следует ли буферу взять права на владение внешней памятью.
	void Grab(byte *ptr, size_t len, bool dupe = false);

	/// @brief Равносильно Grab(ptr, len, true).
	inline void Dupe(const byte *ptr, size_t len) { Grab((byte *)ptr, len, true); }

	/// @brief Заменяет содержимое буфера содержимым из другого буфера.
	/// @param buf - Буфер, содержимое которого вставляется в текущий буфер.
	/// @param dupe - Флаг, указывающий, следует ли буферу взять права на владение внутренней памятью переданного буфера.
	void Grab(Buf &buf, bool dupe = false);

	/// @brief Равносильно Grab(buf, true)
	inline void Dupe(Buf &buf) { Grab(buf, true); }

	/// @brief Добавляет len байт из переданного буфера в текущий буфер.
	/// @param buf - Буфер, первые байты содержимого которого вставляются в текущий буфер.
	/// @param len - Количество байт из буфера, которые следует вставить.
	void Trans(Buf &src, size_t len);

private:
	CLS_COPY(Buf)

	inline void FreeMem()
	{
		if (m_mem && m_state.Check(BS_DYN_OWN))
			delete[] m_mem;
	}
};

/// @brief Реализация буфера на статическом массиве.
/// @details Шаблон класса позволяет создавать реализации с различным размером.
/// По умолчанию используется размер массива Buf::DEF_BUF_SIZE.
template <size_t buf_size = Buf::DEF_BUF_SIZE>
	class StatBuf : public Buf
{
private:
	byte m_buf[buf_size];

public:
	StatBuf()
	{
		m_mem = m_mem_aligned = m_buf;
		m_size = buf_size;
	}
};

/// @brief Тип для буфера с размером по умолчанию.
typedef StatBuf<Buf::DEF_BUF_SIZE> DefStatBuf;

/// @brief Буфер с динамически изменяемым размером.
class DynBuf : public Buf
{
public:
	/// @brief Конструктор. Создает пустой буфер.
	DynBuf(size_t size = 0)
	{
		m_state.Add(BS_DYN_OWN);
		m_size = 0;
		m_mem = m_mem_aligned = nullptr;
		if (size)
			Alloc(size);
	}
};

}	// namespace utils
using namespace utils;
