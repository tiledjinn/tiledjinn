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

#include <math.h>
#include <string.h>
#include "Engine.h"
#include "Draw.h"
#include "Layer.h"
#include "Tileset.h"
#include "Tilemap.h"
#include "Tables.h"

static void SelectBlitter(Layer *layer);

/*!
 * \brief Configures a tiled background layer with the specified tilemap
 * \param nlayer Layer index [0, num_layers - 1]
 * \param tilemap Reference to the tilemap to assign
 * \returns true if success or false if error
 * \see TLN_LoadTilemap()
 */
bool TLN_SetLayerTilemap(int nlayer, TLN_Tilemap tilemap) {
#pragma EXPORT_FUNC

  Layer *layer;
  if (nlayer >= engine->numlayers) {
    TLN_SetLastError(TLN_ERR_IDX_LAYER);
    return false;
  }

  layer = &engine->layers[nlayer];
  layer->ok = false;
  if (!CheckBaseObject(tilemap, OT_TILEMAP)) {
    return false;
  }

  /* select tilemsp's own tileset */
  TLN_Tileset tileset = tilemap->tileset;

  if (!CheckBaseObject(tileset, OT_TILESET)) {
    return false;
  }

  if (tilemap->maxindex <= tileset->numtiles) {
    layer->tileset = tileset;
    layer->tilemap = tilemap;
    layer->width = tilemap->cols * tileset->width;
    layer->height = tilemap->rows * tileset->height;
  }

  /* apply priority attribute */
  if (tileset->attributes != NULL) {
    const int num_tiles = tilemap->rows * tilemap->cols;
    int c;
    Tile *tile = tilemap->tiles;
    for (c = 0; c < num_tiles; c++, tile++) {
      if (tile->index != 0) {
        if (tileset->attributes[tile->index - 1].priority == true) {
          tile->flags |= FLAG_PRIORITY;
        }
        else {
          tile->flags &= ~FLAG_PRIORITY;
        }
      }
    }
  }

  if (tilemap->visible) {
    layer->ok = true;
    layer->draw = GetLayerDraw(layer);
    SelectBlitter(layer);
  }

  TLN_SetLastError(TLN_ERR_OK);
  return true;
}

/*!
 * \brief Sets full layer priority, appearing in front of sprites
 * 
 * \param nlayer Layer index [0, num_layers - 1]
 * \param enable Enable (true) or dsiable (false) full priority
 */
bool TLN_SetLayerPriority(int nlayer, bool enable) {
#pragma EXPORT_FUNC
  Layer *layer;
  if (nlayer >= engine->numlayers) {
    TLN_SetLastError(TLN_ERR_IDX_LAYER);
    return false;
  }

  layer = &engine->layers[nlayer];
  layer->priority = enable;
  return true;
}

/*!
 * \brief
 * Returns the layer width in pixels
 *
 * \param nlayer
 * Layer index [0, num_layers - 1]
 *
 * \see TLN_SetLayer(), TLN_GetLayerHeight()
 */
int TLN_GetLayerWidth(int nlayer) {
#pragma EXPORT_FUNC
  if (nlayer >= engine->numlayers) {
    TLN_SetLastError(TLN_ERR_IDX_LAYER);
    return false;
  }

  TLN_SetLastError(TLN_ERR_OK);
  return engine->layers[nlayer].width;
}

/*!
 * \brief
 * Returns the layer height in pixels
 *
 * \param nlayer
 * Layer index [0, num_layers - 1]
 *
 * \see TLN_SetLayer(), TLN_GetLayerWidth()
 */
int TLN_GetLayerHeight(int nlayer) {
#pragma EXPORT_FUNC
  if (nlayer >= engine->numlayers) {
    TLN_SetLastError(TLN_ERR_IDX_LAYER);
    return false;
  }

  TLN_SetLastError(TLN_ERR_OK);
  return engine->layers[nlayer].height;
}

/*!
 * \brief
 * Sets the blending mode (transparency effect)
 * 
 * \param nlayer
 * Layer index [0, num_layers - 1]
 * 
 * \param mode
 * Member of the TLN_Blend enumeration
 *
 * \see
 * Blending
 */
