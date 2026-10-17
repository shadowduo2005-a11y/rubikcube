#include "PhasePatternDatabases.h"
#include "../Solver/GroupMoves.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

using namespace std;

namespace {

constexpr array<char, 8> MAGIC = {'C', 'U', 'B', 'E', 'P', 'D', 'B', '2'};
constexpr uint32_t FORMAT_VERSION = 2;
constexpr size_t HEADER_BYTES = 32;
constexpr uint8_t UNKNOWN = numeric_limits<uint8_t>::max();

const array<string, 4> FILE_NAMES = {
    "phase1_edges.pdb",
    "phase2_corners_eslice.pdb",
    "phase3_tetrads_slices.pdb",
    "phase4_halfturn.pdb",
};

constexpr array<size_t, 4> STATE_COUNTS = {
    PhaseCoordinates::GROUP1_STATES,
    PhaseCoordinates::GROUP2_STATES,
    PhaseCoordinates::GROUP3_STATES,
    PhaseCoordinates::GROUP4_STATES,
};

// Exact maxima for the four quotient searches. Phase 3 is 13 because its
// database is a multi-source distance to all 12 representations of G3.
constexpr array<uint8_t, 4> EXPECTED_MAXIMUMS = {7, 10, 13, 15};
constexpr array<size_t, 4> EXPECTED_ZERO_COUNTS = {1, 1, 12, 1};

uint32_t checksum(const vector<uint8_t>& values) {
    uint32_t hash = 2166136261U;
    for (uint8_t value : values) {
        hash ^= value;
        hash *= 16777619U;
    }
    return hash;
}

template <typename Value>
void writeValue(ofstream& output, const Value& value) {
    output.write(reinterpret_cast<const char*>(&value), sizeof(Value));
}

template <typename Value>
bool readValue(ifstream& input, Value& value) {
    return static_cast<bool>(input.read(reinterpret_cast<char*>(&value), sizeof(Value)));
}

bool loadDatabase(
    const filesystem::path& path,
    uint32_t phase,
    size_t expectedCount,
    uint8_t expectedMaximum,
    ExactPatternDatabase& database) {
    ifstream input(path, ios::binary | ios::ate);
    if (!input) return false;
    const streamoff fileSize = input.tellg();
    if (fileSize != static_cast<streamoff>(HEADER_BYTES + expectedCount)) return false;
    input.seekg(0);

    array<char, 8> magic{};
    uint32_t version = 0;
    uint32_t storedPhase = 0;
    uint64_t storedCount = 0;
    uint32_t storedMaximum = 0;
    uint32_t storedChecksum = 0;
    if (!input.read(magic.data(), magic.size()) ||
        !readValue(input, version) || !readValue(input, storedPhase) ||
        !readValue(input, storedCount) || !readValue(input, storedMaximum) ||
        !readValue(input, storedChecksum)) {
        return false;
    }
    if (magic != MAGIC || version != FORMAT_VERSION || storedPhase != phase ||
        storedCount != expectedCount || storedMaximum != expectedMaximum) {
        return false;
    }

    vector<uint8_t> values(expectedCount);
    if (!input.read(reinterpret_cast<char*>(values.data()), values.size())) return false;
    if (checksum(values) != storedChecksum ||
        *max_element(values.begin(), values.end()) != expectedMaximum ||
        find(values.begin(), values.end(), UNKNOWN) != values.end()) {
        return false;
    }
    database.assign(std::move(values));
    return true;
}

void saveDatabase(
    const filesystem::path& path,
    uint32_t phase,
    const ExactPatternDatabase& database) {
    ofstream output(path, ios::binary | ios::trunc);
    if (!output) throw runtime_error("Cannot create pattern database: " + path.string());

    output.write(MAGIC.data(), MAGIC.size());
    writeValue(output, FORMAT_VERSION);
    writeValue(output, phase);
    const vector<uint8_t>& values = database.rawDistances();
    const uint64_t count = values.size();
    const uint32_t maximum = database.maximumDistance();
    const uint32_t storedChecksum = checksum(values);
    writeValue(output, count);
    writeValue(output, maximum);
    writeValue(output, storedChecksum);
    output.write(
        reinterpret_cast<const char*>(values.data()), values.size());
    if (!output) throw runtime_error("Could not finish pattern database: " + path.string());
}

struct BuildResult {
    vector<uint8_t> distances;
    vector<Cube> representatives;
};

template <typename Indexer>
BuildResult buildAbstractDatabase(
    size_t stateCount,
    const vector<Cube>& seeds,
    const vector<Cube::Move>& moves,
    Indexer indexer,
    bool retainRepresentatives) {
    BuildResult result;
    result.distances.assign(stateCount, UNKNOWN);
    result.representatives.resize(stateCount);
    vector<uint32_t> pending(stateCount);
    size_t head = 0;
    size_t tail = 0;

    for (const Cube& seed : seeds) {
        const uint32_t index = indexer(seed);
        if (index >= stateCount) throw logic_error("Pattern database seed is out of range");
        if (result.distances[index] != UNKNOWN) continue;
        result.distances[index] = 0;
        result.representatives[index] = seed;
        pending[tail++] = index;
    }

    while (head < tail) {
        const uint32_t index = pending[head++];
        const Cube current = result.representatives[index];
        const uint8_t nextDistance = static_cast<uint8_t>(result.distances[index] + 1);

        for (Cube::Move move : moves) {
            Cube child = current;
            child.apply(move);
            const uint32_t childIndex = indexer(child);
            if (childIndex >= stateCount) {
                throw logic_error("Generated pattern database index is out of range");
            }
            if (result.distances[childIndex] != UNKNOWN) continue;
            result.distances[childIndex] = nextDistance;
            result.representatives[childIndex] = std::move(child);
            pending[tail++] = childIndex;
        }
    }

    if (tail != stateCount) {
        throw runtime_error(
            "Pattern database generation reached " + to_string(tail) + " of " +
            to_string(stateCount) + " coordinates");
    }

    // A quotient coordinate must have a consistent unit-cost graph. Verify
    // every generated representative before accepting or saving the table.
    for (size_t index = 0; index < stateCount; ++index) {
        const Cube current = result.representatives[index];
        for (Cube::Move move : moves) {
            Cube child = current;
            child.apply(move);
            const uint32_t childIndex = indexer(child);
            const int difference =
                static_cast<int>(result.distances[index]) -
                static_cast<int>(result.distances[childIndex]);
            if (difference < -1 || difference > 1) {
                throw runtime_error("Pattern database violates a unit-cost transition");
            }
        }
    }
    if (!retainRepresentatives) {
        result.representatives.clear();
        result.representatives.shrink_to_fit();
    }
    return result;
}

}  // namespace

