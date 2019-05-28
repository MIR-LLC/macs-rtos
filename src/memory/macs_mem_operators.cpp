/// @file macs_mem_operators.cpp
/// @brief Операторы работы с памятью.
/// @details Переопределение операторов работы с памятью при использовании внешнего распределителя памяти.
/// @copyright AstroSoft Ltd, 2016

#include "macs_memory_manager.hpp"
#include <new>

#if defined(__ICCARM__) || defined(__GNUC__) || defined(__VISUALDSPVERSION__)		// IAR, GCC or VISUALDSP Compiler for C/C++

void * operator new(size_t size)
{
	return MemoryManager::Allocate(size);
}

void operator delete(void * ptr)
{
	MemoryManager::Deallocate(ptr);
}

void * operator new[](size_t size)
{
	return MemoryManager::Allocate(size);
}

void operator delete[](void * ptr)
{
	MemoryManager::Deallocate(ptr);
}

#else		// no __ICCARM__

void * operator new(size_t size, const std::nothrow_t &) throw()
{
	return MemoryManager::Allocate(size);
}

void operator delete(void * ptr, const std::nothrow_t &) throw()
{
	MemoryManager::Deallocate(ptr);
}

void * operator new(size_t size) throw(std::bad_alloc)
{
	return MemoryManager::Allocate(size);
}

void operator delete(void * ptr) throw ()
{
	MemoryManager::Deallocate(ptr);
}

void * operator new[](size_t size, const std::nothrow_t &) throw()
{
	return MemoryManager::Allocate(size);
}

void operator delete[](void * ptr, const std::nothrow_t &) throw()
{
	MemoryManager::Deallocate(ptr);
}

void * operator new[](size_t size) throw(std::bad_alloc)
{
	return MemoryManager::Allocate(size);
}

void operator delete[](void * ptr) throw()
{
	MemoryManager::Deallocate(ptr);
}

#endif		// no __ICCARM__
