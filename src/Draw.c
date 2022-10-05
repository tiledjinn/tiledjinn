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

#include <string.h>
#include <stdlib.h>
#include "tiledjinn.h"
#include "Draw.h"
#include "Engine.h"
#include "Tileset.h"
#include "Tilemap.h"
#include "Sprite.h"


extern struct Palette **indexed_palettes;  /* Palette.c */

/* private prototypes */
static void DrawSpriteCollision(int nsprite, const uint8_t *srcpixel, uint16_t *dstpixel, int width, int dx);

static void DrawSpriteCollisionScaling(int nsprite, uint8_t *srcpixel, uint16_t *dstpixel, int width, int dx, int srcx);

static bool check_sprite_coverage(Sprite *sprite, int nscan) {
  /* check sprite coverage */
  if (nscan < sprite->dstrect.y1 || nscan >= sprite->dstrect.y2) {
    return false;
  }
  if (sprite->dstrect.x2 < 0 || sprite->srcrect.x2 < 0) {
    return false;
  }
  if ((sprite->flags & FLAG_MASKED) && nscan >= engine->sprite_mask_top && nscan <= engine->sprite_mask_bottom) {
    return false;
  }
  return true;
}

/* Draws the next scanline of the frame started with TLN_BeginFrame() or TLN_BeginWindowFrame() */
bool DrawScanline(void) {
  int line = engine->line;
  uint8_t *scan = GetFramebufferLine(line);
  int size = engine->framebuffer.width;
  int c;
  bool background_priority = false;  /* at least one tile in priority layer */
  bool sprite_priority = false;    /* at least one sprite in priority layer */

  /* call raster effect callback */
  if (engine->cb_raster) {
    engine->cb_raster(line);
  }

  /* background is solid color */
  BlitColor(scan, engine->bgcolor, size);

  background_priority = false;
  memset(engine->priority, 0, engine->framebuffer.pitch);
  memset(engine->collision, -1, engine->framebuffer.width * sizeof(uint16_t));

  /* draw background layers */
  for (c = engine->numlayers - 1; c >= 0; c--) {
    Layer *layer = &engine->layers[c];

    if (layer->ok) {
      /* update if dirty */
      if (engine->dirty || layer->dirty) {
        UpdateLayer(c);
        layer->dirty = false;
      }

      /* draw */
      if (!layer->priority && line >= layer->clip.y1 && line <= layer->clip.y2) {
        if (layer->draw(c, line) == true) {
          background_priority = true;
        }
      }
    }
  }

  /* draw regular sprites */
  for (int index = 0; index < engine->numsprites; ++index) {
    Sprite *sprite = &engine->sprites[index];

    if (!sprite->ok) {
      continue;
    }

    /* update if dirty */
    if (sprite->world_space && (sprite->dirty || engine->dirty)) {
      sprite->x = sprite->xworld - engine->xworld;
      sprite->y = sprite->yworld - engine->yworld;
      UpdateSprite(sprite);
      sprite->dirty = false;
    }

    if (check_sprite_coverage(sprite, line)) {
      if (!(sprite->flags & FLAG_PRIORITY)) {
        sprite->draw(index, line);
      }
      else {
        sprite_priority = true;
      }
    }
  }

  /* draw background layers with priority */
  for (c = engine->numlayers - 1; c >= 0; c--) {
    const Layer *layer = &engine->layers[c];
    if (layer->ok && layer->priority && line >= layer->clip.y1 && line <= layer->clip.y2) {
      layer->draw(c, line);
    }
  }

  /* overlay background tiles with priority */
  if (background_priority == true) {
    uint32_t *src = (uint32_t *) engine->priority;
    uint32_t *dst = (uint32_t *) scan;
    for (c = 0; c < engine->framebuffer.width; c++) {
      if (*src) {
        *dst = *src;
      }
      src++;
      dst++;
    }
  }

  // TODO: draw sprites with priority
  /* draw sprites with priority */
//  if (sprite_priority == true) {
//    index = list->first;
//    while (index != -1) {
//      Sprite *sprite = &engine->sprites[index];
//      if (check_sprite_coverage(sprite, line) && (sprite->flags & FLAG_PRIORITY))
//        sprite->draw(index, line);
//      index = sprite->list_node.next;
//    }
//  }

  /* next scanline */
  engine->dirty = false;
  engine->line++;
  return engine->line < engine->framebuffer.height;
}

