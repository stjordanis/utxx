//----------------------------------------------------------------------------
/// \file  print.hpp
//----------------------------------------------------------------------------
/// \brief This file contains implementation of printing functions.
//----------------------------------------------------------------------------
// Copyright (c) 2014 Serge Aleynikov <saleyn@gmail.com>
// Created: 2014-09-05
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2014 Serge Aleynikov <saleyn@gmail.com>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

***** END LICENSE BLOCK *****
*/
#pragma once

#include <string>
#include <type_traits>
#include <cstdarg>
#include <iomanip>
#include <utxx/scope_exit.hpp>
#include <utxx/compiler_hints.hpp>
#include <utxx/convert.hpp>
#include <utxx/error.hpp>

namespace utxx {

//------------------------------------------------------------------------------
/// String wrapping class for dealing with possibly non-zero-terminated strings.
//------------------------------------------------------------------------------
template <class Char = char>
struct cstr_wrap {
    cstr_wrap(const Char* a_str, size_t a_sz)
        : m_str(a_str), m_size(a_sz)
    {}

    /// Returned value may not be NULL-terminated
    const Char* c_str() const { return m_str;  }
    size_t      size()  const { return m_size; }

    explicit operator std::string() const { return std::string(m_str, m_size); }
private:
    const Char*  m_str;
    const size_t m_size;
};

//------------------------------------------------------------------------------
/// Output a float to stream formatted with fixed precision
//------------------------------------------------------------------------------
struct fixed {
    fixed(double a_val, int a_digits, int a_precision, char a_fill = ' ')
        : m_value(a_val), m_digits(a_digits), m_precision(a_precision)
        , m_fill(a_fill)
    {}

    fixed(double a_val, int a_precision)
        : m_value(a_val), m_digits(-1), m_precision(a_precision)
        , m_fill(' ')
    {}

    template <typename StreamT>
    inline friend StreamT& operator<<(StreamT& out, const fixed& a) {
        if (a.digits() > -1) {
            char buf[a.digits()];
            utxx::ftoa_right(a.value(), buf, a.digits(), a.precision(), a.fill());
            out.write(buf, a.digits());
        } else {
            char buf[256];
            int n = utxx::ftoa_left(a.value(), buf, sizeof(buf), a.precision(), true);
            if (likely(n >= 0))
                out.write(buf, n);
        }
        return out;
    }

    double value()     const { return m_value;     }
    int    digits()    const { return m_digits;    }
    int    precision() const { return m_precision; }
    char   fill()      const { return m_fill;      }

private:
    double m_value;
    int    m_digits;
    int    m_precision;
    char   m_fill;
};

//------------------------------------------------------------------------------
/// Alignment enumerator
//------------------------------------------------------------------------------
template <int Width, alignment Align, class T>
struct width {
    using TT = typename std::remove_cv<typename std::remove_reference<T>::type>::type;

    // For integers and bool types
    explicit
    width(T a_val, char a_pad = ' ') : m_value(a_val), m_pad(a_pad), m_precision(0)
    {
        static_assert(std::is_integral<TT>::value           ||
                      std::is_same<const char*, TT>::value  ||
                      std::is_same<char*, TT>::value        ||
                      std::is_same<std::string, TT>::value,
                      "Mast be bool, integer or string type");
    }

    // For floating point types
    width(T a_val, int a_precision, char a_pad = ' ')
        : m_value(a_val), m_pad(a_pad), m_precision(a_precision)
    {
        static_assert(std::is_floating_point<T>::value, "Must be floating point");
    }

    width(width&& a)      : m_value(a.value()), m_pad(a.pad()), m_precision(a.precision()) {}
    width(const width& a) : m_value(a.value()), m_pad(a.pad()), m_precision(a.precision()) {}

    template <typename StreamT>
    inline friend StreamT& operator<<(StreamT& out, const width& a) {
        char buf[Width];
        a.write(buf);
        out.write(buf, Width);
        return out;
    }

    /// Write Width characters to \a a_buf.
    /// The argument buffer must have at least Width bytes.
    void write(char* a_buf) const {
        do_write(m_value, a_buf);
    }

    static constexpr const alignment s_align = Align;
    static constexpr const int       s_width = Width;

    T    value()        const { return m_value;     }
    char pad()          const { return m_pad;       }
    int  precision()    const { return m_precision; }
private:
    T    m_value;
    char m_pad;
    int  m_precision;

    void padit(char* p, const char* end) const { while (p < end) *p++ = m_pad; }

