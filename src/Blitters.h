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

#ifndef BLITTERS_H
#define BLITTERS_H

#include "tiledjinn.h"

typedef void (*ScanBlitPtr) \
(uint8_t *srcpixel, TLN_PaletteId palette_id, void *dstptr, int width, int dx, int offset, uint8_t *blend);

ScanBlitPtr GetBlitter(int bpp, bool key, bool scaling, bool blend);

void BlitColor(void *dstptr, uint32_t color, int width);

void BlitMosaicSolid(uint8_t *srcpixel, TLN_PaletteId palette_id, void *dstptr, int width, int size);

void BlitMosaicBlend(uint8_t *srcpixel, TLN_PaletteId palette_id, void *dstptr, int width, int size, uint8_t *blend);

#endif