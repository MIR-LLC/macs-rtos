/// @file macs_utils.hpp
/// @brief Общие определения.
/// @details Содержит общие определения констант, макросов и шаблонов. 
/// @copyright AstroSoft Ltd, 2016
  
#pragma once

#ifndef UINT32_MAX
	#define UINT32_MAX 0xFFFFFFFFu
#endif	
 
#define KILO_B * 1024U
#define MEGA_B KILO_B KILO_B
#define GIGA_B KILO_B MEGA_B

#define KILO_D * 1000U
#define MEGA_D KILO_D KILO_D
#define GIGA_D KILO_D MEGA_D
   
#define SEC  * 1000U
#define MNT  * 60 SEC
#define HRS  * 60 MNT

/// @brief Макрос, реализующий функцию abs.
#ifndef ABS
	#define ABS(v) ((v) < 0 ? -(v) : (v))
#endif

/// @brief Макрос, реализующий функцию min.
#ifndef MIN
	#define MIN(a, b) ((a) < (b) ? (a) : (b)) 
#endif

/// @brief Макрос, реализующий функцию max.
#ifndef MAX
	#define MAX(a, b) ((a) > (b) ? (a) : (b)) 
#endif

/// @brief Макрос, вычисляющий количество элементов в статическом массиве.
#define countof(arr) (sizeof(arr) / sizeof((arr)[0])) 

/// @brief Макрос, реализующий цикл итерации по элементам массива.
#define loop(type, i, lim) for( type i = 0; i < (lim); ++ i )

/// Пространство имен вспомогательного функционала ОС
namespace utils { 
	
typedef unsigned char byte;
typedef unsigned short 	ushort;
typedef unsigned int 		uint;
typedef unsigned int word_t;
typedef unsigned long 	ulong; 
typedef const char * CSPTR;
typedef  			char *  SPTR;
	
#ifdef __ECC__
typedef signed char  	int8_t;   
typedef signed short	int16_t;   
typedef signed int		int32_t;   

typedef unsigned char  	uint8_t;   
typedef unsigned short	uint16_t;   
typedef unsigned int		uint32_t;   
#endif	
	   
#define BYTE_MAX UCHAR_MAX	
	
// Маска для младших битов
#define ALIGN_MASK(gran)  (~(~0x0u << (gran)))
// Количество дополнительных элементов для возможности выравнивания адреса
#define ALIGN_OFFS(gran)  ALIGN_MASK(gran)
// Выровненный адрес (со сдвигом в сторону уменьшения)
#define ALIGN_WPTR_BACK(wptr, gran)  ((word_t *) ((word_t) (wptr) & ~ALIGN_MASK(gran)))
// Выровненный адрес (со сдвигом в сторону увеличения)
#define ALIGN_WPTR(wptr, gran)  ALIGN_WPTR_BACK((word_t *) (wptr) + ALIGN_OFFS(gran), (gran))
	
	/// @brief Масштабирование элемента из одного диапазона в другой.
	/// @details Масштабирование происходит по формуле (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min.
	/// @param x Значение элемента из диапазона [in_min, in_max], которому нужно сопоставить элемент из диапазона [out_min, out_max].
	/// @return Значение элемента, соответствующего входному элементу в новом диапазоне.
	template<typename T>
	T RemapValue(T x, T in_min, T in_max, T out_min, T out_max)
	{
		return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
	}

	/// @brief Шаблон вспомогательной структуру для хранения пары значений указанных типов Т1 и Т2.
	template <typename T1, typename T2>
	struct Pair
	{
	public:
		/// @brief Конструктор, создающий пару со стандартными значениями для типов Т1 и Т2.
		Pair() :
			first(0),
			second(0)
		{
		}

		/// @brief Конструктор, создающий пару со указанными значениями.
		/// @param first Значение первого элемента пары.
		/// @param second Значение второго элемента пары.
		Pair(T1 first, T2 second) :
			first(first),
			second(second)
		{
		}

		/// @brief Стандартный оператор сравнения.
		/// @param b Значение элемента для сравнения с текущим.
		/// @return true - если текущий элемент обязан располагаться до переданного элемента в отсортированной последовательности, иначе false.
		bool operator<(const Pair<T1, T2>& b) const
		{
			return this->first < b.first;
		}

		T1 first;
		T2 second;
	};

	/// @brief Функтор сравнения, реализующий стандартное сравнение с помощью оператора <.
	template <typename T>
	struct Less
	{
		/// @brief Оператор вызова.
		/// @param a Значение первого элемента для сравнения.
		/// @param b Значение второго элемента для сравнения.
		/// @return true - если первый элемент обязан располагаться до второго элемента в отсортированной последовательности, иначе false.
		bool operator()(const T& a, const T& b) const
		{
			return a < b;
		}
	};

	/// @brief Шаблон функции для обмена значениями двух переменных.
	/// @param value1 Ссылка на первую переменную.
	/// @param value2 Ссылка на вторую переменную.
	template <typename T>
	inline void Swap(T &value1, T &value2)
	{
		const T t = value1;
		value1 = value2;
		value2 = t;
	}

