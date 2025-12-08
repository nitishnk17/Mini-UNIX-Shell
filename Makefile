# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++11 -pedantic

# Target executable
TARGET = shell

# Source files
SOURCES = shell.cpp parser.cpp executor.cpp helpers.cpp redirection.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Header files (for dependency tracking)
HEADERS = parser.h executor.h helpers.h redirection.h

# Default target
all: $(TARGET)

# Link object files to create executable
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS)

# Compile source files to object files
%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TARGET)

# Rebuild everything
rebuild: clean all

# Run the shell
run: $(TARGET)
	./$(TARGET)

# Phony targets
.PHONY: all clean rebuild run