/* draw scanline of tiled background */
static bool DrawLayerScanline(int nlayer, int nscan) {
  const Layer *layer = &engine->layers[nlayer];
  const TLN_Tileset tileset = layer->tileset;
  const TLN_Tilemap tilemap = layer->tilemap;
  int shift;
  TLN_Tile tile;
  uint8_t *srcpixel;
  int x, x1;
  int xpos, ypos;
  int xtile, ytile;
  int srcx, srcy;
  int direction, width;
  int column;
  int line;
  uint8_t *dstpixel;
  uint8_t *dstpixel_pri;
  uint8_t *dst;
  bool color_key;
  bool priority = false;

  /* mosaic effect */
  if (layer->mosaic.h != 0) {
    shift = 0;
    dstpixel = layer->mosaic.buffer;
    if (nscan % layer->mosaic.h == 0) {
      memset(dstpixel, 0, engine->framebuffer.width);
    }
    else {
      goto draw_end;
    }
  }
  else {
    shift = 2;
    dstpixel = GetFramebufferLine (nscan);
  }

  /* target lines */
  x = layer->clip.x1;
  dstpixel += (x << shift);
  dstpixel_pri = engine->priority;

  xpos = (layer->hstart + x) % layer->width;
  xtile = xpos >> tileset->hshift;
  srcx = xpos & tileset->hmask;

  /* fill whole scanline */
  column = x % tileset->width;
  while (x < layer->clip.x2) {
    int tilewidth;

    /* column offset: update ypos */
    if (layer->column) {
      ypos = (layer->vstart + nscan + layer->column[column]) % layer->height;
      if (ypos < 0) {
        ypos = layer->height + ypos;
      }
    }
    else {
      ypos = (layer->vstart + nscan) % layer->height;
    }

    ytile = ypos >> tileset->vshift;
    srcy = ypos & tileset->vmask;

    tile = &tilemap->tiles[ytile * tilemap->cols + xtile];

    /* get effective tile width */
    tilewidth = tileset->width - srcx;
    x1 = x + tilewidth;
    if (x1 > layer->clip.x2) {
      x1 = layer->clip.x2;
    }
    width = x1 - x;

    /* paint if not empty tile */
    if (tile->index) {
      const uint16_t tile_index = tileset->tiles[tile->index];

      /* H/V flip */
      if (tile->flags & FLAG_FLIPX) {
        direction = -1;
        srcx = tilewidth - 1;
      }
      else {
        direction = 1;
      }
      if (tile->flags & FLAG_FLIPY) {
        srcy = tileset->height - srcy - 1;
      }

      /* paint tile scanline */
      srcpixel = &GetTilesetPixel(tileset, tile_index, srcx, srcy);
      if (tile->flags & FLAG_PRIORITY) {
        dst = dstpixel_pri;
        priority = true;
      }
      else {
        dst = dstpixel;
      }
      line = GetTilesetLine(tileset, tile_index, srcy);
      color_key = *(tileset->color_key + line);
      layer->blitters[color_key](srcpixel, tile->flags & FLAG_PALETTES, dst, width, direction, 0,
                                 layer->blend);
    }

    /* next tile */
    x += width;
    width <<= shift;
    dstpixel += width;
    dstpixel_pri += width;
    xtile = (xtile + 1) % tilemap->cols;
    srcx = 0;
    column++;
  }

  draw_end:
  if (layer->mosaic.h != 0) {
    int offset = (layer->clip.x1 << shift);
    uint8_t *srcptr = layer->mosaic.buffer + offset;
    uint8_t *dstptr = GetFramebufferLine (nscan) + offset;
    int width = layer->clip.x2 - layer->clip.x1;

    /* TODO: i broke mosaics when i did indexed palettes.. this shouldn't be on the layer anyway though.... */
    if (layer->blend != NULL) {
      BlitMosaicBlend(srcptr, tile->flags & FLAG_PALETTES, dstptr, width, layer->mosaic.w,
                      layer->blend);
    }
    else {
      BlitMosaicSolid(srcptr, tile->flags & FLAG_PALETTES, dstptr, width, layer->mosaic.w);
    }
  }

  return priority;
}

