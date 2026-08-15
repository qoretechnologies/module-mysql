/* -*- mode: c++; indent-tabs-mode: nil -*- */
/*
    QoreMysqlBulkLoad.cpp

    Qore Programming Language

    Copyright (C) 2026 Qore Technologies, s.r.o.

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

#include "qore-mysql.h"

#ifdef QORE_MYSQL_HAVE_BULK_LOAD

#include <errmsg.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <memory>
#include <set>
#include <string>
#include <strings.h>
#include <utility>
#include <vector>

namespace {
constexpr const char* QORE_MYSQL_BULK_SOURCE = "qore-native-bulk";
constexpr int64 QORE_MYSQL_STREAM_REPORT_BYTES = 65536;

class QoreMysqlBulkStream {
public:
    QoreMysqlBulkStream(Datasource* ds, bool enabled)
            : ds(ds), active(enabled && ds->sqlMutationObserverActive()) {
    }

    ~QoreMysqlBulkStream() {
        if (started) {
            ExceptionSink xsink;
            ds->reportMutationStreamEnd(consumed, false, &xsink);
        }
    }

    int begin(ExceptionSink* xsink) {
        if (!active) {
            return 0;
        }
        if (ds->reportMutationStreamBegin(0, xsink)) {
            return -1;
        }
        started = true;
        return 0;
    }

    int addBytes(size_t bytes, ExceptionSink* xsink) {
        if (!started) {
            return 0;
        }
        if (bytes > static_cast<size_t>(std::numeric_limits<int64>::max() - consumed)) {
            ok = false;
            xsink->raiseException("DBI:MYSQL:BULK-LOAD-ERROR", "native LOAD DATA byte counter overflow");
            return -1;
        }
        consumed += static_cast<int64>(bytes);
        if (consumed - reported < QORE_MYSQL_STREAM_REPORT_BYTES) {
            return 0;
        }
        reported = consumed;
        if (ds->reportMutationStreamProgress(consumed, xsink)) {
            ok = false;
            return -1;
        }
        return 0;
    }

    void setError() {
        ok = false;
    }

    int finish(bool success, ExceptionSink* xsink) {
        if (!started) {
            return 0;
        }
        started = false;
        ok = ok && success;
        return ds->reportMutationStreamEnd(consumed, ok, xsink);
    }

private:
    Datasource* ds;
    bool active;
    bool started = false;
    bool ok = true;
    int64 consumed = 0;
    int64 reported = 0;
};

static int qoreMysqlQuery(QoreMysqlConnection& conn, const std::string& sql, const char* action,
        ExceptionSink* xsink) {
    if (qore_check_cancel(xsink, action)) {
        return -1;
    }
    if (sql.size() > std::numeric_limits<unsigned long>::max()) {
        xsink->raiseException("DBI:MYSQL:BULK-LOAD-ERROR", "%s statement is too large for the MySQL client", action);
        return -1;
    }
    if (mysql_real_query(conn.db, sql.data(), static_cast<unsigned long>(sql.size()))) {
        xsink->raiseException("DBI:MYSQL:BULK-LOAD-ERROR", "%s failed: %s", action, mysql_error(conn.db));
        return -1;
    }
    return 0;
}

static bool qoreMysqlParseIdentifier(const char* value, bool qualified, std::vector<std::string>& parts,
        ExceptionSink* xsink) {
    parts.clear();
    if (!value || !*value) {
        return false;
    }
    std::string part;
    size_t count = 0;
    bool at_start = true;
    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(value); *p; ++p, ++count) {
        if (count && !(count % 100) && qore_check_cancel(xsink, "MySQL native bulk-load identifier validation")) {
            return false;
        }
        if (*p == '.') {
            if (!qualified || parts.size() || at_start || !p[1]) {
                return false;
            }
            parts.push_back(std::move(part));
            part.clear();
            at_start = true;
            continue;
        }
        if (at_start) {
            if (!(std::isalpha(*p) || *p == '_')) {
                return false;
            }
            at_start = false;
        } else if (!(std::isalnum(*p) || *p == '_' || *p == '$')) {
            return false;
        }
        if (part.size() == 64) {
            return false;
        }
        part.push_back(static_cast<char>(*p));
    }
    if (at_start) {
        return false;
    }
    parts.push_back(std::move(part));
    return true;
}

static std::string qoreMysqlQuoteIdentifier(const std::vector<std::string>& parts) {
    std::string result;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i) {
            result.push_back('.');
        }
        result.push_back('`');
        result.append(parts[i]);
        result.push_back('`');
    }
    return result;
}

static std::string qoreMysqlEscapeLiteral(MYSQL* db, const char* value) {
    size_t size = strlen(value);
    std::string result((size * 2) + 1, '\0');
    unsigned long length = mysql_real_escape_string(db, &result[0], value, static_cast<unsigned long>(size));
    result.resize(length);
    return result;
}

static int qoreMysqlGetBoolOption(const QoreHashNode* options, const char* name, bool default_value,
        bool& result, ExceptionSink* xsink) {
    result = default_value;
    if (!options) {
        return 0;
    }
    QoreValue value = options->getKeyValue(name);
    if (value.isNothing()) {
        return 0;
    }
    if (value.getType() != NT_BOOLEAN) {
        xsink->raiseException("DBI:MYSQL:BULK-LOAD-ERROR", "native bulk-load option '%s' must be boolean", name);
        return -1;
    }
    result = value.getAsBool();
    return 0;
}

static int qoreMysqlAppendEscaped(std::string& output, const char* data, size_t size, ExceptionSink* xsink) {
    for (size_t i = 0; i < size; ++i) {
        if (i && !(i % 100) && qore_check_cancel(xsink, "MySQL native bulk-load value escaping")) {
            return -1;
        }
        switch (static_cast<unsigned char>(data[i])) {
            case 0:
                output.append("\\0");
                break;
            case '\b':
                output.append("\\b");
                break;
            case '\t':
                output.append("\\t");
                break;
            case '\n':
                output.append("\\n");
                break;
            case '\r':
                output.append("\\r");
                break;
            case 26:
                output.append("\\Z");
                break;
            case '\\':
                output.append("\\\\");
                break;
            default:
                output.push_back(data[i]);
                break;
        }
    }
    return 0;
}

static int qoreMysqlAppendDate(QoreMysqlConnection& conn, const DateTimeNode* date, std::string& output,
        ExceptionSink* xsink) {
    if (date->isRelative()) {
        xsink->raiseException("DBI:MYSQL:BULK-LOAD-ERROR",
            "relative date/time values are not supported by native MySQL bulk loading");
        return -1;
    }
    qore_tm info;
    date->getInfo(conn.getTZ(), info);
    if (info.year < 1 || info.year > 9999) {
        xsink->raiseException("DBI:MYSQL:BULK-LOAD-ERROR",
            "native MySQL bulk-load date/time values must have a year from 1 through 9999; got %d", info.year);
        return -1;
    }
    char buffer[32];
    int count = snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d.%06d", info.year,
        info.month, info.day, info.hour, info.minute, info.second, info.us);
    if (count <= 0 || static_cast<size_t>(count) >= sizeof(buffer)) {
        xsink->raiseException("DBI:MYSQL:BULK-LOAD-ERROR", "could not format a native bulk-load date/time value");
        return -1;
    }
    output.append(buffer, static_cast<size_t>(count));
    return 0;
}

static int qoreMysqlAppendValue(QoreMysqlConnection& conn, QoreValue value, std::string& output,
        ExceptionSink* xsink) {
    if (value.isNullOrNothing()) {
        output.append("\\N");
        return 0;
    }
    switch (value.getType()) {
        case NT_BOOLEAN:
            output.push_back(value.getAsBool() ? '1' : '0');
            return 0;
        case NT_INT: {
            char buffer[32];
            int count = snprintf(buffer, sizeof(buffer), QLLD, value.getAsBigInt());
            if (count <= 0 || static_cast<size_t>(count) >= sizeof(buffer)) {
                xsink->raiseException("DBI:MYSQL:BULK-LOAD-ERROR", "could not format a native bulk-load integer");
                return -1;
            }
            output.append(buffer, static_cast<size_t>(count));
            return 0;
        }
        case NT_FLOAT: {
            double number = value.getAsFloat();
            if (!std::isfinite(number)) {
                xsink->raiseException("DBI:MYSQL:BULK-LOAD-ERROR",
                    "non-finite floats are not supported by native MySQL bulk loading");
                return -1;
            }
            char buffer[64];
            int count = snprintf(buffer, sizeof(buffer), "%.17g", number);
            if (count <= 0 || static_cast<size_t>(count) >= sizeof(buffer)) {
                xsink->raiseException("DBI:MYSQL:BULK-LOAD-ERROR", "could not format a native bulk-load float");
                return -1;
            }
            output.append(buffer, static_cast<size_t>(count));
            return 0;
        }
        case NT_NUMBER: {
            QoreString number;
            value.get<const QoreNumberNode>()->getStringRepresentation(number);
            output.append(number.c_str(), number.size());
            return 0;
        }
        case NT_STRING: {
            QoreStringValueHelper string(value, conn.ds.getQoreEncoding(), xsink);
            if (*xsink) {
                return -1;
            }
            return qoreMysqlAppendEscaped(output, string->c_str(), string->size(), xsink);
        }
        case NT_BINARY: {
            const BinaryNode* binary = value.get<const BinaryNode>();
            return qoreMysqlAppendEscaped(output, static_cast<const char*>(binary->getPtr()), binary->size(), xsink);
        }
        case NT_DATE:
            return qoreMysqlAppendDate(conn, value.get<const DateTimeNode>(), output, xsink);
        default:
            xsink->raiseException("DBI:MYSQL:BULK-LOAD-ERROR",
                "cannot serialize Qore type '%s' for native MySQL bulk loading", value.getTypeName());
            return -1;
    }
}
}

class QoreMysqlBulkLoadState {
public:
    QoreMysqlBulkLoadState(QoreMysqlConnection& conn, bool stream_bounds)
            : conn(conn), stream(&conn.ds, stream_bounds) {
    }

    ~QoreMysqlBulkLoadState() {
        if (savepoint_active) {
            ExceptionSink xsink;
            terminate(false, &xsink);
        }
    }

    int initialize(const QoreString* table, const QoreListNode* column_list, ExceptionSink* xsink) {
        std::vector<std::string> table_parts;
        if (!qoreMysqlParseIdentifier(table->c_str(), true, table_parts, xsink)) {
            return *xsink ? -1 : 1;
        }
        quoted_table = qoreMysqlQuoteIdentifier(table_parts);
        if (!column_list->size() || column_list->size() > 1017) {
            xsink->raiseException("DBI:MYSQL:BULK-LOAD-ERROR",
                "native MySQL bulk loading requires between 1 and 1017 columns; got %zu", column_list->size());
            return -1;
        }

        std::set<std::string> used_columns;
        std::string quoted_columns;
        ConstListIterator iterator(column_list);
        while (iterator.next()) {
            if (columns.size() && !(columns.size() % 100)
                && qore_check_cancel(xsink, "MySQL native bulk-load column validation")) {
                return -1;
            }
            QoreValue value = iterator.getValue();
            if (value.getType() != NT_STRING) {
                xsink->raiseException("DBI:MYSQL:BULK-LOAD-ERROR",
                    "native bulk-load column %zu has type '%s'; expected string", columns.size() + 1,
                    value.getTypeName());
                return -1;
            }
            // note: column names are short enough to be held in inline short string storage (ex:
            // "id"), which has no QoreStringNode; the helper must stay in scope while "name" is used
            QoreStringDataHelper name(value);
            std::vector<std::string> parts;
            if (!qoreMysqlParseIdentifier(name.c_str(), false, parts, xsink)) {
                return *xsink ? -1 : 1;
            }
            std::string normalized(parts[0]);
            std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });
            if (!used_columns.insert(normalized).second) {
                xsink->raiseException("DBI:MYSQL:BULK-LOAD-ERROR",
                    "column '%s' occurs more than once in the native bulk-load column list", name.c_str());
                return -1;
            }
            columns.emplace_back(name.c_str(), name.size());
            if (!quoted_columns.empty()) {
                quoted_columns.append(", ");
            }
            quoted_columns.append(qoreMysqlQuoteIdentifier(parts));
        }

        int available = checkAvailability(table_parts, xsink);
        if (available) {
            return available;
        }

        const char* charset = mysql_character_set_name(conn.db);
        if (!charset || !*charset) {
            xsink->raiseException("DBI:MYSQL:BULK-LOAD-ERROR", "MySQL returned an empty connection character set");
            return -1;
        }
        size_t charset_size = 0;
        for (const unsigned char* p = reinterpret_cast<const unsigned char*>(charset); *p; ++p, ++charset_size) {
            if (charset_size == 64) {
                xsink->raiseException("DBI:MYSQL:BULK-LOAD-ERROR",
                    "MySQL returned an overlong connection character set name");
                return -1;
            }
            if (!(std::isalnum(*p) || *p == '_')) {
                xsink->raiseException("DBI:MYSQL:BULK-LOAD-ERROR",
                    "MySQL returned unsafe connection character set name '%s'", charset);
                return -1;
            }
        }

        load_sql = "LOAD DATA LOCAL INFILE '";
        load_sql.append(QORE_MYSQL_BULK_SOURCE);
        load_sql.append("' INTO TABLE ");
        load_sql.append(quoted_table);
        load_sql.append(" CHARACTER SET ");
        load_sql.append(charset);
        load_sql.append(" FIELDS TERMINATED BY '\\t' ESCAPED BY '\\\\' LINES TERMINATED BY '\\n' (");
        load_sql.append(quoted_columns);
        load_sql.push_back(')');

        char suffix[2 * sizeof(uintptr_t) + 1];
        int count = snprintf(suffix, sizeof(suffix), "%zx", reinterpret_cast<uintptr_t>(this));
        if (count <= 0 || static_cast<size_t>(count) >= sizeof(suffix)) {
            xsink->raiseException("DBI:MYSQL:BULK-LOAD-ERROR", "could not create a native bulk-load savepoint");
            return -1;
        }
        savepoint = "qore_mysql_bulk_";
        savepoint.append(suffix, static_cast<size_t>(count));
        if (qoreMysqlQuery(conn, "SAVEPOINT " + savepoint, "native bulk-load savepoint", xsink)) {
            return -1;
        }
        savepoint_active = true;
        return stream.begin(xsink) ? -1 : 0;
    }

    int loadRows(const QoreHashNode* rows, ExceptionSink* xsink) {
        if (rows->size() != columns.size()) {
            return fail("native bulk-load block has an incorrect number of columns", xsink);
        }

        int64 row_count = -1;
        std::vector<QoreValue> values;
        std::vector<const QoreListNode*> lists;
        values.reserve(columns.size());
        lists.reserve(columns.size());
        for (size_t i = 0; i < columns.size(); ++i) {
            if (i && !(i % 100) && qore_check_cancel(xsink, "MySQL native bulk-load row validation")) {
                failed = true;
                stream.setError();
                return -1;
            }
            bool exists = false;
            QoreValue value = rows->getKeyValueExistence(columns[i].c_str(), exists);
            if (!exists) {
                failed = true;
                stream.setError();
                xsink->raiseException("DBI:MYSQL:BULK-LOAD-ERROR",
                    "native bulk-load block is missing column '%s'", columns[i].c_str());
                return -1;
            }
            values.push_back(value);
            if (value.getType() == NT_LIST) {
                const QoreListNode* list = value.get<const QoreListNode>();
                int64 size = list->size();
                if (row_count < 0) {
                    row_count = size;
                } else if (row_count != size) {
                    failed = true;
                    stream.setError();
                    xsink->raiseException("DBI:MYSQL:BULK-LOAD-ERROR",
                        "native bulk-load column '%s' has " QLLD " rows; expected " QLLD,
                        columns[i].c_str(), size, row_count);
                    return -1;
                }
                lists.push_back(list);
            } else {
                lists.push_back(nullptr);
            }
        }
        if (row_count < 0) {
            row_count = 1;
        }
        if (!row_count) {
            return 0;
        }
        if (static_cast<uint64_t>(row_count) > std::numeric_limits<size_t>::max()) {
            return fail("native bulk-load block is too large for this platform", xsink);
        }

        payload.clear();
        for (size_t row = 0; row < static_cast<size_t>(row_count); ++row) {
            if (row && !(row % 100) && qore_check_cancel(xsink, "MySQL native bulk-load serialization")) {
                failed = true;
                stream.setError();
                return -1;
            }
            for (size_t column = 0; column < columns.size(); ++column) {
                if (column && !(column % 100)
                    && qore_check_cancel(xsink, "MySQL native bulk-load row serialization")) {
                    failed = true;
                    stream.setError();
                    return -1;
                }
                if (column) {
                    payload.push_back('\t');
                }
                QoreValue value = lists[column] ? lists[column]->retrieveEntry(row) : values[column];
                if (qoreMysqlAppendValue(conn, value, payload, xsink)) {
                    failed = true;
                    stream.setError();
                    return -1;
                }
            }
            payload.push_back('\n');
        }
        return feed(xsink);
    }

    int terminate(bool success, ExceptionSink* xsink) {
        bool keep = success && !failed;
        bool cleanup_ok = true;
        if (savepoint_active) {
            if (!keep && qoreMysqlQuery(conn, "ROLLBACK TO SAVEPOINT " + savepoint,
                    "native bulk-load rollback", xsink)) {
                cleanup_ok = false;
            }
            if (qoreMysqlQuery(conn, "RELEASE SAVEPOINT " + savepoint,
                    "native bulk-load savepoint release", xsink)) {
                cleanup_ok = false;
            }
            savepoint_active = false;
        }
        if (stream.finish(keep && cleanup_ok, xsink)) {
            cleanup_ok = false;
        }
        return cleanup_ok && !*xsink ? 0 : -1;
    }

    bool acceptsSource(const char* filename) const {
        return feed_active && filename && !strcmp(filename, QORE_MYSQL_BULK_SOURCE);
    }

    int localInit(void** pointer, const char* filename) {
        if (!acceptsSource(filename)) {
            *pointer = nullptr;
            return 1;
        }
        offset = 0;
        *pointer = this;
        return 0;
    }

    int localRead(char* buffer, unsigned int buffer_size) {
        if (!feed_active || !feed_xsink) {
            return -1;
        }
        if (qore_check_cancel(feed_xsink, "MySQL native LOCAL INFILE callback")) {
            failed = true;
            stream.setError();
            return -1;
        }
        if (offset == payload.size()) {
            return 0;
        }
        size_t count = std::min<size_t>(buffer_size, payload.size() - offset);
        count = std::min<size_t>(count, std::numeric_limits<int>::max());
        if (stream.addBytes(count, feed_xsink)) {
            failed = true;
            return -1;
        }
        memcpy(buffer, payload.data() + offset, count);
        offset += count;
        return static_cast<int>(count);
    }

    int localError(char* error, unsigned int error_size) const {
        if (error && error_size) {
            snprintf(error, error_size, "Qore denied or interrupted the LOCAL INFILE data source");
        }
        return CR_UNKNOWN_ERROR;
    }

private:
    int checkAvailability(const std::vector<std::string>& table_parts, ExceptionSink* xsink) {
        const char* default_schema = conn.ds.getDBName();
        const std::string& table_name = table_parts.back();
        std::string schema = table_parts.size() == 2 ? table_parts[0]
            : (default_schema ? default_schema : "");
        std::string query = "SELECT @@GLOBAL.local_infile, ENGINE FROM information_schema.TABLES WHERE "
            "TABLE_SCHEMA='" + qoreMysqlEscapeLiteral(conn.db, schema.c_str()) + "' AND TABLE_NAME='"
            + qoreMysqlEscapeLiteral(conn.db, table_name.c_str()) + "'";
        if (qoreMysqlQuery(conn, query, "native bulk-load eligibility query", xsink)) {
            return -1;
        }
        std::unique_ptr<MYSQL_RES, void (*)(MYSQL_RES*)> result(mysql_store_result(conn.db), mysql_free_result);
        if (!result) {
            xsink->raiseException("DBI:MYSQL:BULK-LOAD-ERROR",
                "could not read native bulk-load eligibility: %s", mysql_error(conn.db));
            return -1;
        }
        MYSQL_ROW row = mysql_fetch_row(result.get());
        if (!row) {
            // Temporary tables are not exposed through information_schema.TABLES.  Let automatic
            // mode preserve ordinary insert behavior; required mode will report unavailability.
            return 1;
        }
        if (!row[0] || (!strcmp(row[0], "0") || !strcasecmp(row[0], "OFF"))) {
            return 1;
        }
        if (!row[1] || (strcasecmp(row[1], "InnoDB") && strcasecmp(row[1], "XtraDB"))) {
            return 1;
        }
        return 0;
    }

    int feed(ExceptionSink* xsink) {
        offset = 0;
        feed_xsink = xsink;
        feed_active = true;
        int rc = mysql_real_query(conn.db, load_sql.data(), static_cast<unsigned long>(load_sql.size()));
        feed_active = false;
        feed_xsink = nullptr;
        if (rc) {
            failed = true;
            stream.setError();
            if (!*xsink) {
                xsink->raiseException("DBI:MYSQL:BULK-LOAD-ERROR", "native LOAD DATA failed: %s",
                    mysql_error(conn.db));
            }
            return -1;
        }
        unsigned int warnings = mysql_warning_count(conn.db);
        if (warnings) {
            failed = true;
            stream.setError();
            xsink->raiseException("DBI:MYSQL:BULK-LOAD-ERROR",
                "native LOAD DATA produced %u warnings; the block will be rolled back", warnings);
            return -1;
        }
        return 0;
    }

    int fail(const char* description, ExceptionSink* xsink) {
        failed = true;
        stream.setError();
        xsink->raiseException("DBI:MYSQL:BULK-LOAD-ERROR", "%s", description);
        return -1;
    }

    QoreMysqlConnection& conn;
    std::vector<std::string> columns;
    std::string quoted_table;
    std::string load_sql;
    std::string savepoint;
    std::string payload;
    size_t offset = 0;
    ExceptionSink* feed_xsink = nullptr;
    bool savepoint_active = false;
    bool feed_active = false;
    bool failed = false;
    QoreMysqlBulkStream stream;
};

namespace {
static int qoreMysqlLocalInit(void** pointer, const char* filename, void* userdata) {
    QoreMysqlConnection* conn = static_cast<QoreMysqlConnection*>(userdata);
    if (!conn || !conn->bulk_load) {
        *pointer = nullptr;
        return 1;
    }
    return conn->bulk_load->localInit(pointer, filename);
}

static int qoreMysqlLocalRead(void* pointer, char* buffer, unsigned int buffer_size) {
    QoreMysqlBulkLoadState* state = static_cast<QoreMysqlBulkLoadState*>(pointer);
    return state ? state->localRead(buffer, buffer_size) : -1;
}

static void qoreMysqlLocalEnd(void*) {
}

static int qoreMysqlLocalError(void* pointer, char* error, unsigned int error_size) {
    QoreMysqlBulkLoadState* state = static_cast<QoreMysqlBulkLoadState*>(pointer);
    if (state) {
        return state->localError(error, error_size);
    }
    if (error && error_size) {
        snprintf(error, error_size, "Qore denied local file access");
    }
    return CR_UNKNOWN_ERROR;
}
}

void QoreMysqlConnection::installLocalInfileHandler() {
    mysql_set_local_infile_handler(db, qoreMysqlLocalInit, qoreMysqlLocalRead, qoreMysqlLocalEnd,
        qoreMysqlLocalError, this);
}

int QoreMysqlConnection::bulkLoadBegin(const QoreString* table, const QoreListNode* columns,
        const QoreHashNode* options, ExceptionSink* xsink) {
    if (bulk_load) {
        xsink->raiseException("DBI:MYSQL:BULK-LOAD-ERROR", "a native MySQL bulk load is already active");
        return -1;
    }
    bool stream_bounds;
    if (qoreMysqlGetBoolOption(options, "stream_bounds", true, stream_bounds, xsink)) {
        return -1;
    }
    std::unique_ptr<QoreMysqlBulkLoadState> state(new QoreMysqlBulkLoadState(*this, stream_bounds));
    int rc = state->initialize(table, columns, xsink);
    if (rc) {
        return rc;
    }
    bulk_load = state.release();
    return 0;
}

int QoreMysqlConnection::bulkLoadRows(const QoreHashNode* rows, ExceptionSink* xsink) {
    if (!bulk_load) {
        xsink->raiseException("DBI:MYSQL:BULK-LOAD-ERROR", "no native MySQL bulk load is active");
        return -1;
    }
    return bulk_load->loadRows(rows, xsink);
}

int QoreMysqlConnection::bulkLoadEnd(bool success, ExceptionSink* xsink) {
    if (!bulk_load) {
        xsink->raiseException("DBI:MYSQL:BULK-LOAD-ERROR", "no native MySQL bulk load is active");
        return -1;
    }
    std::unique_ptr<QoreMysqlBulkLoadState> state(bulk_load);
    bulk_load = nullptr;
    return state->terminate(success, xsink);
}

#endif // QORE_MYSQL_HAVE_BULK_LOAD
