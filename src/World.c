/*
 * Copyright (C) 2022 TileDjinn Contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 * */

#include "tiledjinn.h"
#include "Engine.h"
#include "Layer.h"
#include "Sprite.h"


/*!
 * \brief Sets layer parallax factor to use in conjunction with \ref TLN_SetWorldPosition
 * \param nlayer Layer index [0, num_layers - 1]
 * \param x Horizontal parallax factor
 * \param y Vertical parallax factor
 */
bool TLN_SetLayerParallaxFactor(int nlayer, float x, float y) {
#pragma EXPORT_FUNC
  Layer *layer;
  if (nlayer >= engine->numlayers) {
    TLN_SetLastError(TLN_ERR_IDX_LAYER);
    return false;
  }

  layer = &engine->layers[nlayer];
  layer->world.xfactor = x;
  layer->world.yfactor = y;
  layer->dirty = true;
  TLN_SetLastError(TLN_ERR_OK);
  return true;
}

/*!
 * \brief Sets global world position, moving all layers in sync according to their parallax factor
 * \param x horizontal position in world space
 * \param y vertical position in world space
 */
void TLN_SetWorldPosition(int x, int y) {
#pragma EXPORT_FUNC
  engine->xworld = x;
  engine->yworld = y;
  engine->dirty = true;
}

/*!
 * \brief Sets the sprite position in world space coordinates
 * \param nsprite Id of the sprite [0, num_sprites - 1]
 * \param x Horizontal world position of pivot (0 = left margin)
 * \param y Vertical world position of pivot (0 = top margin)
 * \sa TLN_SetSpritePivot
 */
bool TLN_SetSpriteWorldPosition(int nsprite, int x, int y) {
#pragma EXPORT_FUNC
  Sprite *sprite;
  if (nsprite >= engine->numsprites) {
    TLN_SetLastError(TLN_ERR_IDX_SPRITE);
    return false;
  }

  sprite = &engine->sprites[nsprite];
  sprite->xworld = x;
  sprite->yworld = y;
  sprite->world_space = true;
  sprite->dirty = true;

  TLN_SetLastError(TLN_ERR_OK);
  return true;
}