/* draw scanline of tiled background with scaling */
static bool DrawLayerScanlineScaling(int nlayer, int nscan) {
  const Layer *layer = &engine->layers[nlayer];
  const TLN_Tileset tileset = layer->tileset;
  const TLN_Tilemap tilemap = layer->tilemap;
  int shift;
  TLN_Tile tile;
  uint8_t *srcpixel;
  int x, x1;
  int xpos, ypos;
  int xtile, ytile;
  int srcx, srcy;
  int direction, width;
  int column;
  int line;
  uint8_t *dstpixel;
  uint8_t *dstpixel_pri;
  uint8_t *dst;
  fix_t fix_tilewidth;
  fix_t fix_x;
  fix_t dx;
  bool color_key;
  bool priority = false;

  /* mosaic effect */
  if (layer->mosaic.h != 0) {
    shift = 0;
    dstpixel = layer->mosaic.buffer;
    if (nscan % layer->mosaic.h == 0) {
      memset(dstpixel, 0, engine->framebuffer.width);
    }
    else {
      goto draw_end;
    }
  }
  else {
    shift = 2;
    dstpixel = GetFramebufferLine (nscan);
  }

  /* target lines */
  x = layer->clip.x1;
  dstpixel += (x << shift);
  dstpixel_pri = engine->priority;

  xpos = (layer->hstart + fix2int(x * layer->dx)) % layer->width;
  xtile = xpos >> tileset->hshift;
  srcx = xpos & tileset->hmask;

  /* fill whole scanline */
  fix_x = int2fix (x);
  column = x % tileset->width;
  while (x < layer->clip.x2) {
    int tilewidth;
    int tilescalewidth;

    /* column offset: update ypos */
    ypos = nscan;
    if (layer->column) {
      ypos += layer->column[column];
    }

    ypos = layer->vstart + fix2int(ypos * layer->dy);
    if (ypos < 0) {
      ypos = layer->height + ypos;
    }
    else {
      ypos = ypos % layer->height;
    }

    ytile = ypos >> tileset->vshift;
    srcy = ypos & tileset->vmask;

    tile = &tilemap->tiles[ytile * tilemap->cols + xtile];

    /* get effective tile width */
    tilewidth = tileset->width - srcx;
    dx = int2fix(tilewidth);
    fix_tilewidth = tilewidth * layer->xfactor;
    fix_x += fix_tilewidth;
    x1 = fix2int (fix_x);
    tilescalewidth = x1 - x;
    if (tilescalewidth) {
      dx /= tilescalewidth;
    }
    else {
      dx = 0;
    }

    /* right clip */
    if (x1 > layer->clip.x2) {
      x1 = layer->clip.x2;
    }
    width = x1 - x;

    /* paint if tile is not empty */
    if (tile->index) {
      const uint16_t tile_index = tileset->tiles[tile->index];

      /* volteado H/V */
      if (tile->flags & FLAG_FLIPX) {
        direction = -dx;
        srcx = tilewidth - 1;
      }
      else {
        direction = dx;
      }
      if (tile->flags & FLAG_FLIPY) {
        srcy = tileset->height - srcy - 1;
      }

      /* pinta tile scanline */
      srcpixel = &GetTilesetPixel (tileset, tile_index, srcx, srcy);
      if (tile->flags & FLAG_PRIORITY) {
        dst = dstpixel_pri;
        priority = true;
      }
      else {
        dst = dstpixel;
      }
      line = GetTilesetLine (tileset, tile_index, srcy);
      color_key = *(tileset->color_key + line);
      layer->blitters[color_key](srcpixel, tile->flags & FLAG_PALETTES, dst, width, direction, 0, layer->blend);
    }

    /* next tile */
    width <<= shift;
    dstpixel += width;
    dstpixel_pri += width;
    x = x1;
    xtile = (xtile + 1) % tilemap->cols;
    srcx = 0;
    column++;
  }

  draw_end:
  if (layer->mosaic.h != 0) {
    int offset = (layer->clip.x1 << shift);
    uint8_t *srcptr = layer->mosaic.buffer + offset;
    uint8_t *dstptr = GetFramebufferLine (nscan) + offset;
    int width = layer->clip.x2 - layer->clip.x1;

    /* TODO: this is a copy paste block?? broken mosaic with indexed palettes */
    if (layer->blend != NULL) {
      BlitMosaicBlend(srcptr, tile->flags & FLAG_PALETTES, dstptr, width, layer->mosaic.w,
                      layer->blend);
    }
    else {
      BlitMosaicSolid(srcptr, tile->flags & FLAG_PALETTES, dstptr, width, layer->mosaic.w);
    }
  }

  return priority;
}

