/* Copyright 2026 Qore Technologies, s.r.o. */
/* Exercise the native default callback through its public MYSQL options. */
#include <mysql.h>
#include <stdio.h>
#include <string.h>

static int check_bounds(MYSQL *mysql, void *context) {
    const unsigned int sizes[] = {0, 1, 2, 13, 14, 512};
    char reference[512];
    int expected_error = mysql->options.local_infile_error(context, reference, sizeof(reference));
    for (unsigned int i = 0; i < sizeof(sizes) / sizeof(*sizes); ++i) {
        unsigned int size = sizes[i];
        unsigned char storage[514];
        memset(storage, 0x5a, sizeof(storage));
        char *buffer = (char *)storage + 1;
        int actual_error = mysql->options.local_infile_error(context, buffer, size);
        if (actual_error != expected_error || storage[0] != 0x5a) {
            return 1;
        }
        for (unsigned int j = size + 1; j < sizeof(storage); ++j) {
            if (storage[j] != 0x5a) {
                fprintf(stderr, "native infile error callback wrote past capacity %u\n", size);
                return 1;
            }
        }
        if (size) {
            size_t length = strlen(reference);
            if (length >= size) {
                length = size - 1;
            }
            if (memcmp(buffer, reference, length) || buffer[length]) {
                return 1;
            }
        }
    }
    return 0;
}

int main(void) {
    MYSQL *mysql = mysql_init(NULL);
    if (!mysql) {
        return 1;
    }
    mysql_set_local_infile_default(mysql);
    int result = check_bounds(mysql, NULL);
    void *context = NULL;
    if (!mysql->options.local_infile_init(&context, "/__qore_nonexistent_infile_test__/missing", mysql)) {
        result = 1;
    } else {
        result |= check_bounds(mysql, context);
    }
    mysql->options.local_infile_end(context);
    mysql_close(mysql);
    mysql_library_end();
    if (!result) {
        puts("native infile error callback: 12 boundary cases passed");
    }
    return result;
}
