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

#include <stdio.h>
#include "tiledjinn.h"
#include "Palette.h"
#include "Tables.h"

struct Palette **indexed_palettes;

/*!
 * \brief
 * Creates a new color table
 * 
 * \param entries
 * Number of color entries (typically 256)
 * 
 * \returns
 * Reference to the created palette or NULL if error
 */
bool TLN_CreatePalette(unsigned char id, int entries) {
#pragma EXPORT_FUNC
  struct Palette *palette;
  int size = sizeof(struct Palette) + (4 * entries);

  palette = (struct Palette *) CreateBaseObject(OT_PALETTE, size);
  if (palette) {
    palette->entries = entries;
    TLN_SetLastError(TLN_ERR_OK);

    if (indexed_palettes[id] != NULL) {
      TLN_DeletePalette(id);
    }

    indexed_palettes[id] = palette;
    return true;
  }

  return false;
}

/*!
 * \brief
 * Deletes the specified palette and frees memory
 * 
 * \param palette
 * Reference to the palette to delete
 * 
 * \remarks
 * Don't delete a palette currently attached to a layer or sprite!
 */
bool TLN_DeletePalette(TLN_PaletteId palette_id) {
#pragma EXPORT_FUNC
  struct Palette *palette = indexed_palettes[palette_id];

  if (CheckBaseObject(palette, OT_PALETTE)) {
    DeleteBaseObject(palette);
    TLN_SetLastError(TLN_ERR_OK);
    return true;
  }
  else {
    return false;
  }
}

/*!
 * \brief
 * Sets the RGB color value of a palette entry
 * 
 * \param palette
 * Reference to the palette to modify
 * 
 * \param index
 * Index of the palette entry to modify (0-255)
 * 
 * \param r
 * Red component of the color (0-255)
 * 
 * \param g
 * Green component of the color (0-255)
 * 
 * \param b
 * Blue component of the color (0-255)
 */
bool TLN_SetPaletteColor(TLN_PaletteId palette_id, int index, uint8_t r, uint8_t g, uint8_t b) {
#pragma EXPORT_FUNC
  struct Palette *palette = indexed_palettes[palette_id];

  if (palette != NULL && index < palette->entries) {
    uint32_t *data = (uint32_t *) GetPaletteData(palette, index);
    *data = PackRGB32(r, g, b);
    TLN_SetLastError(TLN_ERR_OK);
    return true;
  }

  return false;
}

/*!
 * \brief
 * Returns the color value of a palette entry
 * 
 * \param palette
 * Reference to the palette to get the color
 * 
 * \param index
 * Index of the palette entry to obtain (0-255)
 * 
 * \returns
 * 32-bit integer with the packed color in internal pixel format RGBA
 */
const uint8_t *TLN_GetPaletteData(TLN_PaletteId palette_id, int index) {
#pragma EXPORT_FUNC
  const struct Palette *palette = indexed_palettes[palette_id];
  if (palette == NULL || index >= palette->entries) {
    TLN_SetLastError(TLN_ERR_IDX_PICTURE);
    return false;
  }
  TLN_SetLastError(TLN_ERR_OK);
  return GetPaletteData(palette, index);
}

static bool
EditPaletteColor(TLN_PaletteId palette_id, uint8_t *blend_table, uint8_t r, uint8_t g, uint8_t b, uint8_t start,
                 uint8_t num) {
  int end;
  int c;
  const uint8_t *color_ptr;
  struct Palette *palette = indexed_palettes[palette_id];

  if (!CheckBaseObject(palette, OT_PALETTE)) {
    return false;
  }

  if (start >= palette->entries) {
    TLN_SetLastError(TLN_ERR_IDX_PICTURE);
    return false;
  }

  end = start + num - 1;
  if (end >= palette->entries) {
    end = palette->entries - 1;
  }

  color_ptr = TLN_GetPaletteData(palette_id, start);
  for (c = start; c <= end; c++) {
    TLN_SetPaletteColor(palette_id,
                        start,
                        blendfunc(blend_table, color_ptr[0], r),
                        blendfunc(blend_table, color_ptr[1], g),
                        blendfunc(blend_table, color_ptr[2], b));
  }

  TLN_SetLastError(TLN_ERR_OK);
  return true;
}

/*!
 * \brief
 * Modifies a range of colors by adding the provided color value to the selected range. The result is always a brighter color.
 * 
 * \param palette
 * Reference to the palette to modify
 * 
 * \param r
 * Red component of the color (0-255)
 * 
 * \param g
 * Green component of the color (0-255)
 * 
 * \param b
 * Blue component of the color (0-255)
 *
 * \param start
 * index of the first color entry to modify
 * 
 * \param num
 * number of colors from start to modify
 */
bool TLN_AddPaletteColor(TLN_PaletteId palette_id, uint8_t r, uint8_t g, uint8_t b, uint8_t start, uint8_t num) {
#pragma EXPORT_FUNC
  return EditPaletteColor(palette_id, SelectBlendTable(BLEND_ADD), r, g, b, start, num);
}

/*!
 * \brief
 * Modifies a range of colors by subtracting the provided color value to the selected range. The result is always a darker color.
 * 
 * \param palette
 * Reference to the palette to modify
 * 
 * \param r
 * Red component of the color (0-255)
 * 
 * \param g
 * Green component of the color (0-255)
 * 
 * \param b
 * Blue component of the color (0-255)
 *
 * \param start
 * index of the first color entry to modify
 * 
 * \param num
 * number of colors from start to modify
 */
bool TLN_SubPaletteColor(TLN_PaletteId palette_id, uint8_t r, uint8_t g, uint8_t b, uint8_t start, uint8_t num) {
#pragma EXPORT_FUNC
  return EditPaletteColor(palette_id, SelectBlendTable(BLEND_SUB), r, g, b, start, num);
}

/*!
 * \brief
 * Modifies a range of colors by modulating (normalized product) the provided color value to the selected range. The result is always a darker color.
 * 
 * \param palette
 * Reference to the palette to modify
 * 
 * \param r
 * Red component of the color (0-255)
 * 
 * \param g
 * Green component of the color (0-255)
 * 
 * \param b
 * Blue component of the color (0-255)
 *
 * \param start
 * index of the first color entry to modify
 * 
 * \param num
 * number of colors from start to modify
 */
bool TLN_ModPaletteColor(TLN_PaletteId palette_id, uint8_t r, uint8_t g, uint8_t b, uint8_t start, uint8_t num) {
#pragma EXPORT_FUNC
  return EditPaletteColor(palette_id, SelectBlendTable(BLEND_MOD), r, g, b, start, num);
}
