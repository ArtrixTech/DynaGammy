# Gammy

Adjusts pixel brightness based on screen contents. It dims the screen if it's too bright (according to user settings) and viceversa.

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
NOTE: If make fails with "PlaceholderText is not a member of QPalette" errors in ui_mainwindow.h, delete the offending lines and run make again.

## Usage

Gammy starts minimized in the system tray (or maximized if the tray is absent). Click on it to open the settings. Expand the options by dragging the bottom of the window.

## License

GPL-3.0

