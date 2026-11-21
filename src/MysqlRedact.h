/* -*- mode: c++; indent-tabs-mode: nil -*- */
/*
    MysqlRedact.h

    Qore Programming Language

    Copyright (C) 2003 - 2026 Qore Technologies, s.r.o.

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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/


#ifndef QORE_MYSQL_REDACT_H
#define QORE_MYSQL_REDACT_H

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace qore_mysql_redact {
static inline bool name_char(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')
        || c == '_' || c == '$' || c == '#';
}

static inline char lower_ascii(char c) {
    return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c;
}

// Match the Oracle driver's policy: marking an ordinary word or a number in a known
// message template could itself disclose the password.
static inline bool password_maskable(std::string_view pass) {
    if (pass.size() < 8) {
        return false;
    }
    bool letter = false, digit = false;
    for (char c : pass) {
        if (lower_ascii(c) >= 'a' && lower_ascii(c) <= 'z') {
            letter = true;
        } else if (c >= '0' && c <= '9') {
            digit = true;
        } else {
            letter = digit = false;
        }
        if (letter && digit) {
            return true;
        }
    }
    return false;
}
}

static inline std::string mysql_mask_diagnostic(const char* message, const char* user, const char* password,
        const char* database) {
    using namespace qore_mysql_redact;
    std::string_view text(message ? message : "");
    std::vector<bool> masked(text.size());
    auto mark = [&](std::string_view value, bool exact) {
        if (value.empty() || value.size() > text.size()) {
            return;
        }
        for (size_t i = 0; i <= text.size() - value.size(); ++i) {
            size_t end = i + value.size();
            if ((i && name_char(value.front()) && name_char(text[i - 1]))
                    || (end < text.size() && name_char(value.back()) && name_char(text[end]))) {
                continue;
            }
            if (std::equal(value.begin(), value.end(), text.begin() + i, [exact](char a, char b) {
                    return exact ? a == b : lower_ascii(a) == lower_ascii(b);
                })) {
                std::fill(masked.begin() + i, masked.begin() + end, true);
            }
        }
    };
    mark(user ? user : "", false);
    mark(database ? database : "", false);
    if (password && password_maskable(password)) {
        mark(password, true);
    }
    // Coalesce overlapping matches, including when the end of the user name is the start of the database name.
    std::string result;
    for (size_t i = 0; i < text.size(); ++i) {
        if (!masked[i]) {
            result += text[i];
        } else if (!i || !masked[i - 1]) {
            result += "****";
        }
    }
    return result;
}

#endif