    void int_write(T a_value, char* a_buf) const {
        if (Align == RIGHT)
            itoa_right<T, Width>(a_buf, a_value, m_pad);
        else
            itoa_left<T,  Width>(a_buf, a_value, m_pad);
    }

    void do_write(bool a_value, char* a_buf) const {
        do_write(a_value ? "true" : "false", a_buf);
    }

    void do_write(long     a_val, char* a_buf) const { int_write(a_val, a_buf); }
    void do_write(ulong    a_val, char* a_buf) const { int_write(a_val, a_buf); }
    void do_write(int      a_val, char* a_buf) const { int_write(a_val, a_buf); }
    void do_write(uint     a_val, char* a_buf) const { int_write(a_val, a_buf); }
    void do_write(int16_t  a_val, char* a_buf) const { int_write(a_val, a_buf); }
    void do_write(uint16_t a_val, char* a_buf) const { int_write(a_val, a_buf); }

    void do_write(const char* a_value, size_t a_len, char* a_buf) const {
        int len = std::min<int>(Width, a_len);
        if (Align == RIGHT) {
            auto n = Width - len;
            auto e = a_buf + n;
            padit(a_buf, e);
            a_buf  = e;
        }
        memcpy(a_buf, a_value, len);
        if (Align == LEFT)
            padit(a_buf+len, a_buf+Width);
    }

    void do_write(char a_value, char* a_buf) const {
        if (Align == RIGHT) {
            auto e = a_buf + Width - 1;
            padit(a_buf, e);
            a_buf  = e;
        }
        *a_buf = a_value;
        if (Align == LEFT)
            padit(a_buf+1, a_buf+Width);
    }

    void do_write(const char* a_value, char* a_buf) const {
        do_write(a_value, strnlen(a_value, Width), a_buf);
    }

    void do_write(std::string const& a_value, char* a_buf) const {
        do_write(a_value.c_str(), a_value.size(), a_buf);
    }

    void do_write(double a_value, char* a_buf) const {
        if (Align == RIGHT)
            ftoa_right(a_value, a_buf, Width, m_precision, m_pad);
        else {
            int n = ftoa_left(a_value, a_buf, Width, m_precision);
            padit(a_buf + std::max<int>(n, 0), a_buf + Width);
        }
    }

    template <typename U>
    void do_write(U a, char*) const {
        UTXX_THROW_RUNTIME_ERROR
            ("Writing of ", type_to_string<U>(), ' ', a, " not supported!");
    }
};

//------------------------------------------------------------------------------
/// Align and pad given argument
//------------------------------------------------------------------------------
template <int Width, alignment Align, typename T>
width<Width,Align,T> make_width(T a, char a_pad = ' ') { return width<Width, Align, T>(a, a_pad); }

//------------------------------------------------------------------------------
/// Efficient fast printer stream
//------------------------------------------------------------------------------
namespace detail {
    template <size_t N = 256, class Alloc = std::allocator<char>>
    class basic_buffered_print : public Alloc
    {
        using self_t = basic_buffered_print<N, Alloc>;

        mutable char*   m_begin; // mutable so that we can write '\0' in c_str()
        char*           m_pos;
        char*           m_end;
        char            m_data[N];
        int             m_max_src_scope = 3;
        int             m_precision     = 6;

        void deallocate() {
            if (m_begin == m_data) return;
            Alloc::deallocate(m_begin, max_size());
        }

