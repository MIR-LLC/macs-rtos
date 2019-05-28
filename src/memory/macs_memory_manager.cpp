/// @file macs_memory_manager.cpp
/// @brief Управление динамической памятью.
/// @details Определяет интерфейс для работы с динамической памятью.
/// @copyright AstroSoft Ltd, 2016

#include <stdlib.h>
#include <stdio.h>
  
#include "macs_memory_manager.hpp"
#include "macs_scheduler.hpp"
#include "macs_task.hpp"

#if MACS_USE_MPU
void MPU_Init()
{
	System::MpuInit();

	#if MACS_MPU_PROTECT_NULL
	MPU_SetMine(MPU_ZERO_ADR_MINE, 0x00000000U);
	#endif

	#if MACS_MPU_PROTECT_STACK
	MPU_SetMine(MPU_MAIN_STACK_MINE, ((uint32_t) (((byte *) (System::MacsMainStackBottom - System::MacsMainStackSize)) + (0x20 - 1))) & ~0x1F);
	#endif
}

void MPU_SetMine(MPU_MINE_NUM rnum, uint32_t adr)
{
	_ASSERT((adr & 0x1F) == 0);
	System::MpuSetMine(rnum, adr);
}

void MPU_RemoveMine(MPU_MINE_NUM rnum)
{
	System::MpuRemoveMine(rnum);
}
#endif

