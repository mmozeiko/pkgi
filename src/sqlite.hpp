#pragma once

#include <sqlite3.h>

#include <memory>

#define SQLITE_CHECK(call, errstr)                                     \
    do                                                                 \
    {                                                                  \
        auto err = call;                                               \
        if (err != SQLITE_OK)                                          \
            throw std::runtime_error(fmt::format(                      \
                    errstr ":\n{}", sqlite3_errmsg(_sqliteDb.get()))); \
    } while (false)

#define SQLITE_EXEC_RESULT(handle, statement, errstr, cb, data)              \
    do                                                                       \
    {                                                                        \
        char* errmsg;                                                        \
        auto err = sqlite3_exec(handle.get(), statement, cb, data, &errmsg); \
        if (err != SQLITE_OK)                                                \
            throw std::runtime_error(fmt::format(errstr ":\n{}", errmsg));   \
    } while (false)

#define SQLITE_EXEC(handle, statement, errstr) \
    SQLITE_EXEC_RESULT(handle, statement, errstr, nullptr, nullptr)

struct SqliteClose
{
    void operator()(sqlite3* s)
    {
        sqlite3_close(s);
    }
};

using SqlitePtr = std::unique_ptr<sqlite3, SqliteClose>;
