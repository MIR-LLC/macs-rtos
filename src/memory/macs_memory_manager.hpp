/// @file macs_memory_manager.hpp
/// @brief Управление динамической памятью.
/// @details Определяет интерфейс для работы с динамической памятью.
/// @copyright AstroSoft Ltd, 2016

#pragma once

#include <stddef.h>
#include <stdint.h>

#include "macs_system.hpp"
#include "macs_application.hpp"

namespace macs {

/// @brief Распределитель динамической памяти.
/// @details Методы распределителя памяти вызываются ядром системы и не должны использоваться напрямую из пользовательского приложения.
class MemoryManager
{
private:
	static const uint32_t HEAP_SIZE = System::HEAP_SIZE;

	static byte s_lock;
#if MACS_MEM_STATISTICS
	static size_t s_cur_heap_size, s_peak_heap_size;
#endif

	class HeapLocker
	{
	public:
		HeapLocker() 
		{
			if (ExclSet(s_lock))
				MACS_ALARM(AR_MEM_LOCKED);
		}

	 ~HeapLocker() { s_lock = 0; }
	};

public:
	template <uint32_t heapSize>
	static void Initialize();

	static void * Allocate		(size_t size); 
	static void   Deallocate	(void	* ptr);
#if MACS_MEM_WIPE
	static void   Wipe(void	* ptr, size_t size) { memset(ptr, 0xCC, size); }
#endif	

#if MACS_MEM_STATISTICS
	static size_t MaxHeapSize()		{ return s_heap_size; }
	static size_t CurHeapSize()		{ return s_cur_heap_size; }
	static size_t PeakHeapSize()	{ return s_peak_heap_size; }
#endif

private:
	MemoryManager();
	~MemoryManager();

	CLS_COPY(MemoryManager)

	static void * MemAlloc(size_t size);
	static void MemFree(void * ptr);

	static void LogAllocatedSize();

	static bool s_init_flag;
	static size_t s_heap_size;
};

template <uint32_t heapSize>
void MemoryManager::Initialize()
{
	s_heap_size = heapSize;
	s_init_flag = true;
}

////////////////////////////////////////////////////////////////

/// @brief Пулы памяти с фиксированным размером блока.
/// @details Пулы памяти могут быть использованы в пользовательском приложении для управления
/// блоками памяти фиксированного размера с предсказуемым временем выполнения.
class MemoryPool
{
private:
	struct MemPoolHdr {
		MemPoolHdr * next;
	};
	static const size_t HDR_SIZE = sizeof(MemPoolHdr);

private:
	bool	m_is_alien_mem;
	size_t	m_block_size;
	size_t	m_block_total;
	size_t	m_block_used;
	byte *       m_block_mem;
	MemPoolHdr * m_free_list;
	 
public:	
	/// @brief Конструктор.
	/// @details Создаёт объект пула памяти. Если указаны параметры, то пул будет автоматически
	/// инициализирован с использованием соответсвующих значений (см. @ref MemoryPool::Create).
	/// В противном случае будет создан неинициализированный пул.
	/// @param block_size - размер блоков памяти в байтах.
	/// @param block_total - количество блоков памяти.
	/// @param mem - указатель на внешнюю память для размещения пула.
	MemoryPool(size_t block_size = 0, size_t block_total = 0, void * mem = nullptr);

	/// @brief Деструктор.
	/// @details Уничтожает объект пула памяти. Если пул был размещён в динамической памяти
	/// (параметр mem при создании и инициализации не указан), то память, выделенная в куче для
	/// размещения пула, освобождается.
	~MemoryPool() { Free(); }

	/// @brief Получение статуса готовности пула памяти.
	/// @return true - пул инициализирован и готов к использованию, false - в противном случае.
	inline bool IsReady() const { return m_block_total != 0; }

	/// @brief Инициализация пула.
	/// @details Инициализирует объект пула указанными параметрами. Если значение параметра 
	/// block_total равно 0, то инициализация не выполняется. Если функция вызвана для уже
	/// инициализированного объекта, то пул будет переконфигурирован заново: используемая
	/// динамическая память будет освобождена (при размещении пула в динамической памяти),
	/// параметры пула установлены в новые значения, а указатели на все ранее выделенные блоки
	/// памяти будут считаться недействительными.
	/// @param block_total - количество блоков памяти.
	/// @param mem - указатель на внешнюю память для размещения пула. Если не указан, то память для
	/// размещения пула памяти будет автоматически выделена из кучи.
	/// @param block_size - размер блоков памяти в байтах. Должен быть кратен размеру указателя на
	/// целевой платформе. Значение по умолчанию: 0 (задан в конструкторе).
	void Create(size_t block_total, void * mem = nullptr, size_t block_size = 0);

	/// @brief Выделение блока памяти.
	/// @details Если имеется доступный для выделения блок памяти (количество свободных блоков > 0),
	/// функция возвращает указатель на свободный блок. При этом количество доступных блоков
	/// уменьшается на 1, а количество занятых блоков увеличивается на 1. В противном случае
	/// возвращается nullptr (нет доступных блоков).
	/// При выделении блока памяти проверка готовности пула не производится, поэтому попытка
	/// выделения блока из неинициализированного пула может привести к неопределённому поведению.
	/// Для проверки готовности пула должна использоваться функция @ref MemoryPool::IsReady.
	/// @return Указатель на свободный блок памяти или nullptr.
	void * AllocBlock();

