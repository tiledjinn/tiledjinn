/*
 * Tilengine - The 2D retro graphics engine with raster effects
 * Copyright (C) 2015-2019 Marc Palacios Domenech <mailto:megamarc@hotmail.com>
 * Copyright (C) 2022 TileDjinn Contributors
 * All rights reserved
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 * */

#ifndef TILESET_H
#define TILESET_H

#include "Object.h"
#include "Palette.h"

/* types of tilesets */
typedef enum {
    TILESET_NONE,
    TILESET_TILES,
} TilesetType;

/* Tileset definition */
struct Tileset {
    DEFINE_OBJECT;
    TilesetType tstype;     /* tileset type */
    int numtiles;     /* number of tiles */
    int width;       /* horizontal tile size */
    int height;       /* vertical tile size */
    int hshift;       /* horizontal shift */
    int vshift;       /* vertical shift */
    int hmask;       /* horizontal bitmask */
    int vmask;       /* vertical bitmask */
    TLN_TileAttributes *attributes;  /* attribute array */
    bool *color_key;     /* array telling if each line has color key or is solid */
    uint16_t *tiles;    /* tile indexes for animation */
    uint8_t data[];       /* variable size data for images[], attributes[], color_key[] and pixels */
};

#define GetTilesetLine(tileset, index, y) \
  (((index) << (tileset)->vshift) + (y))

#define GetTilesetPixel(tileset, index, x, y) \
  tileset->data[((((index) << (tileset)->vshift) + (y)) << (tileset)->hshift) + (x)]

#endif
