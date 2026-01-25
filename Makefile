CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
PKGCONFIG = pkg-config

# Probe dependencies so we can emit a helpful stub build when missing.
HAS_GTK := $(shell $(PKGCONFIG) --exists gtk+-3.0 && echo 1 || echo 0)
HAS_APPINDICATOR := $(shell $(PKGCONFIG) --exists appindicator3-0.1 && echo 1 || echo 0)
HAS_DEPS := $(shell [ "$(HAS_GTK)" = "1" -a "$(HAS_APPINDICATOR)" = "1" ] && echo 1 || echo 0)

ifeq ($(HAS_DEPS),1)
	# Get flags from pkg-config
	GTK_FLAGS = $(shell $(PKGCONFIG) --cflags --libs gtk+-3.0)
	APPINDICATOR_FLAGS = $(shell $(PKGCONFIG) --cflags --libs appindicator3-0.1)
	SOURCE = src/tempmon.cpp
else
	SOURCE = src/tempmon_stub.cpp
endif

TARGET = tempmon

all: check-deps $(TARGET)

check-deps:
	@if [ "$(HAS_DEPS)" != "1" ]; then \
		echo "Warning: GTK/AppIndicator development metadata missing. Building a lightweight stub binary."; \
		echo "Install gtk3 + libappindicator dev packages to build the full tray application."; \
	fi

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
