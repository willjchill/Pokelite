# CXX = g++
# CXXFLAGS = -std=c++11 -Wall -Wextra -g
# LDFLAGS = -ljsoncpp
# TARGET = pokemon_battle
# SOURCES = main.cpp Battle.cpp Player.cpp Pokemon.cpp Attack.cpp Item.cpp Bag.cpp Type.cpp PokemonData.cpp
# OBJECTS = $(SOURCES:.cpp=.o)
#
# all: $(TARGET)
#
# $(TARGET): $(OBJECTS)
# 	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS)
#
# %.o: %.cpp
# 	$(CXX) $(CXXFLAGS) -c $< -o $@
#
# clean:
# 	rm -f $(OBJECTS) $(TARGET)
#
# .PHONY: all clean

# Switch compiler to the ARM cross-compiler for C++
CXX = arm-linux-gnueabihf-g++

# Add -static so libraries are built INTO the binary
# Add -I. so it finds the local json/ folder headers
# Note: QT uses c++17
CXXFLAGS = -std=c++11 -Wall -Wextra -g -static -I.

# Remove -ljsoncpp because we are compiling it directly now
LDFLAGS = 

TARGET = pokemon_battle

# Add jsoncpp.cpp 
SOURCES = main.cpp Battle.cpp Player.cpp Pokemon.cpp Attack.cpp Item.cpp Bag.cpp Type.cpp PokemonData.cpp jsoncpp.cpp
OBJECTS = $(SOURCES:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all clean