namespace macs {

#if ! MACS_MEM_ON_PAUSE
static SpinLock heapLock;
#endif

bool MemoryManager::s_init_flag = false;
size_t MemoryManager::s_heap_size = 0;
byte MemoryManager::s_lock = 0;

#if MACS_MEM_STATISTICS
size_t MemoryManager::s_cur_heap_size = 0;
size_t MemoryManager::s_peak_heap_size = 0;
	#if MACS_DEBUG
static volatile long dbgCurHeapSize, dbgCurHeapChange; // Статические переменные для отладки
	#endif
#endif

void MemoryManager::LogAllocatedSize()
{
#if MACS_MEM_STATISTICS && MACS_PRINTF_ALLOWED
	printf("memory allocated: %d\n\r", (int) s_cur_heap_size);
#endif
}

void * MemoryManager::MemAlloc(size_t size)
{
#if MACS_MEM_STATISTICS
	if ( s_cur_heap_size + size > s_heap_size )
		return nullptr;
	size_t * ph = (size_t *) malloc(size + sizeof(size_t));
	if ( ! ph )
		return nullptr;
	* ph = size;
	s_cur_heap_size += size;
#if MACS_DEBUG
	dbgCurHeapChange = size;
	dbgCurHeapSize = s_cur_heap_size;
#endif
	if ( s_cur_heap_size > s_peak_heap_size )
		s_peak_heap_size = s_cur_heap_size;
	return ph + 1;
#else
	return malloc(size);
#endif
}

void MemoryManager::MemFree(void * ptr)
{
#if MACS_MEM_STATISTICS
	size_t * ph = (size_t *) ptr - 1;
	s_cur_heap_size -= * ph;
#if MACS_DEBUG
	dbgCurHeapChange = - (long) * ph;
	dbgCurHeapSize = s_cur_heap_size;
#endif
#if MACS_MEM_WIPE
	Wipe(ptr, * ph);
#endif
	free(ph);
#else
	free(ptr);
#endif
}

void* MemoryManager::Allocate(size_t size)
{
	if ( ! s_init_flag )
		Initialize<HEAP_SIZE>();

	if ( size == 0 )
		return nullptr;

#if ! MACS_MEM_ON_PAUSE
	LockGuard lock(heapLock);
#endif

	void * ptr;
	for(;;) {
		{
#if MACS_MEM_ON_PAUSE
			PauseSection _ps_;
#endif
			{
				HeapLocker _hl_;
				ptr = MemAlloc(size);
			}
		}
		if ( ptr )
			break;

		ALARM_ACTION act = MACS_ALARM(AR_OUT_OF_MEMORY);
		if ( act == AA_CONTINUE )
			continue;

		MACS_CRASH(AR_OUT_OF_MEMORY);
	}
	_ASSERT(ptr != nullptr);

	return ptr;
}
 
void MemoryManager::Deallocate(void* ptr)
{
	if ( ! s_init_flag )
		Initialize<HEAP_SIZE>();

	if ( ! ptr )
		return;

#if ! MACS_MEM_ON_PAUSE
	LockGuard lock(heapLock);
#endif

	{
#if MACS_MEM_ON_PAUSE
		PauseSection _ps_;
#endif
		{
			HeapLocker _hl_;
			MemFree(ptr);
		}
	}
}

////////////////////////////////////////////////////////////////

MemoryPool::MemoryPool(size_t block_size, size_t block_total, void * mem)
{
	_ASSERT(block_size == 0 || IsGoodSize(block_size));
	_ASSERT(block_total == 0 || block_size != 0);
	m_block_size = block_size;
	m_block_used = 0;
	m_is_alien_mem = false;
	m_block_mem = nullptr;
	m_free_list = nullptr;
	m_block_total = 0;
	if ( block_total )
		Create(block_total, mem);
}

void MemoryPool::Create(size_t block_total, void * mem, size_t block_size) 
{
	Free();
	if ( block_total ) {
		if ( block_size )
			m_block_size = block_size; 
		if ( m_block_size == 0 || ! IsGoodSize(m_block_size) )
			return;
		m_block_total = block_total;
		if ( mem ) {
			m_block_mem = (byte *) mem;
			m_is_alien_mem = true;
		} else {
			m_block_mem = new byte [m_block_total * m_block_size];
			m_is_alien_mem = false;
		}
		m_free_list = (MemPoolHdr *) m_block_mem;
		MemPoolHdr * const marg = Shift(m_free_list, m_block_total - 1);
		MemPoolHdr * curr = m_free_list;
		while ( curr < marg ) {
			MemPoolHdr * next = Shift(curr);
			curr->next = next;
			curr = next;
		}
		curr->next = nullptr;
		m_block_used = 0;
	}
}

void MemoryPool::Free() 
{
	if ( m_block_total ) {
		if ( ! m_is_alien_mem )
			delete [] m_block_mem;
		m_is_alien_mem = false;
		m_block_total = 0;
		m_block_used = 0;
		m_block_mem = nullptr;
		m_free_list = nullptr;
	}
}
 
void * MemoryPool::AllocBlock()
{
	PauseSection _ps_;
	void * ptr = m_free_list;
	if ( ptr ) {
		m_free_list = m_free_list->next;
		++ m_block_used;
	}
	return ptr;
}

void MemoryPool::FreeBlock(void * pblk)
{
	_ASSERT(m_block_used != 0);
	_ASSERT(!! pblk);
	_ASSERT(pblk >= & m_block_mem[0] && pblk < & m_block_mem[m_block_total * m_block_size]);
	_ASSERT((((byte *) pblk - m_block_mem) % m_block_size) == 0);
#if MACS_MEM_WIPE
	MemoryManager::Wipe(pblk, m_block_size);
#endif
	PauseSection _ps_;
	((MemPoolHdr *) pblk)->next = m_free_list;
	m_free_list = (MemPoolHdr *) pblk;
	-- m_block_used;
}

////////////////////////////////////////////////////////////////

void MemoryHeap::Init(word_t * base, size_t size, bool build)
{
	_ASSERT(! IsReady());
	_ASSERT(!! base == (size != 0));
	
	m_base = base;
	m_size = size;	
	
	if ( IsReady() && build )
		First()->Set(false, size - HdrSize());
}

word_t * MemoryHeap::Alloc(size_t len, uint align)
{
	Header * pcur = First();

	for (;;) {
		if ( ! pcur->IsBusy() ) {
			while ( Combine(pcur) ) ;

			Header * phdr = pcur->Alloc(len, align);
			if ( phdr )
				return (word_t *) (phdr + 1);
		}
		if ( IsLast(pcur) )
			break;
		pcur = pcur->Next();
	} 
	return nullptr;
}

void MemoryHeap::Free(word_t * ptr)
{
	_ASSERT(Check(ptr));
	
	Header * phdr = ((Header *) ptr) - 1;
	phdr->Free();
	Combine(phdr);  // Балансирует нагрузку на Alloc/Free
}

bool MemoryHeap::Check() const
{
	_ASSERT(IsReady());

	const Header * pcur = First();

	while ( ! IsLast(pcur) )
		pcur = pcur->Next();

	return true;
}

bool MemoryHeap::Check(const word_t * ptr) const
{
	if ( !! ptr ) {
		Header * phdr = ((Header *) ptr) - 1;
		if ( phdr < Margine() ) {
			const Header * pcur = First();	
			for (;;) {
				if ( phdr == pcur )
					return pcur->IsBusy();
				if ( IsLast(pcur) )
					break;
				pcur = pcur->Next();
			}
		}		
	} 
	return false;
}

bool MemoryHeap::Combine(Header * phdr)
{
	_ASSERT(! phdr->IsBusy());

	if ( IsLast(phdr) )
		return false;

	Header * pnxt = phdr->Next();

	if ( pnxt->IsBusy() )
		return false;

	phdr->Merge(pnxt);
	return true;
}

}  // namespace macs 