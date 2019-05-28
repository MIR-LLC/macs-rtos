/// @file macs_list.hpp
/// @brief Списки.
/// @details Содержит классы для работы со списками. 
/// @copyright AstroSoft Ltd, 2016
 
#pragma once
 
#include <string.h>
#include <stdint.h>
#include <stddef.h>

#include "macs_common.hpp"

namespace utils {
	  
/// @brief Шаблон класса для представления списка.
/// @details Параметром шаблона является тип элемента.
template <typename T>
class DynArr
{
public:
	typedef T * Iterator;

	/// @brief Специальная константа для индикации недопустимой позиции в списке.
	static const size_t NPOS = static_cast<size_t>(-1);

	/// @brief Конструктор.
	/// @details Создает пустой список.
	DynArr();

	/// @brief Конструктор.
	/// @param capacity - Начальная вместимость внутреннего хранилища(т.е. количество элементов, которое список сможет вместить без реаллокации внутреннего хранилища).
	explicit DynArr(size_t capacity);

	/// @brief Деструктор.
 	~DynArr();

	/// @brief Вставляет новый элемент на указанную позицию.
	/// @param index - Позиция, на которую следует вставить новый элемент.
	/// @param item - Значение нового элемента.
	void Insert(size_t index, const T & item);

	/// @brief Вставляет новый элемент в начало списка.
	/// @param item - Значение нового элемента.
	void AddFront(const T & item) { Insert(0, item); }

	/// @brief Вставляет новый элемент в конец списка.
	/// @param item - Значение нового элемента.
	void Add(const T & item) 			{ Insert(m_count, item); }

	/// @brief Удаляет из списка первый элемент с указанным значением.
	/// @param item - Значение элемента, который следует удалить.
	/// @return true - если элемент с указанным значением найден и удален, false - в противном случае.
	bool Remove(const T & item);

	/// @brief Удаляет из списка элемент на указанной позиции.
	/// @param index - Позиция, с которой следует удалить элемент.
	void RemoveAt(size_t index);

	/// @brief Удаляет из списка все элементы и освобождает память внутреннего хранилища.
	/// @param index - Позиция, с которой следует удалить элемент.
	void Clear();


	/// @brief Возвращает позицию первого элемента в списке, имеющего указанное значение.
	/// @param item - Значение элемента, позицию которого следует найти.
	/// @return Позицию найденного элемента. Если указанное значение не найдено, то функция вернет значение [NPOS](@ref utils::DynArr::NPOS).
	size_t IndexOf(const T & item);

	/// @brief Проверяет, содержится ли в списке элемент с указанным значением.
	/// @param item - Значение элемента, наличие которого следует проверить.
	/// @return true - если элемент с указанным значением имеется в списке, false - в противном случае.
	bool Contains(const T & item);

	/// @brief Возвращает ссылку на элемент в указанной позиции.
	/// @param index - Позиция элемента.
	/// @return Ссылка на элемент в указанной позиции.
	T & operator[](size_t index) 	const { _ASSERT(index < m_count);	return m_items[index]; 			 }

	/// @brief Возвращает ссылку на элемент в начале списка.
	/// @return Ссылка на элемент в начале списка.
	T & First() 									const	{ _ASSERT(m_count); 				return m_items[0]; 					 }

	/// @brief Возвращает ссылку на элемент в конце списка.
	/// @return Ссылка на элемент в конце списка.
	T & Last() 									 	const { _ASSERT(m_count); 				return m_items[m_count - 1]; }

	/// @brief Удаляет из списка элемент на указанной позиции, возвращая его значение.
	/// @param index - Позиция, с которой следует удалить элемент.
	/// @return Значение удаленного элемента.
	T TakeAt(size_t index);

	/// @brief Удаляет из списка элемент с начала списка, возвращая его значение.
	/// @return Значение удаленного элемента.
	T TakeFirst() { return TakeAt(0); }

	/// @brief Удаляет из списка элемент с конца списка, возвращая его значение.
	/// @return Значение удаленного элемента.
	T TakeLast() 	{ return TakeAt(m_count - 1); }

	/// @brief Возвращает количество элементов в списке.
	/// @return Количество элементов в списке.
	size_t Count() const { return m_count; }

	/// @brief Возвращает итератор, указывающий на первый элемент в списке.
	/// @return Итератор, указывающий на первый элемент в списке.
	Iterator Begin() const 	{ return & m_items[0]; }

	/// @brief Возвращает итератор, указывающий на элемент, идущий за последним в списке.
	/// @return Итератор, указывающий на элемент, идущий за последним в списке.
	Iterator End() const 		{ return & m_items[m_count]; }

