#pragma once

#include "sw_fwd.h"  // Forward declaration

#include <cstddef>  // std::nullptr_t

class ControlBlockBase {
public:
    int GetStrongCounter() {
        return strong_counter_;
    }

    int GetWeakCounter() {
        return weak_counter_;
    }

    void IncreaseStrongCounter() {
        ++strong_counter_;
    }

    void IncreaseWeakCounter() {
        ++weak_counter_;
    }

    void DecreaseStrongCounter() {
        --strong_counter_;
    }

    void DecreaseWeakCounter() {
        --weak_counter_;
    }

    virtual void DeleteObject() = 0;

    virtual ~ControlBlockBase() = default;

private:
    int strong_counter_ = 1;
    int weak_counter_ = 0;
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

class EnableSharedFromThisBase {};

// Look for usage examples in testsblock_->DecreaseCounter();
template <typename X>
class EnableSharedFromThis : public EnableSharedFromThisBase {
public:
    SharedPtr<X> SharedFromThis() {
        return weak_this_.Lock();
    };

    SharedPtr<const X> SharedFromThis() const {
        return weak_this_.Lock();
    };

    WeakPtr<X> WeakFromThis() noexcept {
        return weak_this_;
    };

    WeakPtr<const X> WeakFromThis() const noexcept {
        return WeakPtr<const X>(weak_this_);
    };

    ~EnableSharedFromThis() {
        weak_this_.GetRealBlock()->DecreaseWeakCounter();
        weak_this_.GetRealBlock() = nullptr;
        weak_this_.GetPointer() = nullptr;
    }

    void SetWeakThis(SharedPtr<X> other) {
        weak_this_ = other;
    }

private:
    WeakPtr<X> weak_this_;
};

// https://en.cppreference.com/w/cpp/memory/shared_ptr
template <typename X>
class SharedPtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    SharedPtr() : ptr_(nullptr), block_(nullptr){};

    SharedPtr(std::nullptr_t) : SharedPtr(){};

    explicit SharedPtr(X* ptr) : ptr_(ptr), block_(new ControlBlockPointer<X>(ptr)) {
        if constexpr (std::is_convertible_v<X*, EnableSharedFromThisBase*>) {
            ptr->SetWeakThis(*this);
        }
    };

    template <typename Y>
    SharedPtr(Y* ptr) : ptr_(ptr), block_(new ControlBlockPointer<Y>(ptr)) {
        if constexpr (std::is_convertible_v<X*, EnableSharedFromThisBase*>) {
            ptr->SetWeakThis(*this);
        }
    }

    SharedPtr(const SharedPtr& other) : ptr_(other.ptr_), block_(other.block_) {
        if (block_) {
            block_->IncreaseStrongCounter();
        }
    };

    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other) : ptr_(other.ptr_), block_(other.block_) {
        if (block_) {
            block_->IncreaseStrongCounter();
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
        if constexpr (std::is_convertible_v<X*, EnableSharedFromThisBase*>) {
            ptr_->SetWeakThis(*this);
        }
    };

    // Aliasing constructor
    // #8 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    template <typename T>
    SharedPtr(const SharedPtr<T>& other, X* ptr) : ptr_(ptr), block_(other.GetBlock()) {
        if (block_) {
            block_->IncreaseStrongCounter();
        }
    }

    // Promote `WeakPtr`
    // #11 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    explicit SharedPtr(const WeakPtr<X>& other) : ptr_(other.ptr_), block_(other.block_) {
        if (block_->GetStrongCounter() == 0 || !block_) {
            throw BadWeakPtr();
        } else {
            block_->IncreaseStrongCounter();
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    SharedPtr& operator=(const SharedPtr& other) {
        if (this != &other) {
            Decrement();
            ptr_ = other.ptr_;
            block_ = other.block_;
            if (block_) {
                block_->IncreaseStrongCounter();
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
            block_->DecreaseStrongCounter();
            if (block_->GetStrongCounter() == 0 && block_->GetWeakCounter() != 0) {
                block_->DeleteObject();
                if (block_->GetWeakCounter() == 0) {
                    delete block_;
                }
            } else if (block_->GetStrongCounter() == 0 && block_->GetWeakCounter() == 0) {
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

    X*& GetPointer() {
        return ptr_;
    };

    ControlBlockBase* GetBlock() const {
        return block_;
    }

    ControlBlockBase*& GetRealBlock() {
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
            return block_->GetStrongCounter();
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
inline bool operator==(const SharedPtr<X>& left, const SharedPtr<U>& right) {
    return left.Get() == right.Get() && left.GetBlock() == right.GetBlock();
};

// Allocate memory only once
template <typename X, typename... Args>
SharedPtr<X> MakeShared(Args&&... args) {
    ControlBlockHolder<X>* block = new ControlBlockHolder<X>(std::forward<Args>(args)...);
    return SharedPtr<X>(block);
}