        void do_print(char a) { reserve(1); *m_pos++ = a; }
        void do_print(bool a) {
            static const auto f = [](char* p, bool a) {
                static const std::pair<const char*, int>
                s_vals[] = {{"false", 5}, {"true", 4}};
                auto r = s_vals[a]; memcpy(p, r.first, r.second);
                return r.second;
            };
            reserve(8);
            m_pos += f(m_pos, a);
        }
        void do_print(uint64_t a) {
            reserve(32);
            itoa(a, out(m_pos));
        }
        void do_print(long a) {
            reserve(32);
            itoa(a, out(m_pos));
        }
        void do_print(uint32_t a) {
            reserve(16);
            itoa(a, out(m_pos));
        }
        void do_print(int a) {
            reserve(16);
            itoa(a, out(m_pos));
        }
        void do_print(uint16_t a) {
            reserve(8);
            itoa(a, out(m_pos));
        }
        void do_print(int16_t a) {
            reserve(8);
            itoa(a, out(m_pos));
        }
        void do_print(double a)
        {
            int n = ftoa_left(a, m_pos, capacity(), m_precision, true);
            if (unlikely(n < 0)) {
                n = snprintf(m_pos, capacity(), "%.*f", m_precision, a);
                if (unlikely(m_pos + n > m_end)) {
                    reserve(n);
                    snprintf(m_pos, capacity(), "%.*f", m_precision, a);
                }
            }
            m_pos += n;
        }
        void do_print(fixed&& a) {
            if (a.digits() > -1) {
                reserve(a.digits());
                ftoa_right(a.value(), m_pos, a.digits(), a.precision(), a.fill());
                m_pos += a.digits();
            } else {
                int n = ftoa_left(a.value(), m_pos, capacity(), a.precision(), true);
                if (likely(n >= 0))
                    m_pos += n;
            }
        }
        template <int Width, alignment Align, class T>
        void do_print(width<Width, Align, T>&& a) {
            reserve(Width);
            a.write(m_pos);
            m_pos += Width;
        }
        void do_print(const char* a) {
            if (UNLIKELY(!a)) return;
            const char* p = strchr(a, '\0');
            assert(p);
            size_t n = p - a;
            reserve(n);
            memcpy(m_pos, a, n);
            m_pos += n;
        }
        /*Don't move strings or else it may lead to incidental "stealing" of
          string objects from the owner
        void do_print(std::string&& a) {
            size_t n = a.size();
            reserve(n);
            memcpy(m_pos, a.c_str(), n);
            m_pos += n;
        }
        */
        void do_print(const std::string& a) {
            size_t n = a.size();
            reserve(n);
            memcpy(m_pos, a.c_str(), n);
            m_pos += n;
        }
        template <int M>
        void do_print(const char (&a)[M]) {
            size_t n = strnlen(a, M);
            reserve(n);
            memcpy(m_pos, a, n);
            m_pos += n;
        }
        template <class Char>
        void do_print(const cstr_wrap<Char>& a) {
            reserve(a.size());
            memcpy(m_pos, a.c_str(), a.size());
            m_pos += a.size();
        }
        template <int M>
        void do_print(const std::array<char, M>& a) {
            size_t n = strnlen(a.data(), M);
            reserve(n);
            memcpy(m_pos, a, n);
            m_pos += n;
        }
        void do_print(const src_info& a) {
            reserve(64);
            m_pos = a.to_string(m_pos, capacity(), "[", "]");
        }
        template <typename T>
        void do_print(typename std::enable_if<std::is_pointer<T>::value, T&&>::type a) {
            auto  x = reinterpret_cast<uint64_t>(a);
            char* p = m_pos + 2;
            auto  n = capacity() - 2;
            int   i = itoa_hex(x, p, n);
            if (unlikely(i > n)) {
                reserve(i + 2);
                p = m_pos + 2;
                i = itoa_hex(x, p, n);
            }
            *m_pos++ = '0';
            *m_pos++ = 'x';
            m_pos   += i;
        }
        template <class T>
        void do_print(const T& a) {
            std::stringstream s; s << a;
            print(s.str());
        }
    public:
        explicit basic_buffered_print(const Alloc& a_alloc = Alloc())
            : Alloc(a_alloc)
            , m_begin(m_data)
            , m_pos(m_begin)
            , m_end(m_begin + sizeof(m_data)-1)
        {}

        ~basic_buffered_print() {
            deallocate();
        }

        void reset() {
            deallocate();
            m_begin = m_pos = m_data;
            m_end   = m_begin + sizeof(m_data)-1;
        }

        std::string to_string() const { return std::string(m_begin, size()); }
        const char* str()       const { return m_begin; }
        char*       str()             { return m_begin; }
        const char* c_str()     const { *m_pos = '\0'; return m_begin; }
        size_t      size()      const { return m_pos - m_begin;  }
        const char* pos()       const { return m_pos;            }
        char*&      pos()             { return m_pos;            }
        void        pos(const char* p){ assert(p <= m_end); m_pos = const_cast<char*>(p); }
        size_t      max_size()  const { return m_end - m_begin;  }
        size_t      capacity()  const { return m_end - m_pos;    }
        char&       last()      const { return m_pos == m_begin
                                             ? *m_begin : *(m_pos - 1); }
        char&       last()            { return m_pos == m_begin
                                             ? *m_begin : *(m_pos - 1); }
        const char* end()       const { return m_end;   }

        /// Max depth of src_info scope printed
        void        max_src_scope(int a) { m_max_src_scope = a; }
        /// Precision of floating point (default: 6)
        void        precision    (int a) { m_precision     = a; }