	/// @brief Освобождение блока памяти.
	/// @details Функция освобождает указанный блок памяти. При этом количество доступных блоков
	/// увеличивается на 1. Если включена опция @ref MACS_MEM_WIPE, то при освобождении выполняется
	/// уничтожение данных в памяти блока. Освобождаемый блок должен быть предварительно выделен
	/// функцией @ref MemoryPool::AllocBlock. Использование неверного указателя может привести к
	/// неопределённому поведению.
	/// @param pblk - Указатель на начало освобождаемого блока памяти.
	void FreeBlock(void * pblk);

	/// @brief Получение размера блока.
	/// @details Функция возвращает размер блока памяти в байтах, указанный при создании или
	/// инициализации пула.
	/// @return Размер блока.
	inline size_t GetBlockSize() const { return m_block_size; }

	/// @brief Получение общего количества блоков.
	/// @details Функция возвращает количество блоков памяти, указанное при создании или
	/// инициализации пула.
	/// @return Общее количество блоков.
	inline size_t GetTotalBlocksQty() const { return m_block_total; }

	/// @brief Получение количества свободных блоков.
	/// @details Функция возвращает количество свободных (доступных для выделения) в настоящий
	/// момент блоков памяти.
	/// @return Количество свободных блоков.
	inline size_t GetFreeBlocksQty() const { return m_block_total - m_block_used; }

	/// @brief Получение количества занятых блоков.
	/// @details Функция возвращает количество занятых (выделенных методом AllocBlock) в настоящий
	/// момент блоков памяти.
	/// @return Количество занятых блоков.
	inline size_t GetAllocatedBlocksQty() const { return m_block_used; }

private:
	static inline bool IsGoodSize(size_t block_size) { return (block_size & (HDR_SIZE - 1)) == 0x0; }
	void Free();
	inline MemPoolHdr * Shift(MemPoolHdr * ph, size_t ind = 1) { return (MemPoolHdr *) (((byte *) ph) + (ind * m_block_size)); }
};

////////////////////////////////////////////////////////////////

class MemoryHeap  // Размеры в словах
{
private:	
	class Header 
	{
	private:	
		word_t m_len  : 31;
		bool   m_busy : 1;
	public:	
		inline void Set(bool is_busy, size_t len) { m_busy = is_busy; m_len = len; }
		
		inline bool IsBusy() const { return m_busy; }
		inline size_t Length() const { return m_len; }
		
		inline void Merge(const Header * phdr) { m_len += SIZE() + phdr->m_len; } 

		inline Header * Alloc(size_t len, uint align) {
			if ( len > m_len )
				return nullptr;

			if ( m_len - len <= SIZE() && ALIGN_WPTR(this + 1, align) == (word_t *) (this + 1) ) {
				m_busy = true;
				return this;
			} 
			word_t * ptr = (word_t *) (this + 1) + m_len - len;
			uint odd = ptr - ALIGN_WPTR_BACK(ptr, align);
			if ( SIZE() + len + odd >= m_len )
				return nullptr;

			m_len -= (SIZE() + len + odd);
			Header * phdr = Next();
			phdr->Set(true, len + odd);
			return phdr;
		}
		
		inline void Free() { m_busy = false; }
		
		static inline Header * Next(Header * phdr) { return (Header *) ((word_t *) (phdr + 1) + phdr->m_len); }
		inline const Header * Next() const { return Next(const_cast<Header *>(this)); }
		inline Header * Next() { return Next(this); }
		
		static size_t SIZE() { return sizeof(Header) / sizeof(word_t); }
	};
private:	
	word_t * m_base;
  size_t   m_size;
public:	
	MemoryHeap() { m_base = nullptr; Init(nullptr, 0); }
	MemoryHeap(word_t * base, size_t size) { m_base = nullptr; Init(base, size); }
	void Init(word_t * base, size_t size, bool build = true);

	static inline size_t HdrSize() { return Header::SIZE(); }

	inline bool IsReady() const { return !! m_base; }

	word_t * Alloc(size_t len, uint align = 0);
	void     Free(word_t * ptr);
	bool     Check(const word_t * ptr) const;
	inline size_t Size(const word_t * ptr) const { return ((Header *) ptr - 1)->Length(); }

	bool     Check() const;
private:	
	inline const Header * Margine() const { return (Header *) (m_base + m_size); }
	
	static inline Header * First(MemoryHeap * heap) { return (Header *) heap->m_base; }
	inline Header * First() { return First(this); }
	inline const Header * First() const { return First(const_cast<MemoryHeap *>(this)); }

	inline bool IsLast(const Header * phdr) const {
		_ASSERT(phdr->Next() <= Margine());
		return phdr->Next() == Margine();
	}
	bool Combine(Header *);
};

}  // namespace macs
using namespace macs;
