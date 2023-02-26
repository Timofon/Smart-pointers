#pragma once

#include "sw_fwd.h"  // Forward declaration
#include "shared.h"

// https://en.cppreference.com/w/cpp/memory/weak_ptr
template <typename X>
class WeakPtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    WeakPtr() : ptr_(nullptr), block_(nullptr){};

    WeakPtr(const WeakPtr& other) : ptr_(other.ptr_), block_(other.block_) {
        if (block_) {
            block_->IncreaseWeakCounter();
        }
    };

    template <typename Y>
    WeakPtr(const WeakPtr<Y>& other) : ptr_(other.ptr_), block_(other.block_) {
        if (block_) {
            block_->IncreaseWeakCounter();
        }
    };

    WeakPtr(WeakPtr&& other) {
        ptr_ = other.ptr_;
        block_ = other.block_;
        other.ptr_ = nullptr;
        other.block_ = nullptr;
    };

    // Demote `SharedPtr`
    // #2 from https://en.cppreference.com/w/cpp/memory/weak_ptr/weak_ptr
    WeakPtr(const SharedPtr<X>& other) : ptr_(other.ptr_), block_(other.block_) {
        if (block_) {
            block_->IncreaseWeakCounter();
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    WeakPtr& operator=(const WeakPtr& other) {
        if (this != &other) {
            Decrement();
            ptr_ = other.ptr_;
            block_ = other.block_;
            if (block_) {
                block_->IncreaseWeakCounter();
            }
        }
        return *this;
    };

    WeakPtr& operator=(WeakPtr&& other) {
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

    ~WeakPtr() {
        Decrement();
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        Decrement();
        ptr_ = nullptr;
        block_ = nullptr;
    };

    void Swap(WeakPtr& other) {
        std::swap(ptr_, other.ptr_);
        std::swap(block_, other.block_);
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    X*& GetPointer() {
        return ptr_;
    };

    ControlBlockBase*& GetRealBlock() {
        return block_;
    }

    size_t UseCount() const {
        if (block_) {
            return block_->GetStrongCounter();
        } else {
            return 0;
        }
    };

    bool Expired() const {
        return UseCount() == 0;
    };

    SharedPtr<X> Lock() const {
        if (block_) {
            if (block_->GetStrongCounter() == 0) {
                return nullptr;
            } else {
                return SharedPtr<X>(*this);
            }
        } else {
            return nullptr;
        }
    };

    void Decrement() {
        if (block_ != nullptr) {
            block_->DecreaseWeakCounter();
            if (block_->GetStrongCounter() == 0 && block_->GetWeakCounter() == 0) {
                delete block_;
            }
        }
    }

    template <typename T>
    friend class SharedPtr;

    template <typename T>
    friend class WeakPtr;

private:
    X* ptr_;
    ControlBlockBase* block_;
};