bool TLN_SetLayerBlendMode(int nlayer, TLN_Blend mode) {
#pragma EXPORT_FUNC
  Layer *layer;
  if (nlayer >= engine->numlayers) {
    TLN_SetLastError(TLN_ERR_IDX_LAYER);
    return false;
  }

  layer = &engine->layers[nlayer];
  layer->blend = SelectBlendTable(mode);
  SelectBlitter(layer);
  TLN_SetLastError(TLN_ERR_OK);
  return true;
}

/*!
 * \brief Returns the active tileset on a \ref LAYER_TILE or \ref LAYER_OBJECT layer type
 * \param nlayer Layer index [0, num_layers - 1]
 * \returns Reference to the active tileset
 * \see TLN_SetLayerTilemap(), TLN_SetLayerObjects()
 */
TLN_Tileset TLN_GetLayerTileset(int nlayer) {
#pragma EXPORT_FUNC
  if (nlayer < engine->numlayers) {
    TLN_SetLastError(TLN_ERR_OK);
    return engine->layers[nlayer].tileset;
  }

  TLN_SetLastError(TLN_ERR_IDX_LAYER);
  return NULL;
}

/*!
 * \brief Returns the active tilemap on a \ref LAYER_TILE layer type
 * \param nlayer Layer index [0, num_layers - 1]
 * \returns Reference to the active tilemap
 * \see TLN_SetLayerTilemap()
 */
TLN_Tilemap TLN_GetLayerTilemap(int nlayer) {
#pragma EXPORT_FUNC
  if (nlayer < engine->numlayers) {
    TLN_SetLastError(TLN_ERR_OK);
    return engine->layers[nlayer].tilemap;
  }

  TLN_SetLastError(TLN_ERR_IDX_LAYER);
  return NULL;
}

/*!
 * \brief
 * Sets the position of the tileset that corresponds to the upper left corner
 * 
 * \param nlayer
 * Layer index [0, num_layers - 1]
 * 
 * \param hstart
 * Horizontal offset in the tileset on the left side
 * 
 * \param vstart
 * Vertical offset in the tileset on the top side
 * 
 * The tileset usually spans an area much bigger than the viewport. Use this
 * function to move the viewport insde the tileset. Change this value progressively
 * for each frame to get a scrolling effect
 * 
 * \remarks
 * Call this function inside a raster callback to get a raster scrolling effect. 
 * Use this to create horizontal strips of the same
 * layer that move at different speeds to simulate depth. The extreme case of this effect, where
 * the position is changed in each scanline, is called "line scroll" and was the technique used by
 * games such as Street Fighter II to simualte a pseudo 3d floor, or many racing games to simulate
 * a 3D road.
 */
bool TLN_SetLayerPosition(int nlayer, int hstart, int vstart) {
#pragma EXPORT_FUNC
  Layer *layer;
  if (nlayer >= engine->numlayers) {
    TLN_SetLastError(TLN_ERR_IDX_LAYER);
    return false;
  }

  layer = &engine->layers[nlayer];
  if (layer->width == 0 || layer->height == 0) {
    TLN_SetLastError(TLN_ERR_REF_TILEMAP);
    return false;
  }

  /* wrapping */
  layer->hstart = hstart % layer->width;
  layer->vstart = vstart % layer->height;
  if (layer->hstart < 0) {
    layer->hstart += layer->width;
  }
  if (layer->vstart < 0) {
    layer->vstart += layer->height;
  }

  TLN_SetLastError(TLN_ERR_OK);
  if (layer->tilemap && layer->tilemap->visible) {
    layer->ok = true;
  }
  return true;
}

/*!
 * \brief
 * Gets info about the tile located in tilemap space
 * 
 * \param nlayer
 * Id of the layer to query [0, num_layers - 1]
 * 
 * \param x
 * x position
 * 
 * \param y
 * y position
 * 
 * \param info
 * Pointer to an application-allocated TLN_TileInfo struct that will get the data
 * 
 * \returns
 * true if success or false if error
 * 
 * \remarks
 * Use this function to implement collision detection between sprites and the main background layer.
 * 
 * \see
 * TLN_TileInfo
 */
