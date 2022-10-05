![TileDjinn logo](tiledjinn.png)
# TileDjinn - The 2D retro graphics engine
[![License: MPL 2.0](https://img.shields.io/badge/License-MPL%202.0-brightgreen.svg)](https://opensource.org/licenses/MPL-2.0)

TileDjinn is an open source, cross-platform 2D graphics engine for creating classic/retro games with tile maps, sprites and palettes. Its unique scanline-based rendering algorithm makes raster effects a core feature, a technique used by many games running on real 2D graphics chips.

https://tiledjinn.com

# Contents
- [TileDjinn - The 2D retro graphics engine](#tiledjinn---the-2d-retro-graphics-engine)
- [Contents](#contents)
- [Features](#features)
- [Getting binaries](#getting-binaries)
  - [Build from source](#build-from-source)
- [Documentation](#documentation)
- [Editing assets](#editing-assets)

# Features
* Written in portable C (C99)
* MPL 2.0 license: free for any project, including commercial ones, allows console development
* Cross platform: available builds for Windows (32/64), Linux PC(32/64), Mac OS X and Raspberry Pi
* High performance: all samples run at 60 fps with CRT emulation enabled on a Raspberry Pi 3
* Streamlined, easy to learn API that requires very little lines of code
* Built-in SDL-based windowing for quick tests
* Integrate inside any existing framework as a slave renderer
* Create or modify graphic assets procedurally at run time
* True raster effects: modify render parameters between scanlines
* Background layer scaling and rotation
* Sprite scaling
* Several blending modes for layers and sprites
* Pixel accurate sprite vs sprite and sprite vs layer collision detection
* Special effects: per-column offset, mosaic, per-pixel displacement, CRT emulation...

# Getting binaries
A static library for windows MSVC /MDd is provided.

## Build from source
You can also build the library from source. TileDjinn requires `SDL2` and `libpng` to build, you must provide these libraries yourself depending on your target platform.

Just clone the source. The build uses cmake.

# Contributing
Feel free to submit PR's. I'll try to look at them within 7 days. Please understand that my time for managing this project outside my own use is neither infinite nor funded.

# Samples
Coming soon..

# Instructions for use
Call `TLN_Init` and `TLN_CreateWindow` to initialize the engine.

Create palettes based on your platform's constraints using `TLN_CreatePalette`. Create a tileset based on your platform's constraints using `TLN_CreateTileset`. Create a tile map per layer using `TLN_CreateTilemap`.

For my projects, the TileDjinn port's main loop looks something like this:

```C
TLN_Tilemap *tilemaps;
TLN_Tileset tileset;

int main() {
  TLN_Init(WIDTH, HEIGHT, MAX_LAYER, 128);
  TLN_CreateWindow(NULL, 0);
  
  {
    int i;
    for (i = 0; i < NUM_PALETTES; ++i) {
      TLN_CreatePalette(i, NUM_PALETTES);
    }
  }
  
  {
    int screen;

    tileset = TLN_CreateTileset(MAX_TILES, TILE_WIDTH, TILE_HEIGHT, 0);
    for (screen = 0; screen < MAX_LAYER; ++screen) {
      tilemaps[screen] = TLN_CreateTilemap(32, 32, 0, 0, tileset);
    }
  }
  
  /* this is semantically wrong, but is the easiest way to re-order the layers in the right direction.. */
  TLN_SetLayerTilemap(SCREEN1, tilemaps[SCREEN2]);
  TLN_SetLayerTilemap(SCREEN2, tilemaps[SCREEN1]);
  
  game_init(); /* platform independent game initialization */

  /* main loop */
  while (TLN_ProcessWindow()) {
    game_loop(); /* single iteration of the game loop */
    TLN_DrawFrame(0);
    TLN_WaitRedraw();
  }

  /* de-init */
  {
    int i;
    for (i = 0; i < MAX_LAYER; ++i) {
      TLN_DeleteTilemap(tilemaps[i]);
    }
    TLN_DeleteTileset(tileset);

    for (i = 0; i < NUM_PALETTES; ++i) {
      TLN_DeletePalette(i);
    }
  }


  TLN_DeleteWindow();
  TLN_Deinit();
}


```

<!--
# Running the samples

C samples are located in `TileDjinn/samples` folder. To build them you need the gcc compiler suite, and/or Visual C++ in windows.
* **Linux**: the GCC compiler suite is already installed by default
* **Windows**: TileDjinn is tested using the MSVC runtime
* **Apple OS X**: You must install [Command-line tools for Xcode](https://developer.apple.com/xcode/). An Apple ID account is required.

Once installed, open a console window in the C samples folder and type the suitable command depending on your platform:

## Windows
```
> mingw32-make
```

## Unix-like
```
> make
```

# The tiledjinn window
The following actions can be done in the created window:
* Press <kbd>Esc</kbd> to close the window
* Press <kbd>Alt</kbd> + <kbd>Enter</kbd> to toggle full-screen/windowed
* Press <kbd>Backspace</kbd> to toggle built-in CRT effect (enabled by default)

# Creating your first program
The following section shows how to create from scratch and execute a simple tiledjinn application that does the following:
1. Reference the inclusion of TileDjinn module
2. Initialize the engine with a resolution of 400x240, one layer, no sprites and no palette animations
3. Load a *tilemap*, the asset that contains background layer data
4. Attach the loaded tilemap to the allocated background layer
5. Create a display window with default parameters: windowed, auto scale and CRT effect enabled
6. Run the window loop, updating the display at each iteration until the window is closed
7. Release allocated resources

![Test](test.png)

Create a file called `test.c` in `TileDjinn/samples` folder, and type the following code:
```c
#include "tiledjinn.h"

void main(void) {
    TLN_Tilemap foreground;

    TLN_Init (400, 240, 1, 0, 0);
    foreground = TLN_LoadTilemap ("assets/sonic/Sonic_md_fg1.tmx", NULL);
    TLN_SetLayerTilemap (0, foreground);

    TLN_CreateWindow (NULL, 0);
    while (TLN_ProcessWindow()) {
        TLN_DrawFrame (0);
    }

    TLN_DeleteTilemap (foreground);
    TLN_Deinit ();
}
```
Now the program must be built to produce an executable. Open a console window in the C samples folder and type the suitable command for your platform:

## Windows
```
> gcc test.c -o test.exe -I"../include" ../lib/Win32/tiledjinn.dll
> test.exe
```

## Linux
```
> gcc test.c -o test -ltiledjinn -lm
> ./test
```

## Apple OS X
```
> gcc test.c -o test "/usr/local/lib/tiledjinn.dylib" -lm
> ./test
```
-->

# Editing assets
TileDjinn is just a programming library that doesn't come with any editor, but the files it loads are made with standard tools. Samples come bundled with several ready-to-use assets.

I recommend these tools for development (not referral links):
* Source code: [CLion](https://www.jetbrains.com/clion/)
* Graphics, tiles, sprites, and maps: [Cosmigo Pro Motion](https://www.cosmigo.com/)
