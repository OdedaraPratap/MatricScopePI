# Matric Scope — Qt/OpenCV C++11 port

The active measurement and tray-settings workflow is implemented as a native
Qt Widgets/OpenCV application. The original C# WinForms source remains in the
repository as a migration reference; it is not compiled by CMake.

## Raspberry Pi dependencies

```bash
sudo apt install build-essential cmake qtbase5-dev libqt5sql5-sqlite \
  libgpiod-dev gpiod libopencv-dev
```

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j2
```

### Open directly in Qt Creator

Open `MatricScopePI.pro` in Qt Creator and select the Raspberry Pi Qt kit. The
qmake project includes every native `.cpp`/`.h` file and enables Widgets, SQL,
OpenCV 4, Raspberry Pi GPIO through libgpiod, and the Hikrobot MVS SDK. If MVS is not
installed in `/opt/MVS`, add `MVCAMERA_ROOT=/your/mvs/path` to the selected
kit's build environment, rerun qmake, and rebuild.

Qt Creator creates build output outside the source directory by default. The
executable is placed in that build directory's `bin` folder.

Install Hikrobot's Linux ARM MVS SDK under `/opt/MVS` (or set
`MVCAMERA_ROOT` to its location), copy `DiamondRules.xml` beside the executable,
and run:

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
password protection, history database, Raspberry Pi GPIO bin output, XML rule
storage, and OpenCV measurement pipeline.

Windows-only integrations are mapped to Raspberry Pi interfaces:
camera acquisition uses Hikrobot's native `MvCameraControl` Linux SDK, the Windows
Registry uses `QSettings`, and `System.Data.SQLite` uses Qt SQL's SQLite driver.
Printing and serial-port dependencies are not included in the native target.

### Raspberry Pi GPIO bin output

Matched bin numbers are emitted as a five-bit value using libgpiod. The default
BCM lines are GPIO17 (bit 0), GPIO27 (bit 1), GPIO22 (bit 2), GPIO23 (bit 3),
GPIO25 (bit 4), with GPIO24 as a 25 ms active-high strobe. This represents bin
numbers 0–31 and therefore covers all 20 tray positions.
The line numbers and chip can be changed with these `QSettings` keys:
`gpio/data0`, `gpio/data1`, `gpio/data2`, `gpio/data3`, `gpio/data4`, `gpio/strobe`, and
`gpio/chip` (default `gpiochip0`). The application user must have permission to
access `/dev/gpiochip0`.

The build searches for `MvCameraControl.h` and `libMvCameraControl.so` in the
standard `/opt/MVS` ARM locations and in `MVCAMERA_ROOT`. When the SDK is found,
the application enumerates GigE/USB MVS devices, opens the first camera with
exclusive access, starts continuous acquisition, obtains frames with
`MV_CC_GetImageBuffer`, converts vendor pixel formats to BGR, and releases each
SDK buffer. OpenCV/V4L2 is retained only as a build-time fallback for development
machines without the MVS SDK; disable SDK probing explicitly with
`-DMATRIC_USE_MVCAMERA=OFF`.
