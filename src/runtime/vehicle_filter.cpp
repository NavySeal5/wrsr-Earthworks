// Stage 4d prototype: classify vehicle types by their native internal names.
#include <windows.h>
#include "vehicle_filter.h"
#include <cctype>
#include <cstring>

namespace ew {
namespace {
bool ReadableTypeName(const unsigned char* type, char* name, unsigned size, unsigned* category)
{
    if (!type || !name || size < 2 || !category) return false;
    __try {
        *category = *reinterpret_cast<const unsigned*>(type + 0x294);
        unsigned length = 0;
        for (; length + 1 < size && length < 64; ++length) {
            const char value = *reinterpret_cast<const char*>(type + 0x200 + length);
            if (!value) break;
            if (static_cast<unsigned char>(value) < 32 || static_cast<unsigned char>(value) > 126) return false;
            name[length] = value;
        }
        if (!length || length + 1 >= size || length >= 64) return false;
        name[length] = '\0';
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool Contains(const char* text, const char* token)
{
    const size_t tokenSize = std::strlen(token);
    for (const char* at = text; *at; ++at) {
        size_t i = 0;
        while (i < tokenSize && at[i] && static_cast<char>(std::tolower(static_cast<unsigned char>(at[i]))) == token[i]) ++i;
        if (i == tokenSize) return true;
    }
    return false;
}

bool IsAsciiLetter(char value)
{
    value = static_cast<char>(std::tolower(static_cast<unsigned char>(value)));
    return value >= 'a' && value <= 'z';
}

bool ContainsWord(const char* text, const char* token)
{
    const size_t tokenSize = std::strlen(token);
    for (const char* at = text; *at; ++at) {
        if (at != text && IsAsciiLetter(at[-1])) continue;
        size_t i = 0;
        while (i < tokenSize && at[i] && static_cast<char>(std::tolower(static_cast<unsigned char>(at[i]))) == token[i]) ++i;
        if (i == tokenSize && !IsAsciiLetter(at[i])) return true;
    }
    return false;
}
}

bool AllowEarthworksVehicleType(void* vehicleType, char* rejectedType, unsigned rejectedTypeSize)
{
    char name[65]{};
    unsigned category = 0;
    const bool readable = ReadableTypeName(static_cast<const unsigned char*>(vehicleType), name, sizeof(name), &category);
    bool allowed = false;
    if (readable) {
        const bool bulldozer = (ContainsWord(name, "bulldozer") || ContainsWord(name, "buldozer")) && category == 2;
        const bool excavator = (ContainsWord(name, "excavator") || ContainsWord(name, "exvacator")) && category == 2;
        const bool dumper = category == 1 && (ContainsWord(name, "dumper") || ContainsWord(name, "dump_truck")
            || ContainsWord(name, "dumptruck") || Contains(name, "gravel_"));
        const bool bus = category == 1 && (ContainsWord(name, "bus") || ContainsWord(name, "bust"));
        allowed = bulldozer || excavator || dumper || bus;
    }
    if (!allowed && rejectedType && rejectedTypeSize) {
        if (readable) strncpy_s(rejectedType, rejectedTypeSize, name, _TRUNCATE);
        else strcpy_s(rejectedType, rejectedTypeSize, "<unreadable vehicle type>");
    }
    return allowed;
}
}
