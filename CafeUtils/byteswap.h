#pragma once 

#include <cassert>

template<typename T>
inline T ByteSwap(T value)
{
    if constexpr (sizeof(T) == 1)
        return value;
    else if constexpr (sizeof(T) == 2)
        return static_cast<T>(__builtin_bswap16(static_cast<uint16_t>(value)));
    else if constexpr (sizeof(T) == 4)
        return static_cast<T>(__builtin_bswap32(static_cast<uint32_t>(value)));
    else if constexpr (sizeof(T) == 8)
        return static_cast<T>(__builtin_bswap64(static_cast<uint64_t>(value)));

    assert(false && "Unexpected byte size.");
    return value;
}

template<typename T>
inline void ByteSwapInplace(T& value)
{
    value = ByteSwap(value);
}

template<typename T>
struct be
{
    T value;

    be() : value(0) {}
    be(const T v) { set(v); }

    static T byteswap(T val)
    {
        if constexpr (std::is_same_v<T, double>)
        {
            const uint64_t swapped = ByteSwap(*reinterpret_cast<uint64_t*>(&val));
            return *reinterpret_cast<const T*>(&swapped);
        }
        else if constexpr (std::is_same_v<T, float>)
        {
            const uint32_t swapped = ByteSwap(*reinterpret_cast<uint32_t*>(&val));
            return *reinterpret_cast<const T*>(&swapped);
        }
        else if constexpr (std::is_enum_v<T>)
        {
            const std::underlying_type_t<T> swapped = ByteSwap(*reinterpret_cast<std::underlying_type_t<T>*>(&val));
            return *reinterpret_cast<const T*>(&swapped);
        }
        else
        {
            return ByteSwap(val);
        }
    }

    void set(const T v) { value = byteswap(v); }
    T get() const { return byteswap(value); }

    be& operator| (T val) { set(get() | val); return *this; }
    be& operator& (T val) { set(get() & val); return *this; }
    operator T() const { return get(); }
    be& operator=(T v) { set(v); return *this; }
};

