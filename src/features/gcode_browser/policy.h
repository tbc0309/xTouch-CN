#pragma once
#include <string>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cctype>
namespace xtouch_files {
struct Entry {
    std::string path, stamp;
    uint32_t size = 0;
    unsigned plate=0;
    uint32_t seconds=0;double grams=0;bool hasTime=false,hasWeight=false;
};
inline bool serialValid(const char *p) {
    if (!p || !*p || strlen(p) > 31)
        return false;
    for (; *p; ++p)
        if (!isalnum((unsigned char)*p) && *p != '-' && *p != '_')
            return false;
    return true;
}
inline bool pathValid(const std::string &p) {
    if (p.empty() || p.size() > 190 || p[0] != '/' || p.back() == '/')
        return false;
    size_t start = 1;
    for (size_t i = 0; i <= p.size(); ++i) {
        if (i < p.size() && ((unsigned char)p[i] < 32 || p[i] == '\\' || p[i] == 127))
            return false;
        if (i == p.size() || p[i] == '/') {
            if (i > 0) {
                auto s = p.substr(start, i - start);
                if (s.empty() || s == "." || s == "..")
                    return false;
            }
            start = i + 1;
        }
    }
    return true;
}
inline bool suffix(const std::string &path, const char *extension) {
    size_t n = strlen(extension);
    if (path.size() < n)
        return false;
    for (size_t i = 0; i < n; ++i)
        if (tolower((unsigned char)path[path.size() - n + i]) != extension[i])
            return false;
    return true;
}
inline bool archive(const std::string &p) {
    return suffix(p, ".3mf");
}
inline bool printable(const std::string &p) {
    return suffix(p, ".gcode") || archive(p);
}
// Cache paths retain the remote hierarchy and filename, escaping FAT-invalid bytes.
inline std::string cachePath(const char *serial, const std::string &remote) {
    if (!serialValid(serial) || !pathValid(remote))
        return {};
    std::string out = std::string("/xtouch/gcode/") + serial + "/cache";
    for (unsigned char c : remote) {
        if (c == '%' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' ||
            c == '|') {
            char hex[4];
            snprintf(hex, sizeof(hex), "%%%02X", c);
            out += hex;
        } else
            out += char(c);
    }
    return out;
}
// Unknown or malformed listing formats must not be interpreted as an empty directory.
inline bool parse(const std::string &line, const std::string &directory, Entry &entry,
                  bool *malformed = nullptr) {
    auto bad = [&](bool value) {
        if (malformed)
            *malformed = value;
    };
    bad(true);
    if (line.empty() || line.rfind("total ", 0) == 0) {
        bad(false);
        return false;
    }
    std::string name, stamp;
    uint64_t size = 0;
    auto separator = line.find(' ');
    std::string facts = line.substr(0, separator);
    for (char &ch : facts)
        ch = tolower((unsigned char)ch);
    if (facts.find("type=") == 0 || facts.find(";type=") != std::string::npos) {
        if (separator == std::string::npos)
            return false;
        name = line.substr(separator + 1);
        bool file = false, directoryEntry = false, sized = false;
        size_t pos = 0;
        while (pos < facts.size()) {
            auto end = facts.find(';', pos);
            if (end == std::string::npos)
                return false;
            auto fact = facts.substr(pos, end - pos);
            if (fact == "type=file")
                file = true;
            if (fact == "type=dir" || fact == "type=cdir" || fact == "type=pdir" ||
                fact.rfind("type=os.unix=slink", 0) == 0)
                directoryEntry = true;
            if (fact.rfind("size=", 0) == 0) {
                char *tail = nullptr;
                const char *digits = fact.c_str() + 5;
                if (!*digits || *digits == '-' || *digits == '+')
                    return false;
                size = strtoull(digits, &tail, 10);
                if (!tail || *tail)
                    return false;
                sized = true;
            }
            if (fact.rfind("modify=", 0) == 0)
                stamp = fact.substr(7);
            pos = end + 1;
        }
        if (directoryEntry) {
            bad(false);
            return false;
        }
        if (!file || !sized)
            return false;
    } else {
        if (line.size() > 10 && (line[0] == 'd' || line[0] == 'l') &&
            (line[1] == 'r' || line[1] == '-') && (line[2] == 'w' || line[2] == '-')) {
            bad(false);
            return false;
        }
        if (line[0] != '-')
            return false;
        std::string fields[8];
        size_t at = 0;
        for (auto &field : fields) {
            while (at < line.size() && line[at] == ' ')
                ++at;
            auto end = line.find(' ', at);
            if (end == std::string::npos)
                return false;
            field = line.substr(at, end - at);
            at = end + 1;
        }
        while (at < line.size() && line[at] == ' ')
            ++at;
        name = line.substr(at);
        char *tail = nullptr;
        if (fields[4].empty() || fields[4][0] == '-' || fields[4][0] == '+')
            return false;
        size = strtoull(fields[4].c_str(), &tail, 10);
        if (!tail || *tail)
            return false;
        stamp = fields[5] + " " + fields[6] + " " + fields[7];
    }
    if (name.empty() || name.find('/') != std::string::npos)
        return false;
    auto path = directory + (directory == "/" ? "" : "/") + name;
    if (!pathValid(path))
        return false;
    bad(false);
    if (!size || size > UINT32_MAX || !printable(path))
        return false;
    entry = {path, stamp, uint32_t(size)};
    return true;
}
} // namespace xtouch_files
