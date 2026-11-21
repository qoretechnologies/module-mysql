// Copyright 2026 Qore Technologies, s.r.o.
// Pure formatting checks use synthetic connection values and never print the input.
#include "../src/MysqlRedact.h"

#include <cstdio>

int main() {
    struct Case {
        const char *input, *user, *password, *database, *expected;
    };
    const Case cases[] = {
        {"user 'SAMPLE'@'localhost', schema `fixture_db`", "sample", nullptr, "fixture_db",
            "user '****'@'localhost', schema `****`"},
        {"sample fixture_db sample", "sample", "", "fixture_db", "**** **** ****"},
        {"samples sample_db #sample", "sample", nullptr, nullptr, "samples sample_db #sample"},
        {"a-b-c", "a-b", nullptr, "b-c", "****"},
        {"'Demo1234' demo1234", nullptr, "Demo1234", nullptr, "'****' demo1234"},
        {"connection refused", nullptr, "connection", nullptr, "connection refused"},
        {"unchanged %s", "", "", nullptr, "unchanged %s"},
        {nullptr, nullptr, nullptr, nullptr, ""},
    };
    for (const Case& c : cases) {
        if (mysql_mask_diagnostic(c.input, c.user, c.password, c.database) != c.expected) {
            std::fprintf(stderr, "diagnostic masking check %td failed\n", &c - cases);
            return 1;
        }
    }
}