/* draw scanline of tiled background with affine transform */
static bool DrawLayerScanlineAffine(int nlayer, int nscan) {
  Layer *layer = &engine->layers[nlayer];
  const TLN_Tileset tileset = layer->tileset;
  const TLN_Tilemap tilemap = layer->tilemap;
  int shift;
  TLN_Tile tile;
  int x, width;
  int x1, y1, x2, y2;
  fix_t dx, dy;
  int xpos, ypos;
  int xtile, ytile;
  int srcx, srcy;
  uint8_t *dstpixel;
  Point2D p1, p2;

  /* mosaic effect */
  if (layer->mosaic.h != 0) {
    shift = 0;
    dstpixel = layer->mosaic.buffer;
    if (nscan % layer->mosaic.h == 0) {
      memset(dstpixel, 0, engine->framebuffer.width);
    }
    else {
      goto draw_end;
    }
  }
  else {
    shift = 2;
    dstpixel = engine->tmpindex;
    memset(dstpixel, 0, engine->framebuffer.width);
  }

  /* target lines */
  x = layer->clip.x1;
  width = layer->clip.x2;

  xpos = layer->hstart;
  ypos = layer->vstart + nscan;

  Point2DSet(&p1, (math2d_t) xpos, (math2d_t) ypos);
  Point2DSet(&p2, (math2d_t) xpos + width, (math2d_t) ypos);
  Point2DMultiply(&p1, &layer->transform);
  Point2DMultiply(&p2, &layer->transform);

  x1 = float2fix(p1.x);
  y1 = float2fix(p1.y);
  x2 = float2fix(p2.x);
  y2 = float2fix(p2.y);

  dx = (x2 - x1) / width;
  dy = (y2 - y1) / width;

  while (x < width) {
    xpos = abs((fix2int(x1) + layer->width)) % layer->width;
    ypos = abs((fix2int(y1) + layer->height)) % layer->height;

    xtile = xpos >> tileset->hshift;
    ytile = ypos >> tileset->vshift;

    srcx = xpos & tileset->hmask;
    srcy = ypos & tileset->vmask;

    tile = &tilemap->tiles[ytile * tilemap->cols + xtile];

    /* paint if not empty tile */
    if (tile->index) {
      const uint16_t tile_index = tileset->tiles[tile->index];

      /* H/V flip */
      if (tile->flags & FLAG_FLIPX) {
        srcx = tileset->width - srcx - 1;
      }
      if (tile->flags & FLAG_FLIPY) {
        srcy = tileset->height - srcy - 1;
      }

      /* pinta scanline tile */
      *dstpixel = GetTilesetPixel (tileset, tile_index, srcx, srcy);
    }

    /* next pixel */
    x++;
    x1 += dx;
    y1 += dy;
    dstpixel++;
  }

  draw_end:
  if (layer->mosaic.h != 0) {
    int offset = (layer->clip.x1 << shift);
    uint8_t *srcptr = layer->mosaic.buffer + offset;
    uint8_t *dstptr = GetFramebufferLine (nscan) + offset;
    int width = layer->clip.x2 - layer->clip.x1;

    if (layer->blend != NULL) {
      BlitMosaicBlend(srcptr, tile->flags & FLAG_PALETTES, dstptr, width, layer->mosaic.w, layer->blend);
    }
    else {
      BlitMosaicSolid(srcptr, tile->flags & FLAG_PALETTES, dstptr, width, layer->mosaic.w);
    }
  }
  else {
    int offset = (layer->clip.x1 << shift);
    uint8_t *srcptr = engine->tmpindex + offset;
    uint8_t *dstptr = GetFramebufferLine (nscan) + offset;
    int width = layer->clip.x2 - layer->clip.x1;

    layer->blitters[1](srcptr, tile->flags & FLAG_PALETTES, dstptr, width, 1, 0, layer->blend);
  }
  return false;
}

