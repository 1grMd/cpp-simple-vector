#pragma once

#include <algorithm>
#include <cassert>
#include <initializer_list>
#include <iostream>
#include <stdexcept>

#include "array_ptr.h"


struct ReserveProxyObj {
	ReserveProxyObj(size_t capacity_to_reserve)
	: capacity_to_reserve(capacity_to_reserve) {
	}
	size_t capacity_to_reserve;
};

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
	return ReserveProxyObj(capacity_to_reserve);
}

template <typename Type>
class SimpleVector {
public:
	using Iterator = Type*;
	using ConstIterator = const Type*;

	SimpleVector() noexcept = default;

	explicit SimpleVector(size_t size, const Type& value = {})
		: size_(size),
		  capacity_(size),
		  array_(ArrayPtr<Type>(size)) {
		if (value != Type{}) {
			std::fill(begin(), end(), value);
		}
	}

	SimpleVector(std::initializer_list<Type> init)
		: size_(init.size()),
		  capacity_(size_),
		  array_(ArrayPtr<Type>(size_)) {
		CopyContainer(init);
	}

	SimpleVector(const SimpleVector& other)
		: size_(other.GetSize()),
		  capacity_(size_),
		  array_(ArrayPtr<Type>(size_)) {
		CopyContainer(other);
	}

	SimpleVector(SimpleVector&& other) noexcept{
		*this = std::move(other);
	}
	SimpleVector(ReserveProxyObj obj)
		: size_(0),
		  capacity_(obj.capacity_to_reserve),
		  array_(ArrayPtr<Type>(capacity_)) {
		Resize(0);
	}

	Type& At(size_t index) {
		if (index >= size_) {
			throw std::out_of_range("");
		}
		return array_[index];
	}

	const Type& At(size_t index) const {
		if (index >= size_) {
			throw std::out_of_range("");
		}
		return array_[index];
	}

	void Clear() noexcept {
		size_ = 0;
	}

	Iterator Erase(ConstIterator pos) {
		assert(pos >= begin() && pos < end());
		size_t index = pos - cbegin();
		std::move(&array_[index+1], &array_[size_], &array_[index]);
		--size_;
		return &array_[index];
	}

	size_t GetSize() const noexcept {
		return size_;
	}

	size_t GetCapacity() const noexcept {
		return capacity_;
	}

	Iterator Insert(ConstIterator pos, const Type& value) {
		Type copy_item = value;
		return Insert(pos, std::move(copy_item));
	}
	Iterator Insert(ConstIterator pos, Type&& item) {
		assert(pos >= begin() && pos <= end());
		size_t index = pos - cbegin();
		if (size_ == capacity_) {
			IncreaseCapacity(1);
		}
		std::move_backward(&array_[index], &array_[size_], &array_[size_+1]);
		array_[index]= std::move(item);
		++size_;
		return &array_[index];
	}

	bool IsEmpty() const noexcept {
		return size_ == 0;
	}

	SimpleVector& operator=(const SimpleVector& rhs) {
		SimpleVector<Type> tmp(rhs);
		this->swap(tmp);
		return *this;
	}

	SimpleVector& operator=(SimpleVector&& rhs) {
		delete[] array_.Get();
		size_ = std::exchange(rhs.size_, 0);
		capacity_ = std::exchange(rhs.capacity_, 0);
		array_ = std::move(rhs.array_);
		return *this;
	}

	Type& operator[](size_t index) noexcept {
		assert(index < size_);
		return array_[index];
	}

	const Type& operator[](size_t index) const noexcept {
		assert(index < size_);
		return array_[index];
	}

	void PopBack() noexcept {
		if (size_ != 0) {
			--size_;
		}
	}

	void PushBack(const Type& value) {
		Type copy_item = value;
		PushBack(std::move(copy_item));
	}
	void PushBack(Type&& item) {
		if (size_ < capacity_) {
			array_[size_] = std::move(item);
			++size_;
		} else {
			IncreaseCapacity(1);
			array_[size_] = std::move(item);
			++size_;
		}
	}

	void Reserve(size_t new_capacity) {
		if (new_capacity > capacity_) {
			capacity_ = 0;
			IncreaseCapacity(new_capacity - size_);
		}
	}

	void Resize(size_t new_size) {
		if (new_size <= size_) {
			size_ = new_size;
		} else if (new_size <= capacity_){
			FillDummies(new_size);
		} else {
			IncreaseCapacity(new_size - size_);
			FillDummies(new_size);
		}
	}

	void swap(SimpleVector& other) noexcept {
		array_.swap(other.array_);
		std::swap(size_, other.size_);
		std::swap(capacity_, other.capacity_);
	}

	Iterator begin() noexcept {
		return array_.Get();
	}

	Iterator end() noexcept {
		return array_.Get() + size_;
	}

	ConstIterator begin() const noexcept {
		return const_cast<Type*>(array_.Get());
	}

	ConstIterator end() const noexcept {
		return const_cast<Type*>(array_.Get() + size_);
	}

	ConstIterator cbegin() const noexcept {
		return const_cast<Type*>(array_.Get());
	}

	ConstIterator cend() const noexcept {
		return const_cast<Type*>(array_.Get() + size_);
	}
private:
	template<typename Container>
	void CopyContainer(Container& container) {
		std::copy(container.begin(), container.end(), begin());
	}

	void FillDummies(size_t new_size) {
		while (size_ < new_size) {
			PushBack(Type{});
		}
	}
	void IncreaseCapacity(size_t value) {
		size_t new_capacity = std::max(size_ + value, 2*capacity_);
		ArrayPtr<Type> tmp_array(new_capacity);
		std::move(&array_[0], &array_[size_], tmp_array.Get());
		array_ = std::move(tmp_array);
		capacity_ = new_capacity;
	}

	size_t size_ = 0;
	size_t capacity_ = 0;
	ArrayPtr<Type> array_;

};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
	return &lhs == &rhs ? true :
			lhs.GetSize() != rhs.GetSize() ? false : !(lhs<rhs) && !(rhs<lhs);
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
	return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
	return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
	return lhs==rhs || lhs < rhs;
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
	return rhs < lhs;
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
	return lhs==rhs || lhs > rhs;
}
