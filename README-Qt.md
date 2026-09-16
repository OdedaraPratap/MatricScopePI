# Matric Scope — Qt/OpenCV C++11 port

The active measurement and tray-settings workflow is implemented as a native
Qt Widgets/OpenCV application. The original C# WinForms source remains in the
repository as a migration reference; it is not compiled by CMake.

## Raspberry Pi dependencies

```bash
sudo apt install build-essential cmake qtbase5-dev libqt5sql5-sqlite \
  libqt5serialport5-dev libopencv-dev
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

## Ported application areas

The CMake target is independent of the WinForms project and contains native
replacements for the application entry point, automatic measurement screen,
camera settings, calibration, tray/rule settings, measurement variation,
password protection, history database, printing, serial bin output, XML rule
storage, and OpenCV measurement pipeline.

Windows-only integrations are mapped to Raspberry Pi interfaces:
`uEye`/`MvCameraControl` acquisition uses OpenCV's V4L2 capture, the Windows
Registry uses `QSettings`, GDI/Skia printing uses `QPrinter`,
`System.Data.SQLite` uses Qt SQL's SQLite driver, and COM-port output defaults
to `/dev/ttyUSB0`. The port can be changed with the `serial/port` QSettings key.
