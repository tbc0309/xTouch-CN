#ifndef XTOUCH_NATIVE_IDF_TEXT_H
#define XTOUCH_NATIVE_IDF_TEXT_H
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#ifndef PROGMEM
#define PROGMEM
#define PSTR(s) (s)
#define F(s) (s)
#define FPSTR(s) (s)
#define pgm_read_byte(p) (*(const uint8_t *)(p))
#define pgm_read_ptr(p) (*(const void *const *)(p))
#define strlen_P strlen
#define strcmp_P strcmp
#define strncmp_P strncmp
#define memcpy_P memcpy
#define memccpy_P memccpy
#define snprintf_P snprintf
#define vsnprintf_P vsnprintf
#define strcpy_P strcpy
#define strncpy_P strncpy
#define strstr_P strstr
#define memcmp_P memcmp
#endif
#ifdef __cplusplus
#include <string>
#include <algorithm>
#include <limits>
#include <type_traits>
#include <cstdarg>
#include <vector>
#include <functional>
using byte = uint8_t;
using boolean = bool;
using PGM_P = const char *;
using PGM_VOID_P = const void *;
constexpr unsigned HEX = 16, DEC = 10, OCT = 8, BIN = 2;
// Application text value, backed only by the C++ standard library.
class String : public std::string {
    template <class N> void number(N n, unsigned base) {
        bool minus = std::is_signed<N>::value && n < 0;
        using U = typename std::make_unsigned<N>::type;
        U value = (minus && base == 10) ? U(0) - U(n) : U(n);
        char buffer[70];
        char *p = buffer + sizeof(buffer);
        *--p = 0;
        if (base < 2 || base > 36)
            base = 10;
        do {
            *--p = "0123456789abcdefghijklmnopqrstuvwxyz"[value % base];
            value /= base;
        } while (value);
        if (minus && base == 10)
            *--p = '-';
        assign(p);
    }

