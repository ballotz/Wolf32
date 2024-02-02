# Wolf32 - Cross-platform porting of Wolf3D

The aim is to make the game build and run on as many compilers/platforms as possible

Files bridge.h is used by the game and gives the interface that should be implemented for the porting to a new platform.

Once the original game will be fully functional, some features could be added, e.g. modern control system, hi res rendering, etc

## Platforms

### SDL2

- Windows
- MacOS

## Compiling

### Windows

Open the provided Visual Studio 2019 solution, should build out of the box

The SDL2 dll is needed for the executable to run, a post build step will copy it in the build folder

To support Windows versions back to Windows XP the Platform Toolset can be changed (v141_xp)

### MacOS

Open the SDL2 dmg and place the SDL2 framework in your /Library/Frameworks folder

Now the provided Xcode project should build

## Game assets

Only WL6 assets compatible with the source release are supported now

The resource are searched in the current path, that for the VS and Xcode solutions should be the root directory of the repository

bridge.c can be easily modified to search files in any path

## TODO

Fix load/save: now direct pointers to global variables of the program are stored in data structures
