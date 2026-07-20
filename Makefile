# TurtleClass Makefile
# Windows Client Selection: win32 (default) or winui3

WINDOWS_CLIENT ?= win32

.PHONY: all clean test build-win32 build-winui3

all: configure build

configure:
@echo "Configuring for $(WINDOWS_CLIENT) client..."
cmake -B build -DWINDOWS_CLIENT=$(WINDOWS_CLIENT)

build: configure
@echo "Building TurtleClass ($(WINDOWS_CLIENT) client)..."
cmake --build build -j$(shell nproc 2>/dev/null || echo 4)

build-win32:
$(MAKE) WINDOWS_CLIENT=win32

build-winui3:
$(MAKE) WINDOWS_CLIENT=winui3

test: build
@echo "Running tests..."
cd build && ctest --output-on-failure

clean:
rm -rf build

install: build
@echo "Installing TurtleClass..."
cd build && cmake --install .

# Development helpers
debug:
cmake -B build-debug -DWINDOWS_CLIENT=$(WINDOWS_CLIENT) -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug

run-server: build
./build/TurtleClassServer

run-simulator: build
./build/SyncSimulator

# Package
package: build
@echo "Creating installation package..."
cd deploy && ./build_installer.sh $(WINDOWS_CLIENT)

help:
@echo "TurtleClass Build System"
@echo ""
@echo "Usage:"
@echo "  make                  - Build with default client (win32)"
@echo "  make WINDOWS_CLIENT=win32   - Build Win32 Legacy client"
@echo "  make WINDOWS_CLIENT=winui3  - Build WinUI 3 Modern client"
@echo "  make test             - Build and run tests"
@echo "  make clean            - Remove build artifacts"
@echo "  make install          - Install to system"
@echo "  make package          - Create installer package"
@echo ""
@echo "Examples:"
@echo "  make                          # Default build"
@echo "  make WINDOWS_CLIENT=winui3    # Build WinUI 3 version"
@echo "  make test WINDOWS_CLIENT=win32  # Test Win32 build"