/* draw scanline of tiled background with per-pixel mapping */
static bool DrawLayerScanlinePixelMapping(int nlayer, int nscan) {
  Layer *layer = &engine->layers[nlayer];
  const TLN_Tileset tileset = layer->tileset;
  const TLN_Tilemap tilemap = layer->tilemap;
  const int hstart = layer->hstart + layer->width;
  const int vstart = layer->vstart + layer->height;
  int shift;
  TLN_Tile tile;
  int x, width;
  int xpos, ypos;
  int xtile, ytile;
  int srcx, srcy;
  uint8_t *dstpixel;
  TLN_PixelMap *pixel_map;

  /* mosaic effect */
  if (layer->mosaic.h != 0) {
    shift = 0;
    dstpixel = layer->mosaic.buffer;
    if (nscan % layer->mosaic.h == 0) {
      memset(dstpixel, 0, engine->framebuffer.width);
    }
    else {
      goto draw_end;
    }
  }
  else {
    shift = 2;
    dstpixel = engine->tmpindex;
    memset(dstpixel, 0, engine->framebuffer.width);
  }

  /* target lines */
  x = layer->clip.x1;
  width = layer->clip.x2 - layer->clip.x1;

  pixel_map = &layer->pixel_map[nscan * engine->framebuffer.width + x];
  while (x < width) {
    xpos = abs(hstart + pixel_map->dx) % layer->width;
    ypos = abs(vstart + pixel_map->dy) % layer->height;

    xtile = xpos >> tileset->hshift;
    ytile = ypos >> tileset->vshift;

    srcx = xpos & tileset->hmask;
    srcy = ypos & tileset->vmask;

    tile = &tilemap->tiles[ytile * tilemap->cols + xtile];

    /* paint if not empty tile */
    if (tile->index) {
      const uint16_t tile_index = tileset->tiles[tile->index];

      /* H/V flip */
      if (tile->flags & FLAG_FLIPX) {
        srcx = tileset->width - srcx - 1;
      }
      if (tile->flags & FLAG_FLIPY) {
        srcy = tileset->height - srcy - 1;
      }

      /* paint tile scanline */
      *dstpixel = GetTilesetPixel (tileset, tile_index, srcx, srcy);
    }

    /* next pixel */
    x++;
    dstpixel++;
    pixel_map++;
  }

  draw_end:
  if (layer->mosaic.h != 0) {
    int offset = (layer->clip.x1 << shift);
    uint8_t *srcptr = layer->mosaic.buffer + offset;
    uint8_t *dstptr = GetFramebufferLine (nscan) + offset;
    int width = layer->clip.x2 - layer->clip.x1;

    if (layer->blend != NULL) {
      BlitMosaicBlend(srcptr, tile->flags & FLAG_PALETTES, dstptr, width, layer->mosaic.w, layer->blend);
    }
    else {
      BlitMosaicSolid(srcptr, tile->flags & FLAG_PALETTES, dstptr, width, layer->mosaic.w);
    }
  }
  else {
    int offset = (layer->clip.x1 << shift);
    uint8_t *srcptr = engine->tmpindex + offset;
    uint8_t *dstptr = GetFramebufferLine (nscan) + offset;
    int width = layer->clip.x2 - layer->clip.x1;

    layer->blitters[1](srcptr, tile->flags & FLAG_PALETTES, dstptr, width, 1, 0, layer->blend);
  }
  return true;
}

/* draw sprite scanline */
static bool DrawSpriteScanline(int nsprite, int nscan) {
  int w;
  Sprite *sprite;
  uint8_t *srcpixel;
  uint8_t *dstscan;
  uint32_t *dstpixel;
  int srcx, srcy;
  int direction;

  sprite = &engine->sprites[nsprite];
  dstscan = GetFramebufferLine(nscan);

  srcx = sprite->srcrect.x1;
  srcy = sprite->srcrect.y1 + (nscan - sprite->dstrect.y1);
  w = sprite->dstrect.x2 - sprite->dstrect.x1;

  /* H/V flip */
  if (sprite->flags & FLAG_FLIPX) {
    direction = -1;
    srcx = sprite->info.w - srcx - 1;
  }
  else {
    direction = 1;
  }
  if (sprite->flags & FLAG_FLIPY) {
    srcy = sprite->info.h - srcy - 1;
  }

  /*
#define GetTilesetPixel(tileset, index, x, y) \
  tileset->data[(((index << tileset->vshift) + y) << tileset->hshift) + x]
   */

  //srcpixel = sprite->pixels + (srcy * sprite->pitch) + srcx;
  srcpixel = &GetTilesetPixel(sprite->tileset, sprite->tileset_entry, srcx, srcy);

  dstpixel = (uint32_t *) (dstscan + (sprite->dstrect.x1 << 2));
  sprite->blitter(srcpixel, sprite->palette_id, dstpixel, w, direction, 0, sprite->blend);

  if (sprite->do_collision) {
    uint16_t *dstpixel = engine->collision + sprite->dstrect.x1;
    DrawSpriteCollision(nsprite, srcpixel, dstpixel, w, direction);
  }
  return true;
}

