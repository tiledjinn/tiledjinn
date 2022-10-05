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

#ifndef SPRITE_H
#define SPRITE_H

#include "tiledjinn.h"
#include "Draw.h"
#include "Blitters.h"

/* rectangulo */
typedef struct {
    int x1, y1, x2, y2;
} rect_t;

extern void MakeRect(rect_t *rect, int x, int y, int w, int h);

typedef struct {
    int w, h;
} SpriteEntry;

/* sprite */
typedef struct Sprite {
    TLN_PaletteId palette_id;
    SpriteEntry info;
    TLN_Tileset tileset;
    int tileset_entry;
    int x, y;      /* screen space location (TLN_SetSpritePosition) */
    int dx, dy;
    int xworld, yworld;  /* world space location (TLN_SetSpriteWorldPosition) */
    float sx, sy;
    float ptx, pty;    /* normalized pivot position inside sprite (default = 0,0) */
    rect_t srcrect;
    rect_t dstrect;
    draw_t mode;
    uint8_t *blend;
    uint32_t flags;
    ScanDrawPtr draw;
    ScanBlitPtr blitter;
    bool ok;  /* draw if true */
    bool do_collision;
    bool collision;
    bool world_space;  /* valid position is world space, false = screen space */
    bool dirty;      /* requires call to UpdatePosition() before drawing */
} Sprite;

extern void UpdateSprite(Sprite *sprite);

#endif