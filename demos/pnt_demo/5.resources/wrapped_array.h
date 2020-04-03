#pragma once

template <typename T>
struct wrapped_array {
    wrapped_array(T* first, T* last) : begin_ {first}, end_ {last} {}

    T*  begin() const noexcept { return begin_; }
    T*  end() const noexcept { return end_; }

    T* begin_;
    T* end_;
};

template <typename T>
wrapped_array<T> wrap_array(T* first, T* end) noexcept
{ 
    return {first, end};
}
