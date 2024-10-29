#pragma once

#include <string>
#include <random>
#include <chrono>
#include <Windows.h>
#include <type_traits>

class Obfuscator {
private:
    static std::mt19937& GetRNG() {
        static std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
        return rng;
    }

public:
    // String encryption
    static std::string EncryptString(const std::string& str) {
        std::string result = str;
        for (char& c : result) {
            c ^= 0x5A;
        }
        return result;
    }

    static std::wstring EncryptString(const std::wstring& str) {
        std::wstring result = str;
        for (wchar_t& c : result) {
            c ^= 0x5A5A;
        }
        return result;
    }

    static std::string DecryptString(const std::string& str) {
        return EncryptString(str); // XOR is symmetric
    }

    static std::wstring DecryptString(const std::wstring& str) {
        return EncryptString(str); // XOR is symmetric
    }

    // Polymorphic addition for signed integral types
    template<typename T>
    static typename std::enable_if<std::is_integral<T>::value&& std::is_signed<T>::value, T>::type
        PolymorphicAdd(T a, T b) {
        std::uniform_int_distribution<int> dist(0, 3);
        switch (dist(GetRNG())) {
        case 0: return static_cast<T>(a + b);
        case 1: return static_cast<T>(-a - b);
        case 2: return static_cast<T>(a - (-b));
        case 3: return static_cast<T>(~(~a - b));
        }
        return static_cast<T>(a + b); // Fallback
    }

    // Polymorphic addition for unsigned integral types
    template<typename T>
    static typename std::enable_if<std::is_integral<T>::value&& std::is_unsigned<T>::value, T>::type
        PolymorphicAdd(T a, T b) {
        std::uniform_int_distribution<int> dist(0, 2);
        switch (dist(GetRNG())) {
        case 0: return static_cast<T>(a + b);
        case 1: return static_cast<T>((a ^ b) + 2 * (a & b));  // Alternative addition
        case 2: return static_cast<T>(~(~a - b));
        }
        return static_cast<T>(a + b); // Fallback
    }

    // Overload for floating-point types
    template<typename T>
    static typename std::enable_if<std::is_floating_point<T>::value, T>::type
        PolymorphicAdd(T a, T b) {
        return a + b;
    }

    // Overload for pointer types
    template<typename T>
    static T* PolymorphicAdd(T* a, std::ptrdiff_t b) {
        return reinterpret_cast<T*>(reinterpret_cast<char*>(a) + b);
    }

    // Obfuscated memory copy
    static void ObfuscatedMemcpy(void* dest, const void* src, size_t n) {
        unsigned char* d = static_cast<unsigned char*>(dest);
        const unsigned char* s = static_cast<const unsigned char*>(src);
        for (size_t i = 0; i < n; ++i) {
            d[i] = s[i] ^ 0x55;
            d[i] ^= 0x55;
        }
    }

    // Control flow obfuscation wrapper
    template<typename Func>
    static auto ObfuscateControlFlow(Func f) {
        return [f]() -> decltype(f()) {
            int state = 0;
            while (true) {
                switch (state) {
                case 0:
                    state = 1;
                    break;
                case 1:
                    return f();
                default:
                    state = 0;
                    break;
                }
            }
            };
    }

    // Junk code insertion
    static void InsertJunkCode() {
        volatile int a = 1, b = 2, c = 3;
        for (int i = 0; i < 10; ++i) {
            a = (b + c) % 100;
            b = (a * c) % 100;
            c = (a + b) % 100;
        }
    }
};

// Macro for easy string decryption
#define OBFUSCATE(str) Obfuscator::DecryptString(Obfuscator::EncryptString(str))