	/// @brief Удаляет из списка элемент, на который указывает итератор.
	/// @param pos - Итератор, указывающий на удаляемый элемент.
	/// @return Переданный итератор pos
	Iterator Erase(Iterator pos);

	/// @brief Копирует содержимое списка в указанную внешнюю память.
	/// @param data - Указатель на внешнюю память, куда следует скопировать содержимое списка.
	void CopyTo(void * data) const;

	/// @brief Заполняет список содержимым из указанной внешней памяти.
	/// @param data - Указатель на внешнюю память, содержимым которой следует заполнить список.
	/// @param length - Размер внешней памяти в байтах.
	void CopyFrom(const void * data, size_t length);

	/// @brief Сортирует список так, чтобы для любых двух последовательных элементов выполнялось указанное условие.
	/// @details Less - это бинарный функтор, принимающий на вход два элемента типа Т и возвращающий значение, приводимое к bool.
	/// @details Значение, возвращаемое функтором, означает, следует ли второй переданный элемент за первым в отсортированном списке.
	/// @param less - Функтор сравнения двух элементов списка.
	template <typename Less>
	void Sort(Less less);

	/// @brief Сортирует список в неубывающем порядке.
	void Sort();

	void SetCapacity(size_t capacity);
	
private:	
	CLS_COPY(DynArr)

	void EnsureCapacity(size_t capacity);
	void Copy(T * srcArray, size_t srcIndex, T * dstArray, size_t dst_index, size_t length);