uint8_t ExactPatternDatabase::distance(uint32_t index) const {
    if (index >= distances.size()) throw out_of_range("Pattern database index is out of range");
    return distances[index];
}

uint8_t ExactPatternDatabase::maximumDistance() const {
    if (distances.empty()) return 0;
    return *max_element(distances.begin(), distances.end());
}

PhasePatternDatabases::PhasePatternDatabases(const string& databaseDirectory) {
    if (!loadAll(databaseDirectory)) {
        generateAll();
        saveAll(databaseDirectory);
        generated = true;
    }
}

bool PhasePatternDatabases::loadAll(const string& directory) {
    array<ExactPatternDatabase, 4> loaded;
    for (size_t phase = 0; phase < loaded.size(); ++phase) {
        if (!loadDatabase(
                filesystem::path(directory) / FILE_NAMES[phase],
                static_cast<uint32_t>(phase + 1),
                STATE_COUNTS[phase], EXPECTED_MAXIMUMS[phase], loaded[phase])) {
            return false;
        }
        if (count(
                loaded[phase].rawDistances().begin(),
                loaded[phase].rawDistances().end(), uint8_t{0}) !=
            static_cast<ptrdiff_t>(EXPECTED_ZERO_COUNTS[phase])) {
            return false;
        }
    }
    databases = std::move(loaded);
    return true;
}

void PhasePatternDatabases::generateAll() {
    const vector<Cube> solvedSeed = {Cube{}};

    BuildResult group4Build = buildAbstractDatabase(
        PhaseCoordinates::GROUP4_STATES, solvedSeed, GroupMoves::group3(),
        [](const Cube& cube) { return PhaseCoordinates::group4(cube); }, true);
    databases[3].distances = std::move(group4Build.distances);

    vector<Cube> group3Seeds;
    vector<bool> seenGroup3(PhaseCoordinates::GROUP3_STATES, false);
    for (const Cube& cube : group4Build.representatives) {
        const uint32_t index = PhaseCoordinates::group3(cube);
        if (!seenGroup3[index]) {
            seenGroup3[index] = true;
            group3Seeds.push_back(cube);
        }
    }
    group4Build.representatives.clear();
    group4Build.representatives.shrink_to_fit();
    if (group3Seeds.size() != 12) {
        throw runtime_error(
            "Expected 12 G3 goal coordinates, generated " +
            to_string(group3Seeds.size()));
    }

    BuildResult group3Build = buildAbstractDatabase(
        PhaseCoordinates::GROUP3_STATES, group3Seeds, GroupMoves::group2(),
        [](const Cube& cube) { return PhaseCoordinates::group3(cube); }, false);
    databases[2].distances = std::move(group3Build.distances);

    BuildResult group2Build = buildAbstractDatabase(
        PhaseCoordinates::GROUP2_STATES, solvedSeed, GroupMoves::group1(),
        [](const Cube& cube) { return PhaseCoordinates::group2(cube); }, false);
    databases[1].distances = std::move(group2Build.distances);

    BuildResult group1Build = buildAbstractDatabase(
        PhaseCoordinates::GROUP1_STATES, solvedSeed, GroupMoves::group0(),
        [](const Cube& cube) { return PhaseCoordinates::group1(cube); }, false);
    databases[0].distances = std::move(group1Build.distances);

    for (size_t phase = 0; phase < databases.size(); ++phase) {
        if (databases[phase].maximumDistance() != EXPECTED_MAXIMUMS[phase]) {
            throw runtime_error(
                "Phase " + to_string(phase + 1) +
                " pattern database has unexpected maximum distance " +
                to_string(databases[phase].maximumDistance()));
        }
        if (count(
                databases[phase].rawDistances().begin(),
                databases[phase].rawDistances().end(), uint8_t{0}) !=
            static_cast<ptrdiff_t>(EXPECTED_ZERO_COUNTS[phase])) {
            throw runtime_error(
                "Phase " + to_string(phase + 1) +
                " pattern database has an incorrect goal-set size");
        }
    }
}

void PhasePatternDatabases::saveAll(const string& directory) const {
    filesystem::create_directories(directory);
    for (size_t phase = 0; phase < databases.size(); ++phase) {
        saveDatabase(
            filesystem::path(directory) / FILE_NAMES[phase],
            static_cast<uint32_t>(phase + 1), databases[phase]);
    }
}