  public:
    using std::string::operator=;
    String() = default;
    String(const char *s) : std::string(s ? s : "") {}
    String(const char *s, size_t n) : std::string(s ? s : "", s ? n : 0) {}
    String(const std::string &s) : std::string(s) {}
    String(bool value) : std::string(value ? "1" : "0") {}
    String(char c) : std::string(1, c) {}
    template <class N,
              typename std::enable_if<std::is_integral<N>::value && !std::is_same<N, char>::value,
                                      int>::type = 0>
    String(N n, unsigned base = 10) {
        number(n, base);
    }
    String(double value, unsigned places = 2) {
        char b[80];
        snprintf(b, sizeof(b), "%.*f", int(std::min(places, 20U)), value);
        assign(b);
    }
    String(float value, unsigned places = 2) : String(double(value), places) {}
    size_t write(uint8_t c) {
        push_back(char(c));
        return 1;
    }
    size_t write(const uint8_t *p, size_t n) {
        append((const char *)p, n);
        return n;
    }
    bool equalsConstantTime(const String &s) const {
        unsigned difference = size() ^ s.size();
        for (size_t i = 0; i < std::min(size(), s.size()); ++i)
            difference |= uint8_t(at(i)) ^ uint8_t(s[i]);
        return difference == 0;
    }
    bool isEmpty() const {
        return empty();
    }
    bool reserve(size_t n) {
        try {
            std::string::reserve(n);
            return true;
        } catch (...) {
            return false;
        }
    }
    bool concat(const char *p, size_t n) {
        try {
            append(p, n);
            return true;
        } catch (...) {
            return false;
        }
    }
    bool concat(const String &s) {
        return concat(s.data(), s.size());
    }
    bool concat(char c) {
        push_back(c);
        return true;
    }
    template <class N,
              typename std::enable_if<std::is_arithmetic<N>::value && !std::is_same<N, char>::value,
                                      int>::type = 0>
    bool concat(N value) {
        return concat(String(value));
    }
    String substring(unsigned first, unsigned last = UINT32_MAX) const {
        first = std::min(first, unsigned(size()));
        last = std::min(last, unsigned(size()));
        if (first > last)
            std::swap(first, last);
        return substr(first, last - first);
    }
    int indexOf(char c, unsigned from = 0) const {
        auto at = find(c, from);
        return at == npos ? -1 : int(at);
    }
    int indexOf(const String &s, unsigned from = 0) const {
        auto at = find(s, from);
        return at == npos ? -1 : int(at);
    }
    int lastIndexOf(char c) const {
        auto at = rfind(c);
        return at == npos ? -1 : int(at);
    }
    int lastIndexOf(const String &s) const {
        auto at = rfind(s);
        return at == npos ? -1 : int(at);
    }
    bool startsWith(const String &s) const {
        return compare(0, s.size(), s) == 0;
    }
    bool endsWith(const String &s) const {
        return size() >= s.size() && compare(size() - s.size(), s.size(), s) == 0;
    }
    bool equalsIgnoreCase(const String &s) const {
        return size() == s.size() && !strcasecmp(c_str(), s.c_str());
    }
    bool equals(const String &s) const {
        return *this == s;
    }
    void toLowerCase() {
        for (char &c : *this)
            c = tolower((unsigned char)c);
    }
    void toUpperCase() {
        for (char &c : *this)
            c = toupper((unsigned char)c);
    }
    void trim() {
        auto first = find_first_not_of(" \t\r\n"), last = find_last_not_of(" \t\r\n");
        if (first == npos)
            clear();
        else
            assign(substr(first, last - first + 1));
    }
    void remove(unsigned at, unsigned n = UINT32_MAX) {
        if (at < size())
            erase(at, std::min(size_t(n), size() - at));
    }
    void replace(const String &from, const String &to) {
        if (from.empty())
            return;
        size_t at = 0;
        while ((at = find(from, at)) != npos) {
            std::string::replace(at, from.size(), to);
            at += to.size();
        }
    }
    void replace(char from, char to) {
        std::replace(begin(), end(), from, to);
    }
    long toInt() const {
        return strtol(c_str(), nullptr, 10);
    }
    float toFloat() const {
        return strtof(c_str(), nullptr);
    }
    char charAt(unsigned n) const {
        return n < size() ? at(n) : 0;
    }
    void setCharAt(unsigned n, char c) {
        if (n < size())
            (*this)[n] = c;
    }
    void toCharArray(char *out, unsigned length, unsigned at = 0) const {
        if (!out || !length)
            return;
        auto s = substring(at);
        size_t n = std::min(size_t(length - 1), s.size());
        memcpy(out, s.data(), n);
        out[n] = 0;
    }
    void getBytes(unsigned char *out, unsigned length, unsigned at = 0) const {
        toCharArray((char *)out, length, at);
    }
    String &operator+=(char c) {
        push_back(c);
        return *this;
    }
    String &operator+=(const String &s) {
        append(s);
        return *this;
    }
    // Arithmetic overloads must outrank the implicit numeric-to-char conversion.
    template <class N,
              typename std::enable_if<std::is_arithmetic<N>::value && !std::is_same<N, char>::value,
                                      int>::type = 0>
    String &operator+=(N value) {
        append(String(value));
        return *this;
    }
    friend String operator+(const String &a, const String &b) {
        String out(a);
        out.append(b);
        return out;
    }
    friend String operator+(const char *a, const String &b) {
        return String(a) + b;
    }
    friend String operator+(const String &a, const char *b) {
        return a + String(b);
    }
    friend String operator+(const String &a, char b) {
        String out(a);
        out.push_back(b);
        return out;
    }
    template <class N,
              typename std::enable_if<std::is_arithmetic<N>::value && !std::is_same<N, char>::value,
                                      int>::type = 0>
    friend String operator+(const String &a, N value) {
        return a + String(value);
    }
    template <class N,
              typename std::enable_if<std::is_arithmetic<N>::value && !std::is_same<N, char>::value,
                                      int>::type = 0>
    friend String operator+(N value, const String &b) {
        return String(value) + b;
    }
};
class Print {
  public:
    virtual ~Print() = default;
    virtual size_t write(uint8_t) = 0;
    virtual size_t write(const uint8_t *p, size_t n) {
        size_t done = 0;
        while (done < n && write(p[done]))
            ++done;
        return done;
    }
    size_t write(const char *s) {
        return write((const uint8_t *)s, strlen(s));
    }
    size_t write(const char *s, size_t n) {
        return write((const uint8_t *)s, n);
    }
    size_t print(const String &s) {
        return write((const uint8_t *)s.data(), s.size());
    }
    template <class T> size_t print(const T &v) {
        return print(String(v));
    }
    size_t println() {
        return write((const uint8_t *)"\r\n", 2);
    }
    template <class T> size_t println(const T &v) {
        return print(v) + println();
    }
    size_t printf(const char *fmt, ...) {
        va_list ap;
        va_start(ap, fmt);
        va_list copy;
        va_copy(copy, ap);
        int n = vsnprintf(nullptr, 0, fmt, copy);
        va_end(copy);
        if (n < 0) {
            va_end(ap);
            return 0;
        }
        std::vector<char> b(n + 1);
        vsnprintf(b.data(), b.size(), fmt, ap);
        va_end(ap);
        return write((uint8_t *)b.data(), n);
    }
};
class Stream : public Print {
  protected:
    unsigned timeoutMs = 1000;

  public:
    virtual int available() = 0;
    virtual int read() = 0;
    virtual int peek() = 0;
    virtual void flush() = 0;
    void setTimeout(unsigned value) {
        timeoutMs = value;
    }
    unsigned getTimeout() const {
        return timeoutMs;
    }
    size_t readBytes(uint8_t *, size_t);
    size_t readBytes(char *out, size_t n) {
        return readBytes((uint8_t *)out, n);
    }
    String readStringUntil(char end);
    String readString();
    bool find(const char *needle);
};
#endif

#endif
