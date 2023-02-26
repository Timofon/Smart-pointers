#pragma once

#include <type_traits>
#include <memory>

// Me think, why waste time write lot code, when few code do trick.
template <typename F, typename S, bool f_empty = std::is_empty_v<F> && !std::is_final_v<F>,
          bool s_empty = std::is_empty_v<S> && !std::is_final_v<S>>
class CompressedPair;

template <typename F, typename S>
class CompressedPair<F, S, true, false> : F {
public:
    template <typename T1, typename T2>
    CompressedPair(T1&& t1, T2&& t2) : F(std::forward<T1>(t1)), second_(std::forward<T2>(t2)) {
    }

    CompressedPair() {
        second_ = S();
    }

    const F& GetFirst() const {
        return *this;
    };

    F& GetFirst() {
        return *this;
    }

    const S& GetSecond() const {
        return second_;
    };

    S& GetSecond() {
        return second_;
    }

private:
    S second_;
};

template <typename F, typename S>
class CompressedPair<F, S, false, true> : S {
public:
    template <typename T1, typename T2>
    CompressedPair(T1&& t1, T2&& t2) : first_(std::forward<T1>(t1)), S(std::forward<T2>(t2)) {
    }

    CompressedPair() {
        first_ = F();
    };

    const F& GetFirst() const {
        return first_;
    };

    F& GetFirst() {
        return first_;
    }

    const S& GetSecond() const {
        return *this;
    }

    S& GetSecond() {
        return *this;
    }

private:
    F first_;
};

template <typename F, typename S>
class CompressedPair<F, S, false, false> {
public:
    template <typename T1, typename T2>
    CompressedPair(T1&& t1, T2&& t2) : first_(std::forward<T1>(t1)), second_(std::forward<T2>(t2)) {
    }

    CompressedPair() {
        first_ = F();
        second_ = S();
    }

    const F& GetFirst() const {
        return first_;
    };

    F& GetFirst() {
        return first_;
    };

    const S& GetSecond() const {
        return second_;
    };

    S& GetSecond() {
        return second_;
    };

private:
    F first_;
    S second_;
};

template <typename F, typename S>
class CompressedPair<F, S, true, true> : F, S {
public:
    template <typename T1, typename T2>
    CompressedPair(T1&& t1, T2&& t2) : F(std::forward<T1>(t1)), S(std::forward<T2>(t2)) {
    }

    CompressedPair() = default;

    const F& GetFirst() const {
        return *this;
    }

    F& GetFirst() {
        return *this;
    }

    const S& GetSecond() const {
        return *this;
    }

    S& GetSecond() {
        return *this;
    }
};

template <typename F>
class CompressedPair<F, F, true, true> : F {
public:
    template <typename T>
    CompressedPair(T&& t1, T&& t2) : first_(std::forward<T>(t1)), F(std::forward<T>(t2)) {
    }

    CompressedPair() = default;

    const F& GetFirst() const {
        return first_;
    }

    F& GetFirst() {
        return first_;
    }

    const F& GetSecond() const {
        return *this;
    }

    F& GetSecond() {
        return *this;
    }

private:
    F first_;
};

template <typename F>
class CompressedPair<F, F, false, false> {
public:
    template <typename T>
    CompressedPair(T&& t1, T&& t2) : first_(std::forward<T>(t1)), second_(std::forward<T>(t2)) {
    }

    CompressedPair() = default;

    const F& GetFirst() const {
        return first_;
    }

    F& GetFirst() {
        return first_;
    }

    const F& GetSecond() const {
        return second_;
    }

    F& GetSecond() {
        return second_;
    }

private:
    F first_;
    F second_;
};
