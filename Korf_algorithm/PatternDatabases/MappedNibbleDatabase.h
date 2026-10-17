#ifndef CUBELAB_KORF_MAPPED_NIBBLE_DATABASE_H
#define CUBELAB_KORF_MAPPED_NIBBLE_DATABASE_H

#include <cstddef>
#include <cstdint>
#include <string>

using namespace std;

// Read-only, memory-mapped four-bit pattern database. Mapping avoids copying
// roughly 758 MiB into every short-lived solver process.
class MappedNibbleDatabase {
public:
    MappedNibbleDatabase(const string& path, size_t entryCount);
    ~MappedNibbleDatabase();

    MappedNibbleDatabase(const MappedNibbleDatabase&) = delete;
    MappedNibbleDatabase& operator=(const MappedNibbleDatabase&) = delete;

    uint8_t get(size_t index) const;
    size_t size() const { return entries; }

private:
    int descriptor = -1;
    size_t entries = 0;
    size_t byteCount = 0;
    const uint8_t* bytes = nullptr;
};

#endif