bool TLN_GetLayerTile(int nlayer, int x, int y, TLN_TileInfo *info) {
#pragma EXPORT_FUNC
  Layer *layer;
  TLN_Tileset tileset;
  TLN_Tilemap tilemap;
  TLN_Tile tile;
  int xpos, ypos;
  int xtile, ytile;
  int srcx, srcy;
  int column = 0;
  int column_offset = 0;

  if (nlayer >= engine->numlayers) {
    TLN_SetLastError(TLN_ERR_IDX_LAYER);
    return false;
  }
  if (!info) {
    TLN_SetLastError(TLN_ERR_NULL_POINTER);
    return false;
  }

  layer = &engine->layers[nlayer];
  if (!CheckBaseObject(layer->tileset, OT_TILESET) || !CheckBaseObject(layer->tilemap, OT_TILEMAP)) {
    return false;
  }

  tileset = layer->tileset;
  tilemap = layer->tilemap;

  xpos = x % layer->width;
  if (xpos < 0) {
    xpos += layer->width;
  }
  xtile = xpos >> tileset->hshift;
  srcx = xpos & tileset->hmask;

  if (layer->column) {
    column = x / tileset->width;
    if (xpos != 0 && x > xpos) {
      column++;
    }
    column_offset = layer->column[column];
  }

  ypos = (y + column_offset) % layer->height;
  if (ypos < 0) {
    ypos += layer->height;
  }
  srcy = ypos & tileset->vmask;

  ytile = ypos >> tileset->vshift;
  tile = &tilemap->tiles[ytile * tilemap->cols + xtile];

  memset(info, 0, sizeof(TLN_TileInfo));
  info->col = xtile;
  info->row = ytile;
  info->xoffset = srcx;
  info->yoffset = srcy;
  if (tile->index != 0) {
    info->index = tile->index - 1;
    info->flags = tile->flags;
    info->color = GetTilesetPixel (tileset, tile->index, srcx, srcy);
    info->type = tileset->attributes[info->index].type;
  }
  else {
    info->empty = true;
  }

  TLN_SetLastError(TLN_ERR_OK);
  return true;
}

/*!
 * \brief
 * Enables column offset mode for this layer
 * 
 * \param nlayer
 * Layer index [0, num_layers - 1]
 * 
 * \param offset
 * Array of offsets to set. Set NULL to disable column offset mode
 * 
 * Column offset is a value that is added or substracted (depeinding on the
 * sign) to the vertical position for that layer (see TLN_SetLayerPosition) for
 * each column in the tilemap assigned to that layer. 
 * 
 * \remarks
 * This feature is tipically used to simulate vertical strips moving at different
 * speeds, or combined with a line scroll effect, to fake rotations where the angle
 * is small. The Sega Genesis games Puggsy and Chuck Rock II used this trick to simulate
 * partially rotating backgrounds
 */
bool TLN_SetLayerColumnOffset(int nlayer, int *offset) {
#pragma EXPORT_FUNC
  if (nlayer >= engine->numlayers) {
    TLN_SetLastError(TLN_ERR_IDX_LAYER);
    return false;
  }

  engine->layers[nlayer].column = offset;
  TLN_SetLastError(TLN_ERR_OK);
  return true;
}

/*! \brief Enables a layer previously disabled with \ref TLN_DisableLayer 
 * \param nlayer Layer index [0, num_layers - 1]
 * \remarks The layer must have been previously configured. A layer without a prior configuration can't be enabled 
 */
bool TLN_EnableLayer(int nlayer) {
#pragma EXPORT_FUNC
  Layer *layer = NULL;

  if (nlayer >= engine->numlayers) {
    TLN_SetLastError(TLN_ERR_IDX_LAYER);
    return false;
  }

  layer = &engine->layers[nlayer];

  /* check proper config */
  if (layer->tilemap && layer->tileset) {
    layer->ok = true;
    TLN_SetLastError(TLN_ERR_IDX_LAYER);
    return true;
  }

  TLN_SetLastError(TLN_ERR_NULL_POINTER);
  return false;
}

