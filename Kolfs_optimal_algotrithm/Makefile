CXX = g++
CXXFLAGS = -O3 -std=c++17
INCLUDES = -I.

SRCS = main.cpp \
       Model/RubiksCube.cpp \
       Model/RubiksCube1dArray.cpp \
       Model/RubiksCube3dArray.cpp \
       Model/RubiksCubeBitboard.cpp \
       PatternDatabases/CornerPatternDatabase.cpp \
       PatternDatabases/CornerDBMaker.cpp \
       PatternDatabases/PatternDatabase.cpp \
       PatternDatabases/NibbleArray.cpp \
       PatternDatabases/math.cpp

# Note: NibbleArray.cpp was not listed in my previous g++ command but checking if it exists
# if it doesn't I will fix it. Let's list it anyway or remove it if it fails.

OBJS = $(SRCS:.cpp=.o)
TARGET = rubiks_solver

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
