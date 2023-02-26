#pragma once

#include "compressed_pair.h"

#include <cstddef>  // std::nullptr_t
#include <type_traits>

template <typename T>
class CustomDeleter {
public:
    CustomDeleter() = default;

    template <typename U>
    CustomDeleter(const CustomDeleter<U>& other) noexcept {
    }

    void operator()(T* p) const {
        static_assert(sizeof(T) > 0);
        static_assert(!std::is_void<T>::value);
        delete p;
    }
};

template <typename T>
class CustomDeleter<T[]> {
public:
    CustomDeleter() = default;

    template <typename U>
    CustomDeleter(const CustomDeleter<U[]>& other) noexcept {
    }

    void operator()(T* p) const {
        static_assert(sizeof(T) > 0);
        static_assert(!std::is_void<T>::value);
        delete[] p;
    }
};

// Primary template
template <typename T, typename Deleter = CustomDeleter<T>>
class UniquePtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    explicit UniquePtr(T* ptr = nullptr) : pair_(ptr, Deleter{}) {
    }
    UniquePtr(T* ptr, const Deleter& deleter) : pair_(ptr, deleter) {
    }
    UniquePtr(T* ptr, Deleter&& deleter) : pair_(ptr, std::move(deleter)) {
    }

    template <typename U, typename E>
    UniquePtr(UniquePtr<U, E>&& pointer) noexcept
        : pair_(pointer.Release(), std::forward<E>(pointer.GetDeleter())) {
    }

    UniquePtr(UniquePtr&& other) noexcept {
        std::swap(pair_, other.pair_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    UniquePtr& operator=(UniquePtr&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        GetDeleter()(GetPointer());
        GetPointer() = other.GetPointer();
        other.GetPointer() = nullptr;
        GetDeleter() = std::move(other.GetDeleter());
        return *this;
    };

    UniquePtr& operator=(std::nullptr_t) {
        GetDeleter()(GetPointer());
        GetPointer() = nullptr;
        return *this;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~UniquePtr() {
        if (GetPointer()) {
            GetDeleter()(GetPointer());
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    T* Release() {
        T* tmp = GetPointer();
        GetPointer() = nullptr;
        return tmp;
    };

    void Reset(T* ptr = nullptr) {
        T* old_ptr = GetPointer();
        GetPointer() = ptr;
        GetDeleter()(old_ptr);
    };

    void Swap(UniquePtr& other) {
        std::swap(pair_, other.pair_);
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return pair_.GetFirst();
    };

    T*& GetPointer() {
        return pair_.GetFirst();
    }

    Deleter& GetDeleter() {
        return pair_.GetSecond();
    };

    const Deleter& GetDeleter() const {
        return pair_.GetSecond();
    };

    explicit operator bool() const {
        return Get() != nullptr;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Single-object dereference operators

    std::add_lvalue_reference_t<T> operator*() const {
        return *Get();
    };

    T* operator->() const {
        return Get();
    };

private:
    CompressedPair<T*, Deleter> pair_;
};

// Specialization for arrays

template <typename T, typename Deleter>
class UniquePtr<T[], Deleter> {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    explicit UniquePtr(T* ptr = nullptr) : pair_(ptr, Deleter{}) {
    }

    UniquePtr(UniquePtr&& other) noexcept {
        std::swap(pair_, other.pair_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    UniquePtr& operator=(UniquePtr&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        GetDeleter()(GetPointer());
        GetPointer() = other.GetPointer();
        other.GetPointer() = nullptr;
        GetDeleter() = std::move(other.GetDeleter());
        return *this;
    };

    UniquePtr& operator=(std::nullptr_t) {
        GetDeleter()(GetPointer());
        GetPointer() = nullptr;
        return *this;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~UniquePtr() {
        if (GetPointer()) {
            GetDeleter()(GetPointer());
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    T* Release() {
        T* tmp = GetPointer();
        GetPointer() = nullptr;
        return tmp;
    };

    void Reset(T* ptr = nullptr) {
        T* old_ptr = GetPointer();
        GetPointer() = ptr;
        GetDeleter()(old_ptr);
    };

    void Swap(UniquePtr& other) {
        std::swap(pair_, other.pair_);
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return pair_.GetFirst();
    };

    T*& GetPointer() {
        return pair_.GetFirst();
    }

    Deleter& GetDeleter() {
        return pair_.GetSecond();
    };

    const Deleter& GetDeleter() const {
        return pair_.GetSecond();
    };

    explicit operator bool() const {
        return Get() != nullptr;
    };

    T& operator[](size_t ind) {
        return GetPointer()[ind];
    }

    const T& operator[](size_t ind) const {
        return Get()[ind];
    }

private:
    CompressedPair<T*, Deleter> pair_;
};