/*!
 * \brief
 * Disables the specified layer so it is not drawn
 * 
 * \param nlayer
 * Layer index [0, num_layers - 1]
 *
 * \remarks
 * A layer configured with an invalid tileset, tilemap or palette is
 * automatically disabled
 * 
 * \see
 * TLN_SetLayer()
 */
bool TLN_DisableLayer(int nlayer) {
#pragma EXPORT_FUNC
  if (nlayer >= engine->numlayers) {
    TLN_SetLastError(TLN_ERR_IDX_LAYER);
    return false;
  }

  engine->layers[nlayer].ok = false;
  TLN_SetLastError(TLN_ERR_OK);
  return true;
}

/*!
 * \brief
 * Sets affine transform matrix to enable rotating and scaling of this layer
 * 
 * \param nlayer
 * Layer index [0, num_layers - 1]
 * 
 * \param affine
 * Pointer to an TLN_Affine matrix, or NULL to disable it
 * 
 * Enable the transformation matrix to give the layer the capabilities of the famous 
 * Super Nintendo / Famicom Mode 7. Beware that the rendering of a transformed layer
 * uses more CPU than a regular layer. Unlike the original Mode 7, that could only transform
 * the single layer available, Tilengine can transform all the layers at the same time. The only
 * limitation is the available CPU power.
 *
 * \remarks
 * Call this function inside a raster callback to set the transformation matrix in the middle of
 * the frame. Setting it for each scanline is the trick used by many Super Nintendo games to fake
 * a 3D perspective projection.
 * 
 * \see
 * TLN_SetLayerTransform()
 */
bool TLN_SetLayerAffineTransform(int nlayer, TLN_Affine *affine) {
#pragma EXPORT_FUNC
  Layer *layer;
  if (nlayer >= engine->numlayers) {
    TLN_SetLastError(TLN_ERR_IDX_LAYER);
    return false;
  }

  layer = &engine->layers[nlayer];
  if (affine) {
    Matrix3 transform;
    math2d_t dx = layer->hstart + /*(engine->framebuffer.width>>1)  +*/ affine->dx;
    math2d_t dy = layer->vstart + /*(engine->framebuffer.height>>1) +*/ affine->dy;

    Matrix3SetIdentity(&layer->transform);
    Matrix3SetTranslation(&transform, -dx, -dy);
    Matrix3Multiply(&layer->transform, &transform);
    Matrix3SetRotation(&transform, (math2d_t) fmod(-affine->angle, 360.0f));
    Matrix3Multiply(&layer->transform, &transform);
    Matrix3SetScale(&transform, 1 / affine->sx, 1 / affine->sy);
    Matrix3Multiply(&layer->transform, &transform);
    Matrix3SetTranslation(&transform, dx, dy);
    Matrix3Multiply(&layer->transform, &transform);

    layer->mode = MODE_TRANSFORM;
    layer->draw = GetLayerDraw(layer);
    SelectBlitter(layer);

    /*printf ("TLN_SetLayerAffineTransform (ptr=%08Xh, a=%.02f, d=%.02f,%.02f, s=%.02f,%.02f)\n",
      affine, affine->angle, affine->dx, affine->dy, affine->sx, affine->sy);*/

    TLN_SetLastError(TLN_ERR_OK);
    return true;
  }
  else {
    return TLN_ResetLayerMode(nlayer);
  }
}

/*!
 * \brief
 * Sets affine transform matrix to enable rotating and scaling of this layer
 * 
 * \param layer
 * Layer index [0, num_layers - 1]
 * 
 * \param angle
 * Rotation angle in degrees
 * 
 * \param dx
 * Horizontal displacement
 * 
 * \param dy
 * Vertical displacement
 * 
 * \param sx
 * Horizontal scaling
 * 
 * \param sy
 * Vertical scaling
 * 
 * \remarks
 * This function is a simple wrapper to TLN_SetLayerAffineTransform() without using the TLN_Affine struct
 * 
 * \see
 * TLN_SetLayerAffineTransform()
 */
