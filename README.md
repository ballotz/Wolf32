# Wolf32 - Cross-platform porting of Wolf3D

The aim is to make the game build and run on as many compilers/platforms as possible

Files bridge.h and bridge.c contains the system depended part of the game, and should be the only part that must be re-implemented to support a new platform

Once the original game will be fully functional, some features could be added, e.g. modern control system, hi res rendering, etc

## Platforms

At the moment Windows and MacOS are supported with SDL2

## Compiling

### Windows

Open the provided Visual Studio 2019 solution, should build out of the box

The SDL2 dll is needed for the executable to run, a post build step will copy it in the build folder

#### Known Issues

The x64 build in not working yet, use x86

### MacOS

Open the SDL2 dmg and place the SDL2 framework in your /Library/Frameworks folder

Now the provided Xcode project should build

## Game assets

Only WL6 assets compatible with the source release are supported now

The resource are searched in the current path, that for the VS and Xcode solutions should be the root directory of the repository

bridge.c can be easily modified to search files in any path

## TODO

Fix load/save: now direct pointers to global variables of the program are stored in data structures
