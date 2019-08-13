# Gammy

Adjusts pixel brightness based on screen contents. It dims the screen if it's too bright (according to user settings) and viceversa. Works on Windows and Linux.

## Installation

### Windows

#### Requirements

[Visual C++ 2017](https://aka.ms/vs/16/release/vc_redist.x64.exe)

#### Run

Unpack the [latest releases](https://github.com/Fushko/gammy/releases) .zip file.

### Linux

#### Requirements

##### apt-get (or other pack. manager equivalent):
- build-essential 
- libgl1-mesa-dev 
- qt5-default

#### Build and run
```
git clone https://github.com/Fushko/gammy.git
cd gammy
qmake Gammy.pro
make
./Gammy
```

## Usage
Gammy starts minimized in the tray. Click on it to open the window. Expand the options by dragging the arrow in the bottom right.

## License

GPL-3.0