bool TLN_SetLayerTransform(int layer, float angle, float dx, float dy, float sx, float sy) {
#pragma EXPORT_FUNC
  TLN_Affine affine;

  affine.angle = angle;
  affine.dx = dx;
  affine.dy = dy;
  affine.sx = sx;
  affine.sy = sy;

  return TLN_SetLayerAffineTransform(layer, &affine);
}

/*!
 * \brief
 * Sets simple scaling
 * 
 * \param nlayer
 * Layer index [0, num_layers - 1]
 * 
 * \param sx
 * Horizontal scale factor
 * 
 * \param sy
 * Vertical scale factor
 * 
 * By default the scaling factor of a given layer is 1.0f, 1.0f, which means
 * no scaling. Use values below 1.0 to downscale (shrink) and above 1.0 to upscale (enlarge).
 * Call TLN_ResetLayerMode() to disable scaling
 * 
 * Write detailed description for TLN_SetLayerScaling here.
 * 
 * \see TLN_ResetLayerMode()
 * 
 */
bool TLN_SetLayerScaling(int nlayer, float sx, float sy) {
#pragma EXPORT_FUNC
  Layer *layer;
  if (nlayer >= engine->numlayers) {
    TLN_SetLastError(TLN_ERR_IDX_LAYER);
    return false;
  }

  layer = &engine->layers[nlayer];
  layer->xfactor = float2fix(sx);
  layer->dx = float2fix((1.0f / sx));
  layer->dy = float2fix((1.0f / sy));
  layer->mode = MODE_SCALING;
  layer->draw = GetLayerDraw(layer);
  SelectBlitter(layer);
  TLN_SetLastError(TLN_ERR_OK);
  return true;
}

/*!
 * \brief
 * Sets the table for pixel mapping render mode
 * 
 * \param nlayer
 * Layer index [0, num_layers - 1]
 * \param table
 * User-provided array of hres*vres sized TLN_PixelMap items 
 * 
 * \see
 * TLN_SetLayerScaling(), TLN_SetLayerAffineTransform()
 */
bool TLN_SetLayerPixelMapping(int nlayer, TLN_PixelMap *table) {
#pragma EXPORT_FUNC
  Layer *layer;
  if (nlayer >= engine->numlayers) {
    TLN_SetLastError(TLN_ERR_IDX_LAYER);
    return false;
  }

  layer = &engine->layers[nlayer];
  layer->pixel_map = table;
  if (table != NULL) {
    layer->mode = MODE_PIXEL_MAP;
  }
  else {
    layer->mode = MODE_NORMAL;
  }
  layer->draw = GetLayerDraw(layer);
  return true;
}

/*!
 * \brief
 * Disables scaling or affine transform for the layer
 * 
 * \param nlayer
 * Layer index [0, num_layers - 1]
 * 
 * Write detailed description for TLN_ResetLayerMode here.
 * 
 * \see
 * TLN_SetLayerScaling(), TLN_SetLayerAffineTransform()
 */
bool TLN_ResetLayerMode(int nlayer) {
#pragma EXPORT_FUNC
  Layer *layer;
  if (nlayer >= engine->numlayers) {
    TLN_SetLastError(TLN_ERR_IDX_LAYER);
    return false;
  }

  layer = &engine->layers[nlayer];
  layer->mode = MODE_NORMAL;
  layer->draw = GetLayerDraw(layer);
  SelectBlitter(layer);
  TLN_SetLastError(TLN_ERR_OK);
  return true;
}

/*!
 * \brief
 * Enables clipping rectangle on selected layer
 * 
 * \param nlayer
 * Layer index [0, num_layers - 1]
 * 
 * \param x1
 * left coordinate
 *
 * \param y1
 * top coordinate
 *
 * \param x2
 * right coordinate
 *
 * \param y2
 * bottom coordinate
 *
 * \see
 * TLN_DisableLayerClip()
 */
