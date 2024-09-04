# Nazwa pliku wykonywalnego
TARGET = program

# Kompilator i flagi kompilatora
CXX = g++
CXXFLAGS = -Wall -Wextra -O2 `pkg-config --cflags opencv4`

# Flagi linkera
LDFLAGS = `pkg-config --libs opencv4` -lpng

# Pliki źródłowe
SRCS = main.cpp

# Pliki obiektowe
OBJS = $(SRCS:.cpp=.o)

# Reguła domyślna
all: $(TARGET)

# Reguła budowania pliku wykonywalnego
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

# Reguła budowania plików obiektowych
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Reguła czyszczenia plików wynikowych
clean:
	rm -f $(OBJS) $(TARGET)

# Reguła uruchamiania programu
run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
