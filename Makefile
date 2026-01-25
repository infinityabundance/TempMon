CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
PKGCONFIG = pkg-config

# Get flags from pkg-config
GTK_FLAGS = $(shell $(PKGCONFIG) --cflags --libs gtk+-3.0)
APPINDICATOR_FLAGS = $(shell $(PKGCONFIG) --cflags --libs appindicator3-0.1)

TARGET = tempmon
SOURCE = tempmon.cpp

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CXX) $(CXXFLAGS) $(SOURCE) $(GTK_FLAGS) $(APPINDICATOR_FLAGS) -o $(TARGET)

clean:
	rm -f $(TARGET)

install: $(TARGET)
	install -Dm755 $(TARGET) $(DESTDIR)/usr/local/bin/$(TARGET)
	@echo "Installed to /usr/local/bin/$(TARGET)"
	@echo "To run at startup, add it to your desktop environment's autostart"

uninstall:
	rm -f $(DESTDIR)/usr/local/bin/$(TARGET)

.PHONY: all clean install uninstall
