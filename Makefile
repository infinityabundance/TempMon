CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
PKGCONFIG = pkg-config

# Get flags from pkg-config
GTK_FLAGS = $(shell $(PKGCONFIG) --cflags --libs gtk+-3.0)
APPINDICATOR_FLAGS = $(shell $(PKGCONFIG) --cflags --libs appindicator3-0.1)

TARGET = tempmon
SOURCE = src/tempmon.cpp

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CXX) $(CXXFLAGS) $(SOURCE) $(GTK_FLAGS) $(APPINDICATOR_FLAGS) -o $(TARGET)

clean:
	rm -f $(TARGET)

install: $(TARGET)
	install -Dm755 $(TARGET) $(DESTDIR)/usr/local/bin/$(TARGET)
	install -Dm644 tempmon.service $(DESTDIR)/usr/lib/systemd/user/tempmon.service
	@echo "Installed to /usr/local/bin/$(TARGET)"
	@echo "Installed systemd user unit to /usr/lib/systemd/user/tempmon.service"
	@echo "To run at startup, add it to your desktop environment's autostart"

uninstall:
	rm -f $(DESTDIR)/usr/local/bin/$(TARGET)
	rm -f $(DESTDIR)/usr/lib/systemd/user/tempmon.service

.PHONY: all clean install uninstall
