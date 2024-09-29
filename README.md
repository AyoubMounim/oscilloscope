# Oscilloscope Tool

Simple oscilloscope emulator tool written using raylib. The application listens
to real-time data coming through a UDP socket and visualizes the signal on
screen.
The GUI has some basic controls to adjust the view, more features may be added
in future.

## Building

Run the `build.sh` script in the project's root directory. The script makes a
build directory and builds the executable `oscilloscope` inside of it. It also builds a simple
golang program `signal-generator` that can be used as a signal generator for testing.

