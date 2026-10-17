#ifndef CUBELAB_THISTLETHWAITE_PHASE_PATTERN_DATABASES_H
#define CUBELAB_THISTLETHWAITE_PHASE_PATTERN_DATABASES_H

#include "PhaseCoordinates.h"

#include <array>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

using namespace std;

class ExactPatternDatabase {
public:
    uint8_t distance(uint32_t index) const;
    size_t size() const { return distances.size(); }
    uint8_t maximumDistance() const;
    const vector<uint8_t>& rawDistances() const { return distances; }
    void assign(vector<uint8_t> values) { distances = std::move(values); }

private:
    vector<uint8_t> distances;

    friend class PhasePatternDatabases;
};

class PhasePatternDatabases {
public:
    explicit PhasePatternDatabases(const string& databaseDirectory);

    const ExactPatternDatabase& group1() const { return databases[0]; }
    const ExactPatternDatabase& group2() const { return databases[1]; }
    const ExactPatternDatabase& group3() const { return databases[2]; }
    const ExactPatternDatabase& group4() const { return databases[3]; }

    bool generatedDuringConstruction() const { return generated; }

private:
    array<ExactPatternDatabase, 4> databases;
    bool generated = false;

    bool loadAll(const string& directory);
    void generateAll();
    void saveAll(const string& directory) const;
};

#endif
