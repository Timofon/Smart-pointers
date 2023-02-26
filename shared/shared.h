#pragma once

#include "sw_fwd.h"  // Forward declaration

#include <cstddef>  // std::nullptr_t

class ControlBlockBase {
public:
    int GetCounter() {
        return counter_;
    }

    void IncreaseCounter() {
        ++counter_;
    }

    void DecreaseCounter() {
        --counter_;
    }

    virtual void DeleteObject() = 0;

    virtual ~ControlBlockBase() = default;

private:
    int counter_ = 1;
};

template <typename T>
class ControlBlockPointer : public ControlBlockBase {
public:
    ControlBlockPointer(T* ptr) : ptr_(ptr) {
    }

    T* GetPointer() {
        return ptr_;
    }

    void DeleteObject() override {
        delete ptr_;
    }

    ~ControlBlockPointer() override{};

private:
    T* ptr_;
};

template <typename T>
class ControlBlockHolder : public ControlBlockBase {
public:
    template <typename... Args>
    ControlBlockHolder(Args&&... args) {
        new (&storage_) T(std::forward<Args>(args)...);
    }

    T* GetPointer() {
        return reinterpret_cast<T*>(&storage_);
    }

    void DeleteObject() override {
        GetPointer()->~T();
    }

    ~ControlBlockHolder() override{};

private:
    std::aligned_storage_t<sizeof(T), alignof(T)> storage_;
};

// https://en.cppreference.com/w/cpp/memory/shared_ptr
template <typename X>
class SharedPtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    SharedPtr() : ptr_(nullptr), block_(nullptr){};

    SharedPtr(std::nullptr_t) : SharedPtr(){};

    explicit SharedPtr(X* ptr) : ptr_(ptr), block_(new ControlBlockPointer<X>(ptr)){};

    template <typename Y>
    SharedPtr(Y* ptr) : ptr_(ptr), block_(new ControlBlockPointer<Y>(ptr)) {
    }

    SharedPtr(const SharedPtr& other) : ptr_(other.ptr_), block_(other.block_) {
        if (block_) {
            block_->IncreaseCounter();
        }
    };

    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other) : ptr_(other.ptr_), block_(other.block_) {
        if (block_) {
            block_->IncreaseCounter();
        }
    }

    SharedPtr(SharedPtr&& other) {
        ptr_ = other.ptr_;
        block_ = other.block_;
        other.ptr_ = nullptr;
        other.block_ = nullptr;
    };

    template <typename Y>
    SharedPtr(SharedPtr<Y>&& other) {
        ptr_ = other.ptr_;
        block_ = other.block_;
        other.ptr_ = nullptr;
        other.block_ = nullptr;
    }

    SharedPtr(ControlBlockHolder<X>* block) {
        block_ = block;
        ptr_ = block->GetPointer();
    };

    // Aliasing constructor
    // #8 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    template <typename T>
    SharedPtr(const SharedPtr<T>& other, X* ptr) : ptr_(ptr), block_(other.GetBlock()) {
        block_->IncreaseCounter();
    }

    // Promote `WeakPtr`
    // #11 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    explicit SharedPtr(const WeakPtr<X>& other);

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    SharedPtr& operator=(const SharedPtr& other) {
        if (this != &other) {
            Decrement();
            ptr_ = other.ptr_;
            block_ = other.block_;
            if (block_) {
                block_->IncreaseCounter();
            }
        }
        return *this;
    };

    SharedPtr& operator=(SharedPtr&& other) {
        if (this == &other) {
            return *this;
        }
        Decrement();
        ptr_ = other.ptr_;
        block_ = other.block_;
        other.ptr_ = nullptr;
        other.block_ = nullptr;
        return *this;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~SharedPtr() {
        Decrement();
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        Decrement();
        ptr_ = nullptr;
        block_ = nullptr;
    };

    void Reset(X* ptr) {
        Decrement();
        ptr_ = ptr;
        block_ = new ControlBlockPointer<X>(ptr);
    };

    template <typename Y>
    void Reset(Y* ptr) {
        Decrement();
        ptr_ = ptr;
        block_ = new ControlBlockPointer<Y>(ptr);
    }

    void Swap(SharedPtr& other) {
        std::swap(ptr_, other.ptr_);
        std::swap(block_, other.block_);
    };

    void Decrement() {
        if (block_ != nullptr) {
            block_->DecreaseCounter();
            if (block_->GetCounter() == 0) {
                block_->DeleteObject();
                delete block_;
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    X* Get() const {
        return ptr_;
    };

    ControlBlockBase* GetBlock() const {
        return block_;
    }

    X& operator*() const {
        return *ptr_;
    };

    X* operator->() const {
        return ptr_;
    };

    size_t UseCount() const {
        if (block_) {
            return block_->GetCounter();
        } else {
            return 0;
        }
    };

    explicit operator bool() const {
        return ptr_ != nullptr;
    };

    template <typename T>
    friend class SharedPtr;

    template <typename T>
    friend class WeakPtr;

private:
    X* ptr_;
    ControlBlockBase* block_;
};

template <typename X, typename U>
inline bool operator==(const SharedPtr<X>& left, const SharedPtr<U>& right);

// Allocate memory only once
template <typename X, typename... Args>
SharedPtr<X> MakeShared(Args&&... args) {
    ControlBlockHolder<X>* block = new ControlBlockHolder<X>(std::forward<Args>(args)...);
    return SharedPtr<X>(block);
}

// Look for usage examples in testsblock_->DecreaseCounter();
template <typename X>
class EnableSharedFromThis {
public:
    SharedPtr<X> SharedFromThis();
    SharedPtr<const X> SharedFromThis() const;

    WeakPtr<X> WeakFromThis() noexcept;
    WeakPtr<const X> WeakFromThis() const noexcept;
};