	size_t m_count;
	size_t m_capacity;
	T * 	 m_items;
};

template <typename T>
	DynArr<T>::DynArr() :
		m_count(0), 
		m_capacity(0), 
		m_items(nullptr)
{}

template <typename T>
	DynArr<T>::DynArr(size_t capacity) : 
		m_count(0), 
		m_capacity(capacity), 
		m_items(new T[capacity])
{}

template <typename T>
	DynArr<T>::~DynArr()
{
	Clear();
}

template <typename T>
	void DynArr<T>::Clear()
{
	if ( m_items )
		delete[] m_items;

	m_items = nullptr;
	m_capacity = 0;
	m_count = 0;
}

template <typename T>
	void DynArr<T>::Insert(size_t index, const T & item)
{
	_ASSERT(index <= m_count);
	if ( index > m_count )
		return;

	if ( m_count == m_capacity )
		EnsureCapacity(m_count + 1);

	if ( index < m_count )
		Copy(m_items, index, m_items, index + 1, m_count - index);

	m_items[index] = item;
	++ m_count;
}

template <typename T>
	bool DynArr<T>::Remove(const T & item)
{
	size_t index = IndexOf(item);
	if ( index == NPOS ) 
		return false;
	
	RemoveAt(index);
	return true;
}

template <typename T>
	size_t DynArr<T>::IndexOf(const T & item)
{
	for ( size_t index = 0; index < m_count; ++ index )
		if ( m_items[index] == item )
			return index;

	return NPOS;
}

template <typename T>
	inline bool DynArr<T>::Contains(const T & item)
{
	return IndexOf(item) != NPOS;
}

template <typename T>
	T DynArr<T>::TakeAt(size_t index)
{
	_ASSERT(index < m_count);

	T item = m_items[index];
	RemoveAt(index);

	return item;
}

template <typename T>
	void DynArr<T>::RemoveAt(size_t index)
{
	_ASSERT(index < m_count);
	if ( index >= m_count )
		return;

	-- m_count;	
	if ( index < m_count )
		Copy(m_items, index + 1, m_items, index, m_count - index);
}

template <typename T>
	typename DynArr<T>::Iterator DynArr<T>::Erase(Iterator pos)
{
	size_t index = pos - Begin();
	RemoveAt(index);

	return & m_items[index];
}

template <typename T>
	void DynArr<T>::EnsureCapacity(size_t min_capacity)
{
	static const size_t max_capacity = static_cast<size_t>(-1);
	static const size_t default_capacity = 4;

	if ( m_capacity < min_capacity ) {
		size_t num = (m_capacity == 0 ? default_capacity : m_capacity * 2);
		 
		if ( num > max_capacity )
			num = max_capacity;

		if ( num < min_capacity )
			num = min_capacity;

		SetCapacity(num);
	}
}

template <typename T>
	void DynArr<T>::SetCapacity(size_t capacity)
{
	T * new_items = new T[capacity];
	if ( m_count > 0 )
		Copy(m_items, 0, new_items, 0, m_count);

	if ( m_items )
		delete[] m_items;

	m_capacity = capacity;
	m_items = new_items;
}

template <typename T>
	inline void DynArr<T>::Copy(T * src_array, size_t src_index, T * dst_array, size_t dst_index, size_t length)
{
	memmove(dst_array + dst_index, src_array + src_index, sizeof(T) * length);
}

template <typename T>
	template <typename Less>
		inline void DynArr<T>::Sort(Less less)
{
	StableSort(Begin(), End(), less);
}

template <typename T>
	inline void DynArr<T>::Sort()
{
	Sort(Less<T>());
}

template <typename T>
	void DynArr<T>::CopyTo(void * data) const
{
	memcpy(data, m_items, m_count * sizeof(T));
}

template <typename T>
	void DynArr<T>::CopyFrom(const void * data, size_t length)
{
	Clear();
	m_count = length / sizeof(T);
	m_capacity = m_count;
	m_items = new T[m_capacity];

	memcpy(m_items, data, length); 
} 

typedef byte * LIST_PTR;
 
template<typename T, const size_t next_elm_offset>
	class SList
{
private:
	static const size_t m_offset = next_elm_offset;
public:	
	static inline const T * & Next(const T * elm) { return * (const T **) (((LIST_PTR) elm) + m_offset); }
	static inline T * & Next(T * elm) { return * (T **) (((LIST_PTR) elm) + m_offset); }
	static ulong Qty(const T * head) {
		ulong qty = 0;
		while ( head ) {
			++ qty;
			head = Next(head);
		}
		return qty;	
	}
	static T ** Find(T * & head, T * elm) {
		T ** ptr = & head;
		while ( (* ptr) ) {
			if ( (* ptr) == elm )
				break;
			ptr = & Next(* ptr);
		}
		return ptr;
	}
	static void Add(T * & head, T * elm) {
		_ASSERT(elm);
		_ASSERT(! Next(elm));	// элемент не находится в другом списке
		_ASSERT(! * Find(head, elm));
		
		Next(elm) = head;
		head = elm;		
	} 
	static void Del(T * & head, T * elm) {
		_ASSERT(elm);
		 
		T ** ptr = Find(head, elm);
		if ( (* ptr) ) {
			(* ptr) = Next(elm);
#if MACS_DEBUG
			Next(elm) = nullptr;	// очистить ссылку для отладки
#endif		
		}
	}	
	static T * Fetch(T * & head) {
		T * elm = head;		
		if ( head ) {
			head = Next(head);
#if MACS_DEBUG
			Next(elm) = nullptr;	// очистить ссылку для отладки
#endif	
		}
		return elm;
	}	 
	static T * Func(T * & head, bool (* pf)(T *)) {
		_ASSERT(pf);
		T * elm = head; 
		while ( elm ) {
			if ( ! (* pf)(elm) )
				break;
			elm = Next(elm);
		}			
		return elm;
	}
};	
#define SLIST_NEXT_OFFSET(type, next)		((size_t) & ((type *) 0)->next)
#ifndef MACS_CCC
	#define SLIST_DECLARE(name, type, next) \
		static const size_t name##next_elm_offset = SLIST_NEXT_OFFSET(type, next); \
		typedef SList<type, name##next_elm_offset> name
#else	// пока просто проверяем сборку
	#define SLIST_DECLARE(name, type, next) typedef SList<type, 0> name
#endif
	 
template<typename T, size_t next_elm_offset, bool Preceeding(T * a, T * b)>	 // Функция Preceeding возвращает true если a должно предшествовать b
	class SListOrd :
		public SList<T, next_elm_offset>
{
public:	
	static void Add(T * & head, T * elm) {
		T ** ptr = & head;
		while ( (* ptr) ) {
			_ASSERT((* ptr) != elm);
			if ( Preceeding(elm, (* ptr)) )
				break;
			ptr = & SList<T, next_elm_offset>::Next(* ptr);
		}
		SList<T, next_elm_offset>::Add((* ptr), elm);
	}
};
#ifndef MACS_CCC 
	#define SLISTORD_DECLARE(name, type, next, less) \
		static const size_t name##next_elm_offset = SLIST_NEXT_OFFSET(type, next); \
		typedef SListOrd<type, name##next_elm_offset, less> name
#else	// пока просто проверяем сборку
	#define SLISTORD_DECLARE(name, type, next, less) typedef SListOrd<type, 0, less> name
#endif		
	
}	// namespace utils 

using namespace utils;
