#include "MappedNibbleDatabase.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <stdexcept>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace std;

MappedNibbleDatabase::MappedNibbleDatabase(const string& path, size_t entryCount)
    : entries(entryCount), byteCount(entryCount / 2 + 1) {
    descriptor = open(path.c_str(), O_RDONLY);
    if (descriptor < 0) {
        throw runtime_error("Pattern database not found: " + path);
    }

    struct stat fileStatus {};
    if (fstat(descriptor, &fileStatus) != 0 ||
        static_cast<size_t>(fileStatus.st_size) != byteCount) {
        close(descriptor);
        descriptor = -1;
        throw runtime_error("Pattern database has the wrong size: " + path);
    }

    void* mapping = mmap(nullptr, byteCount, PROT_READ, MAP_SHARED, descriptor, 0);
    if (mapping == MAP_FAILED) {
        const string reason = strerror(errno);
        close(descriptor);
        descriptor = -1;
        throw runtime_error("Could not map pattern database " + path + ": " + reason);
    }
    bytes = static_cast<const uint8_t*>(mapping);
}

MappedNibbleDatabase::~MappedNibbleDatabase() {
    if (bytes != nullptr) munmap(const_cast<uint8_t*>(bytes), byteCount);
    if (descriptor >= 0) close(descriptor);
}

uint8_t MappedNibbleDatabase::get(size_t index) const {
    if (index >= entries) throw out_of_range("Pattern database index is out of range");
    const uint8_t packed = bytes[index / 2];
    return index % 2 == 0 ? static_cast<uint8_t>(packed >> 4)
                          : static_cast<uint8_t>(packed & 0x0F);
}