	/// @brief Шаблон функции для поиска первого вхождения элемента со значением, который не обязан стоять в отсортированной последовательности после указанного значения.
	/// @details Входная последовательность должна быть отсортирована согласно функтору less.
	/// @details Поиск ведется с помощью половинного деления. Алгоритмическая сложность O(log(n)).
	/// @param begin Итератор, указывающий на первый элемент последовательности.
	/// @param end Итератор, указывающий элемент, идущий за последним элементом последовательности.
	/// @param value Значение для сравнения.
	/// @param less Функтор сравнения.
	/// @return Итератор, указывающий на первый элемент, который не обязан стоять в отсортированной последовательности после указанного значения, либо end, если такое значение в последовательности отсутствует.
	template <typename Iterator, typename T, typename Less>
	Iterator LowerBound(Iterator begin, Iterator end, const T &value, Less less)
	{
		Iterator middle;
		int n = int(end - begin);
		int half;

		while (n > 0) 
		{
			half = n >> 1;
			middle = begin + half;
			if (less(*middle, value)) 
			{
				begin = middle + 1;
				n -= half + 1;
			}
			else
			{
				n = half;
			}
		}
		return begin;
	}

	/// @brief Шаблон функции для поиска первого вхождения элемента со значением, который стоит правее указанного значения в отсортированной последовательности.
	/// @details Входная последовательность должна быть отсортирована согласно функтору less.
	/// @details Поиск ведется с помощью половинного деления. Алгоритмическая сложность O(log(n)).
	/// @param begin Итератор, указывающий не первый элемент последовательности.
	/// @param end Итератор, указывающий элемент, идущий за последним элементом последовательности.
	/// @param value Значение для сравнения.
	/// @param less Функтор сравнения.
	/// @return Итератор, указывающий на первый элемент, который стоит правее указанного значения в отсортированной последовательности, либо end, если такое значение в последовательности отсутствует.
	template <typename Iterator, typename T, typename Less>
	Iterator UpperBound(Iterator begin, Iterator end, const T &value, Less less)
	{
		Iterator middle;
		int n = end - begin;
		int half;

		while (n > 0) 
		{
			half = n >> 1;
			middle = begin + half;
			if (less(value, *middle)) 
			{
				n = half;
			}
			else 
			{
				begin = middle + 1;
				n -= half + 1;
			}
		}
		return begin;
	}

	/// @brief Шаблон функции для реверса элементов последовательности.
	/// @param begin Итератор, указывающий на первый элемент последовательности.
	/// @param end Итератор, указывающий элемент, идущий за последним элементом последовательности.
	template <typename Iterator>
	void Reverse(Iterator begin, Iterator end)
	{
		--end;
		while (begin < end)
		{
			Swap(*begin++, *end--);
		}
	}

	/// @brief Шаблон функции для поворота элементов последовательности таким образом, что указанный элемент и элементы после него становятся в начало последовательности.
	/// @param begin Итератор, указывающий на первый элемент последовательности.
	/// @param middle Итератор, указывающий элемент, который должен стать первым элементом повернутой последовательности.
	/// @param end Итератор, указывающий элемент, идущий за последним элементом последовательности.
	template <typename Iterator>
	void Rotate(Iterator begin, Iterator middle, Iterator end)
	{
		Reverse(begin, middle);
		Reverse(middle, end);
		Reverse(begin, end);
	}

	/// @brief Шаблон функции, объединяющей две отсортированных части последовательности в одну новую отсортированную последовательность.
	/// @param begin Итератор, указывающий на первый элемент последовательности.
	/// @param pivot Итератор, указывающий первый элемент второй отсортированной подпоследовательности.
	/// @param end Итератор, указывающий элемент, идущий за последним элементом последовательности.
	template <typename Iterator, typename Less>
	void Merge(Iterator begin, Iterator pivot, Iterator end, Less less)
	{
		const int len1 = pivot - begin;
		const int len2 = end - pivot;

		if (len1 == 0 || len2 == 0)
		{
			return;
		}

		if (len1 + len2 == 2)
		{
			if (less(*(begin + 1), *(begin)))
			{
				Swap(*begin, *(begin + 1));
			}
			return;
		}

		Iterator firstCut;
		Iterator secondCut;
		int len2Half;
		if (len1 > len2) 
		{
			const int len1Half = len1 / 2;
			firstCut = begin + len1Half;
			secondCut = LowerBound(pivot, end, *firstCut, less);
			len2Half = secondCut - pivot;
		}
		else 
		{
			len2Half = len2 / 2;
			secondCut = pivot + len2Half;
			firstCut = UpperBound(begin, pivot, *secondCut, less);
		}

		Rotate(firstCut, pivot, secondCut);
		const Iterator newPivot = firstCut + len2Half;
		Merge(begin, firstCut, newPivot, less);
		Merge(newPivot, secondCut, end, less);
	}

	/// @brief Шаблон функции, сортирующий входную последовательность с использованием указанного функтора.
	/// @param begin Итератор, указывающий на первый элемент последовательности.
	/// @param end Итератор, указывающий элемент, идущий за последним элементом последовательности.
	/// @param less Функтор сравнения.
	template <typename Iterator, typename Less>
	void StableSort(Iterator begin, Iterator end, Less less)
	{
		if (begin == end)
		{
			return;
		}

		const int span = end - begin;
		if (span < 2)
		{
			return;
		}

		const Iterator middle = begin + span / 2;
		StableSort(begin, middle, less);
		StableSort(middle, end, less);
		Merge(begin, middle, end, less);
	}
}	// namespace utils 

using namespace utils;
