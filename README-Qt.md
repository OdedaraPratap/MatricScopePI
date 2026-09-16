# Matric Scope — Qt/OpenCV C++11 port

The active measurement and tray-settings workflow is implemented as a native
Qt Widgets/OpenCV application. The original C# WinForms source remains in the
repository as a migration reference; it is not compiled by CMake.

## Raspberry Pi dependencies

```bash
sudo apt install build-essential cmake qtbase5-dev libopencv-dev
```

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j2
```

Copy `DiamondRules.xml` beside the executable (the install target does this),
connect a V4L2-compatible camera as camera index 0, and run:

```bash
./build/MatricScopePI
```

The application starts full-screen and uses a responsive layout with minimum
48-pixel action buttons for a typical 7-inch 800×480 or 1024×600 touch display.
Use **BACKGROUND** with an empty tray before measuring, select the shape, and
use **CALIBRATE** to enter the camera's pixels-per-millimetre value.
