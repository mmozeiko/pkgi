#pragma once

#include <fmt/format.h>

#include <stdexcept>

#ifdef PKGI_ENABLE_LOGGING
#define LOG(msg, ...)                 \
    do                                \
    {                                 \
        pkgi_log(msg, ##__VA_ARGS__); \
    } while (0)
#define LOGF(msg, ...)                                     \
    do                                                     \
    {                                                      \
        pkgi_log(fmt::format(msg, ##__VA_ARGS__).c_str()); \
    } while (0)
#else
#define LOG(...) \
    do           \
    {            \
    } while (0)
#define LOGF(...) \
    do            \
    {             \
    } while (0)
#endif

template <typename E = std::runtime_error, typename... Args>
[[nodiscard]] E formatEx(Args&&... args) {
    return E(fmt::format(std::forward<Args>(args)...));
}

void pkgi_log(const char* msg, ...);