bool TLN_SetLayerClip(int nlayer, int x1, int y1, int x2, int y2) {
#pragma EXPORT_FUNC
  Layer *layer;
  if (nlayer >= engine->numlayers) {
    TLN_SetLastError(TLN_ERR_IDX_LAYER);
    return false;
  }

  layer = &engine->layers[nlayer];
  layer->clip.x1 = x1 >= 0 && x1 <= engine->framebuffer.width ? x1 : 0;
  layer->clip.x2 = x2 >= 0 && x2 <= engine->framebuffer.width ? x2 : engine->framebuffer.width;
  layer->clip.y1 = y1 >= 0 && y1 <= engine->framebuffer.height ? y1 : 0;
  layer->clip.y2 = y2 >= 0 && y2 <= engine->framebuffer.height ? y2 : engine->framebuffer.height;
  TLN_SetLastError(TLN_ERR_OK);
  return true;
}

/*!
 * \brief
 * Disables clipping rectangle on selected layer
 * 
 * \param nlayer
 * Layer index [0, num_layers - 1]
 * 
 * \see
 * TLN_SetLayerClip()
 */
bool TLN_DisableLayerClip(int nlayer) {
#pragma EXPORT_FUNC
  Layer *layer;
  if (nlayer >= engine->numlayers) {
    TLN_SetLastError(TLN_ERR_IDX_LAYER);
    return false;
  }

  layer = &engine->layers[nlayer];
  layer->clip.x1 = 0;
  layer->clip.x2 = engine->framebuffer.width;
  layer->clip.y1 = 0;
  layer->clip.y2 = engine->framebuffer.height;
  TLN_SetLastError(TLN_ERR_OK);
  return true;
}

/*!
 * \brief
 * Enables mosaic effect (pixelation) for selected layer
 *
 * \param nlayer
 * Layer index [0, num_layers - 1]
 *
 * \param width
 * horizontal pixel size
 *
 * \param height
 * vertical pixel size
 *
 * \see
 * TLN_DisableLayerMosaic()
 */
bool TLN_SetLayerMosaic(int nlayer, int width, int height) {
#pragma EXPORT_FUNC
  Layer *layer;
  if (nlayer >= engine->numlayers) {
    TLN_SetLastError(TLN_ERR_IDX_LAYER);
    return false;
  }

  layer = &engine->layers[nlayer];
  layer->mosaic.w = width;
  layer->mosaic.h = height;
  SelectBlitter(layer);
  TLN_SetLastError(TLN_ERR_OK);
  return true;
}

/*!
 * \brief
 * Disables mosaic effect for selected layer
 * 
 * \param nlayer
 * Layer index [0, num_layers - 1]
 *
 * \see
 * TLN_SetLayerMosaic()
 */
bool TLN_DisableLayerMosaic(int nlayer) {
#pragma EXPORT_FUNC
  Layer *layer;
  if (nlayer >= engine->numlayers) {
    TLN_SetLastError(TLN_ERR_IDX_LAYER);
    return false;
  }

  layer = &engine->layers[nlayer];
  layer->mosaic.h = 0;
  TLN_SetLastError(TLN_ERR_OK);
  return true;
}

Layer *GetLayer(int idx) {
  return &engine->layers[idx];
}

/* updates layer from world position, accounting offset and parallax */
void UpdateLayer(int nlayer) {
  Layer *layer = GetLayer(nlayer);
  const int lx = (int) (engine->xworld * layer->world.xfactor) - layer->world.offsetx;
  const int ly = (int) (engine->yworld * layer->world.yfactor) - layer->world.offsety;
  TLN_SetLayerPosition(nlayer, lx, ly);
}

static void SelectBlitter(Layer *layer) {
  bool scaling = layer->mode == MODE_SCALING;
  bool blend;
  int bpp;

  /* without mosaic effect */
  if (layer->mosaic.h == 0) {
    blend = layer->blend != NULL;
    bpp = 32;
  }
    /* with mosaic effect */
  else {
    blend = false;
    bpp = 8;
  }

  layer->blitters[0] = GetBlitter(bpp, false, scaling, blend);
  layer->blitters[1] = GetBlitter(bpp, true, scaling, blend);
}