/* draw sprite scanline with scaling */
static bool DrawScalingSpriteScanline(int nsprite, int nscan) {
  Sprite *sprite;
  uint8_t *srcpixel;
  uint8_t *dstscan;
  uint32_t *dstpixel;
  int srcx, srcy;
  int dstw, dstx, dx;

  sprite = &engine->sprites[nsprite];

  dstscan = GetFramebufferLine(nscan);
  srcx = sprite->srcrect.x1;
  srcy = sprite->srcrect.y1 + (nscan - sprite->dstrect.y1) * sprite->dy;
  dstw = sprite->dstrect.x2 - sprite->dstrect.x1;

  /* H/V flip */
  if (sprite->flags & FLAG_FLIPX) {
    srcx = int2fix(sprite->info.w) - srcx;
    dstx = sprite->dstrect.x2;
    dx = -sprite->dx;
  }
  else {
    dstx = sprite->dstrect.x1;
    dx = sprite->dx;
  }
  if (sprite->flags & FLAG_FLIPY) {
    srcy = int2fix(sprite->info.h) - srcy;
  }

  srcpixel = &GetTilesetPixel(sprite->tileset, sprite->tileset_entry, srcx, srcy);
  dstpixel = (uint32_t *) (dstscan + (sprite->dstrect.x1 << 2));
  sprite->blitter(srcpixel, sprite->palette_id, dstpixel, dstw, dx, srcx, sprite->blend);

  if (sprite->do_collision) {
    uint16_t *dstpixel = engine->collision + sprite->dstrect.x1;
    DrawSpriteCollisionScaling(nsprite, srcpixel, dstpixel, dstw, dx, srcx);
  }
  return true;
}

/* updates per-pixel sprite collision buffer */
static void DrawSpriteCollision(int nsprite, const uint8_t *srcpixel, uint16_t *dstpixel, int width, int dx) {
  while (width) {
    if (*srcpixel) {
      if (*dstpixel != 0xFFFF) {
        engine->sprites[nsprite].collision = true;
        engine->sprites[*dstpixel].collision = true;
      }
      *dstpixel = (uint16_t) nsprite;
    }
    srcpixel += dx;
    dstpixel++;
    width--;
  }
}

/* updates per-pixel sprite collision buffer for scaled sprite */
static void
DrawSpriteCollisionScaling(int nsprite, uint8_t *srcpixel, uint16_t *dstpixel, int width, int dx, int srcx) {
  while (width) {
    uint32_t src = *(srcpixel + srcx / (1 << FIXED_BITS));
    if (src) {
      if (*dstpixel != 0xFFFF) {
        engine->sprites[nsprite].collision = true;
        engine->sprites[*dstpixel].collision = true;
      }
      *dstpixel = (uint16_t) nsprite;
    }

    /* next pixel */
    srcx += dx;
    dstpixel++;
    width--;
  }
}

/* draw modes */
enum {
    DRAW_SPRITE,
    DRAW_TILED_LAYER,
    MAX_DRAW_TYPE,
};

/* table of function pointers to draw procedures */
static const ScanDrawPtr drawers[MAX_DRAW_TYPE][MAX_DRAW_MODE] =
        {
                {DrawSpriteScanline, DrawScalingSpriteScanline, NULL, NULL},
                {DrawLayerScanline,  DrawLayerScanlineScaling, DrawLayerScanlineAffine, DrawLayerScanlinePixelMapping},
        };

/* returns suitable draw procedure based on layer configuration */
ScanDrawPtr GetLayerDraw(Layer *layer) {
  if (layer->tilemap != NULL) {
    return drawers[DRAW_TILED_LAYER][layer->mode];
  }
  else {
    return NULL;
  }
}

/* returns suitable draw procedure based on sprite configuration */
ScanDrawPtr GetSpriteDraw(draw_t mode) {
  return drawers[DRAW_SPRITE][mode];
}
