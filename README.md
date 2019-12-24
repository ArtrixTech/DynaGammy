# Gammy

is a GUI tool for adjusting pixel brightness automatically (or manually) and temperature (manual only for now).

It can dim the screen if its content is too bright, or brighten it otherwise.
This can help your eyes adjust when switching between dark and light windows, especially at night or in suboptimal lighting conditions.

Screenshots available on its [website](https://getgammy.com).

## Installation

### Windows

#### Requirements

[Visual C++ 2017](https://aka.ms/vs/16/release/vc_redist.x64.exe)

#### Run

Unpack the [latest releases](https://github.com/Fushko/gammy/releases) .zip file.

### Linux/X11

#### Requirements

##### apt-get (or other package manager equivalent):
- build-essential
- libgl1-mesa-dev
- qt5-default

#### Build and run
```
git clone https://github.com/Fushko/gammy.git
cd gammy
qmake Gammy.pro
make
./gammy
```
NOTE: If make fails with ```PlaceholderText is not a member of QPalette``` errors in ui_mainwindow.h, your Qt version is older than 5.12.
I recommend updating Qt to a newer version, but as a workaround you can delete the offending lines in ui_mainwindow.h and run make again.

## Usage

Gammy starts minimized in the system tray (or maximized if the tray is absent). Click on the icon to open the settings. 
Dragging the bottom of the window will reveal additional settings.

Unticking the Auto checkbox allows manual brightness adjustment.
The padlock button can be toggled to extend the brightness range to a max. of 200%. (Linux only)

## Third party

- Qt 5 ([LGPL](https://doc.qt.io/qt-5/lgpl.html))
- Plog ([MPL](https://github.com/SergiusTheBest/plog/blob/master/LICENSE))

## License

[GPLv3](https://github.com/Fushko/gammy/blob/master/LICENSE)

