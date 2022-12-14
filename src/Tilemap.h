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

#ifndef TILEMAP_H
#define TILEMAP_H

#include "Object.h"
#include "Tileset.h"

/* mapa */
struct Tilemap {
    DEFINE_OBJECT;
    int rows;    /* rows*/
    int cols;    /* columns */
    int maxindex;  /* highest tile index */
    int bgcolor;  /* background color */
    int id;      /* id property */
    bool visible;  /* visible property */
    struct Tileset *tileset; /* attached tileset (if any) */
    Tile tiles[];
};

#endif
