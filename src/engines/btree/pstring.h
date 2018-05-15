/*
 * Copyright 2017-2018, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PERSISTENT_PSTRING_H
#define PERSISTENT_PSTRING_H

#include <string.h>
#include <stdexcept>

template<size_t CAPACITY>
class pstring {
    static const size_t BUFFER_SIZE = CAPACITY + 1;
public:
    pstring(const std::string& s = "\0") {
        init(s.c_str(), s.size());
    }

    pstring(const pstring& other) {
        init(other.c_str(), other.size());
    }

    pstring& operator=(const pstring& other) {
        init(other.c_str(), other.size());
        return *this;
    }

    pstring& operator=(const std::string& s) {
        init(s.c_str(), s.size());
        return *this;
    }

    const char* c_str() const {
        return str;
    }

    size_t size() const {
        return _size;
    }

    char* begin() {
        return str;
    }

    char* end() {
        return str + _size;
    }

    const char* begin() const {
        return str;
    }

    const char* end() const {
        return str + _size;
    }

private:
    void init(const char* src, size_t size) {
        if(size > CAPACITY) throw std::length_error("size exceed pstring capacity");
        memcpy(str, src, size);
        str[size] = '\0';
        _size = size;
    }

    char str[BUFFER_SIZE];
    size_t _size;
};

template<size_t size>
inline bool operator<(const pstring<size>& lhs, const pstring<size>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template<size_t size>
inline bool operator>(const pstring<size>& lhs, const pstring<size>& rhs) {
    return std::lexicographical_compare(rhs.begin(), rhs.end(), lhs.begin(), lhs.end());
}

template<size_t size>
inline bool operator==(const pstring<size>& lhs, const pstring<size>& rhs) {
    return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template<size_t size>
std::ostream& operator<<(std::ostream& os, const pstring<size>& obj) {
    return os << obj.c_str();           
}

#endif // PERSISTENT_PSTRING_H
