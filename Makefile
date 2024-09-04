# Nazwa katalogu binarnego
BINDIR = bin

# Nazwy plików wykonywalnych
TARGETS = mtpFrame mtpSeries

# Kompilator i flagi kompilatora
CXX = g++
CXXFLAGS = -Wall -Wextra -O2 `pkg-config --cflags opencv4`

# Flagi linkera
LDFLAGS = `pkg-config --libs opencv4` -lpng

# Pliki źródłowe
SRCS_mtpFrame = mtpFrame.cpp ConfigReader.cpp
SRCS_mtpSeries = mtpSeries.cpp ConfigReader.cpp

# Pliki obiektowe
OBJS_mtpFrame = $(SRCS_mtpFrame:.cpp=.o)
OBJS_mtpSeries = $(SRCS_mtpSeries:.cpp=.o)

# Katalog docelowy dla instalacji
INSTALLDIR = /usr/local/bin

# Reguła domyślna
all: $(BINDIR) $(BINDIR)/mtpFrame $(BINDIR)/mtpSeries

# Reguły budowania plików wykonywalnych
$(BINDIR)/mtpFrame: $(OBJS_mtpFrame)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS_mtpFrame) $(LDFLAGS)

$(BINDIR)/mtpSeries: $(OBJS_mtpSeries)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS_mtpSeries) $(LDFLAGS)

# Reguła budowania plików obiektowych
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Tworzenie katalogu binarnego
$(BINDIR):
	mkdir -p $(BINDIR)

# Reguła czyszczenia plików wynikowych
clean:
	rm -f $(OBJS_mtpFrame) $(OBJS_mtpSeries)
	rm -f $(BINDIR)/mtpFrame $(BINDIR)/mtpSeries

# Reguła instalacji
install: all
	install -m 0755 $(BINDIR)/mtpFrame $(INSTALLDIR)/mtpFrame
	install -m 0755 $(BINDIR)/mtpSeries $(INSTALLDIR)/mtpSeries

# Reguła deinstalacji
uninstall:
	rm -f $(INSTALLDIR)/mtpFrame $(INSTALLDIR)/mtpSeries

# Reguła uruchamiania programu mtpFrame
runFrame: $(BINDIR)/mtpFrame
	./$(BINDIR)/mtpFrame

# Reguła uruchamiania programu mtpSeries
runSeries: $(BINDIR)/mtpSeries
	./$(BINDIR)/mtpSeries

.PHONY: all clean install uninstall runFrame runSeries
