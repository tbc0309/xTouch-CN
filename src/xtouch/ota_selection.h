#pragma once
#include <cctype>
#include <climits>
#include <cstddef>
#include <cstring>
namespace XTouchOtaSelection {
struct Version {
    unsigned major, minor, patch;
    bool test = false;
};
inline bool parse(const char *s, Version &out) {
    if (!s)
        return false;
    unsigned values[3]{};
    for (unsigned n = 0; n < 3; ++n) {
        if (*s < '0' || *s > '9')
            return false;
        while (*s >= '0' && *s <= '9') {
            unsigned digit = unsigned(*s++ - '0');
            if (values[n] > (UINT_MAX - digit) / 10)
                return false;
            values[n] = values[n] * 10 + digit;
        }
        if (n < 2 && *s++ != '.')
            return false;
    }
    const bool test = std::strcmp(s, "-test") == 0;
    if (*s && !test)
        return false;
    out = {values[0], values[1], values[2], test};
    return true;
}
inline int compare(Version a, Version b) {
    if (a.major != b.major)
        return a.major > b.major ? 1 : -1;
    if (a.minor != b.minor)
        return a.minor > b.minor ? 1 : -1;
    if (a.patch != b.patch)
        return a.patch > b.patch ? 1 : -1;
    if (a.test != b.test)
        return a.test ? -1 : 1;
    return 0;
}
} // namespace XTouchOtaSelection