        /// Reserve space in the buffer to hold additional \a a_sz bytes
        void reserve(size_t a_sz) {
            if (m_pos + a_sz <= m_end) return;
            auto sz = max_size() + a_sz + N;
            char* p = Alloc::allocate(sz);
            strncpy(p, m_begin, size());
            m_pos   = p + size();
            m_end   = p + sz;
            if (m_begin != m_data)  delete [] m_begin;
            m_begin = p;
        }

        /// Advance the pointer at the end of the buffer to \a n bytes.
        /// Call this function if the content of this buffer needs to be
        /// written to by external snprintf:
        /// \code
        /// size_t c = buf.capacity();
        /// size_t n = snprintf(buf.str(), c, "%s", "Something");
        /// if (n > c) {
        ///     // write didn't fit - repeat
        ///     buf.reserve(n);
        ///     n = snprintf(buf.str(), c, "%s", "Something");
        /// }
        /// buf.advance(n);
        /// \endcode
        void advance(size_t n) { m_pos += n; }

        /// Remove the trailing character equal to \a ch.
        /// If the buffer doesn't end with \a ch, nothing is removed.
        void chop(char ch = '\n')
        { if (m_begin < m_pos && *(m_pos-1) == ch) --m_pos; }

        /// Remove the trailing character.
        /// If the buffer is empty, nothing is removed.
        void chop() { if (m_begin < m_pos) --m_pos; }

        // Terminal case of template recursion:
        void print() {}

        template <class T, class... Args>
        void print(T&& a, Args&&... args) {
            do_print(std::forward<T>(a));
            print(std::forward<Args>(args)...);
        }

        void sprint(const char* a_str, size_t a_size) {
            reserve(a_size);
            memcpy(m_pos, a_str, a_size);
            m_pos += a_size;
        }

        int vprintf(const char* a_fmt, va_list a_args) {
            int n = vsnprintf(m_pos, capacity(), a_fmt, a_args);
            if (n < 0)
                return n;
            if (size_t(n) > capacity()) {
                reserve(n);
                n = vsnprintf(m_pos, capacity(), a_fmt, a_args);
            }
            m_pos += n;
            return n;
        }

        int printf(const char* a_fmt, ...) {
            va_list args; va_start(args, a_fmt);
            UTXX_SCOPE_EXIT([&args]() { va_end(args); });
            return this->vprintf(a_fmt, args);
        }

        template <typename T>
        friend inline basic_buffered_print<N>&
        operator<< (basic_buffered_print<N>& out, T&& a) {
            out.print(std::forward<T>(a));
            return out;
        }

        // For compatibility with ostream
        self_t& write(const char* a_str, size_t a_size) {
            sprint(a_str, a_size);
            return *this;
        }

        self_t& put(char c) { do_print(c); return *this; }
    };
}

/// Analogous to sprintf, but returns an std::string instead
template <int SizeHint = 256>
std::string print_string(const char* fmt, ...) {
    va_list args; va_start(args, fmt);
    UTXX_SCOPE_EXIT([&args]() { va_end(args); });

    std::string buf;
    buf.resize(SizeHint);
    int n = vsnprintf(const_cast<char*>(buf.c_str()), buf.capacity(), fmt, args);

    if (unlikely(n < 0))
        throw io_error(errno, "print_string(): error formatting arguments");

    buf.resize(n);

    if (unlikely(n > SizeHint)) // String didn't fit - redo
        vsnprintf(const_cast<char*>(buf.c_str()), buf.capacity(), fmt, args);
    return buf;
}

/// Print arguments to string.
/// This function is a faster alternative to printf and std::stringstream.
template <class... Args>
std::string print(Args&&... args) {
    detail::basic_buffered_print<> b;
    b.print(std::forward<Args>(args)...);
    return b.to_string();
}

using buffered_print = detail::basic_buffered_print<>;

} // namespace utxx

// Handling of std::endl, std::ends, std::flush
namespace std {
    /// Write a newline to the buffered_print object
    template <size_t N>
    inline utxx::detail::basic_buffered_print<N>&
    endl(utxx::detail::basic_buffered_print<N>& a_out)
    { a_out << '\n'; return a_out; }

    template <size_t N>
    inline utxx::detail::basic_buffered_print<N>&
    ends(utxx::detail::basic_buffered_print<N>& a_out)
    { a_out << '\0'; return a_out; }

    template <size_t N>
    inline utxx::detail::basic_buffered_print<N>&
    flush(utxx::detail::basic_buffered_print<N>& a_out)
    { return a_out; }
}