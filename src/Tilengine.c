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

#include <stdlib.h>
#include <stdarg.h>
#include "tiledjinn.h"
#include "Tilemap.h"
#include "Tileset.h"
#include "Palette.h"
#include "Engine.h"
#include "Layer.h"
#include "Sprite.h"
#include "Tables.h"

/* magic number to recognize context object */
#define ID_CONTEXT  0x7E5D0AB1

TLN_Engine engine;  /* current context */

extern struct Palette **indexed_palettes;  // Palette.c

/*!
 * \brief
 * Initializes the graphic engine
 * 
 * \param hres
 * horizontal resolution in pixels
 * 
 * \param vres
 * vertical resolution in pixels
 * 
 * \param numlayers
 * number of layers
 * 
 * \param numsprites
 * number of sprites
 * 
 * \param numanimations
 * number of palette animation slots
 * 
 * Performs initialisation of the main engine, creates the viewport with the specified dimensions
 * and allocates the number of layers, sprites and animation slots
 */
TLN_Engine TLN_Init(int hres, int vres, int numlayers, int numsprites) {
#pragma EXPORT_FUNC
  printf("TileDjinn v%d.%d.%d %d-bit built %s %s\n", TILENGINE_VER_MAJ, TILENGINE_VER_MIN, TILENGINE_VER_REV,
         (int) (sizeof(UINTPTR_MAX) << 3), __DATE__, __TIME__); // NOLINT(bugprone-sizeof-expression)

  int c;
  TLN_Engine context;

  indexed_palettes = malloc(sizeof(struct Palette *) * 256);

  for (int i = 0; i < 256; ++i) {
    indexed_palettes[i] = 0;
  }

  TLN_SetLastError(TLN_ERR_OK);

  int bpp = 32;

  /* create framebuffer */
  context = (TLN_Engine) calloc(sizeof(Engine), 1);
  context->header = ID_CONTEXT;
  context->framebuffer.width = hres;
  context->framebuffer.height = vres;
  context->framebuffer.pitch = (((hres * bpp) >> 3) + 3) & ~0x03;
  context->priority = (uint8_t *) malloc(context->framebuffer.pitch);
  if (!context->priority) {
    TLN_DeleteContext(context);
    TLN_SetLastError(TLN_ERR_OUT_OF_MEMORY);
    return NULL;
  }

  /* sprite collision buffer */
  context->collision = (uint16_t *) calloc(hres * sizeof(uint16_t), 1);
  context->tmpindex = (uint8_t *) calloc(hres, 1);

  /* create static items */
  context->numlayers = numlayers;
  context->layers = (Layer *) calloc(numlayers, sizeof(Layer));
  if (!context->layers) {
    TLN_DeleteContext(context);
    TLN_SetLastError(TLN_ERR_OUT_OF_MEMORY);
    return NULL;
  }
  for (c = 0; c < context->numlayers; c++) {
    context->layers[c].mosaic.buffer = (uint8_t *) malloc(hres);
  }

  context->numsprites = numsprites;
  context->sprites = (Sprite *) calloc(numsprites, sizeof(Sprite));
  if (!context->sprites) {
    TLN_DeleteContext(context);
    TLN_SetLastError(TLN_ERR_OUT_OF_MEMORY);
    return NULL;
  }
  for (c = 0; c < context->numsprites; c++) {
    Sprite *sprite = &context->sprites[c];
    sprite->draw = GetSpriteDraw(MODE_NORMAL);
    sprite->blitter = GetBlitter(bpp, true, false, false);
    sprite->sx = sprite->sy = 1.0f;
  }

  context->bgcolor = PackRGB32(0, 0, 0);
  context->blit_fast = GetBlitter(bpp, false, false, false);
  if (!CreateBlendTables()) {
    TLN_DeleteContext(context);
    TLN_SetLastError(TLN_ERR_OUT_OF_MEMORY);
    return NULL;
  }
  context->blend_table = SelectBlendTable(BLEND_MOD);

  /* set as default context if it's the first one */
  if (engine == NULL) {
    engine = context;
  }

  for (c = 0; c < context->numlayers; c++) {
    TLN_DisableLayerClip(c);
  }

#ifdef _DEBUG
  TLN_SetLogLevel(TLN_LOG_ERRORS);
#endif

  return context;
}

static bool check_context(TLN_Engine context) {
  if (context != NULL) {
    if (context->header == ID_CONTEXT) {
      return true;
    }
  }
  return false;
}

/*!
* \brief
* Sets current engine context
*
* \param context
* TLN_Engine object to set as current context, returned by TLN_Init()
*
* \returns
* true if success or false if wrong context is supplied
*/
bool TLN_SetContext(TLN_Engine context) {
#pragma EXPORT_FUNC
  if (check_context(context)) {
    engine = context;
    TLN_SetLastError(TLN_ERR_OK);
    return true;
  }
  else {
    TLN_SetLastError(TLN_ERR_NULL_POINTER);
    return false;
  }
}

/*!
* \brief
* Returns the current engine context
*/
TLN_Engine TLN_GetContext(void) {
#pragma EXPORT_FUNC
  return engine;
}

/*!
* \brief
* Deinitialises current engine context and frees used resources
*/
void TLN_Deinit(void) {
#pragma EXPORT_FUNC
  if (engine != NULL) {
    TLN_DeleteContext(engine);
    engine = NULL;
  }
}

/*!
* \brief
* Deletes explicit context
*
* \param context
* context reference to delete
*/
bool TLN_DeleteContext(TLN_Engine context) {
#pragma EXPORT_FUNC
  int c;

  if (!check_context(context)) {
    TLN_SetLastError(TLN_ERR_NULL_POINTER);
    return false;
  }

  if (indexed_palettes) {
    free(indexed_palettes);
  }

  DeleteBlendTables();

  for (c = 0; c < context->numlayers; c++) {
    free(context->layers[c].mosaic.buffer);
  }

  if (context->sprites) {
    free(context->sprites);
  }

  if (context->layers) {
    free(context->layers);
  }

  if (context->priority) {
    free(context->priority);
  }

  if (context->collision) {
    free(context->collision);
  }

  if (context->tmpindex) {
    free(context->tmpindex);
  }

  free(context);
  return true;
}

/*!
 * \brief
 * Sets logging level for current instance
 *
 * \param log_level
 * value to set, member of the TLN_LogLevel enumeration
 */
void TLN_SetLogLevel(TLN_LogLevel log_level) {
#pragma EXPORT_FUNC
  if (engine != NULL) {
    engine->log_level = log_level;
  }
}

/*!
 * \brief
 * Retrieves Tilengine dll version
 * 
 * \returns
 * Returns a 32-bit integer containing three packed numbers:
 * bits 23:16 -> major version
 * bits 15: 8 -> minor version
 * bits  7: 0 -> bugfix revision
 * 
 * \remarks
 * Compare this number with the TILENGINE_HEADER_VERSION macro to check that both versions match!
 * 
 */
uint32_t TLN_GetVersion(void) {
#pragma EXPORT_FUNC
  TLN_SetLastError(TLN_ERR_OK);
  return TILENGINE_HEADER_VERSION;
}

/*!
 * \brief
 * Returns the width in pixels of the framebuffer
 * 
 * \see
 * TLN_Init(), TLN_GetHeight()
 */
int TLN_GetWidth(void) {
#pragma EXPORT_FUNC
  TLN_SetLastError(TLN_ERR_OK);
  return engine->framebuffer.width;
}

/*!
 * \brief
 * Returns the height in pixels of the framebuffer
 * 
 * \see
 * TLN_Init(), TLN_GetWidth()
 */
int TLN_GetHeight(void) {
  TLN_SetLastError(TLN_ERR_OK);
  return engine->framebuffer.height;
}

/*!
 * \brief
 * Sets the output surface for rendering
 * 
 * \param data
 * Pointer to the start of the target framebuffer
 * 
 * \param pitch
 * Number of bytes per each scanline of the framebuffer
 * 
 * Sets the output surface for rendering. Tilengine doesn't provide a windowing or hardware
 * video access. The application is responsible of allocating and maintaining the surface where
 * tilengine does the rendering. It can be a SDL surface, a locked DirectX surface, an OpenGL texture,
 * or whatever the application has access to.
 *
 * \remarks
 * The render target pixel format must be 32 bits RGBA
 *
 * \see
 * TLN_UpdateFrame()
 */
void TLN_SetRenderTarget(uint8_t *data, int pitch) {
#pragma EXPORT_FUNC
  engine->framebuffer.data = data;
  engine->framebuffer.pitch = pitch;
  TLN_SetLastError(TLN_ERR_OK);
}

/*!
 * \brief
 * Gets the location of the currently set render target
 * 
 * \see
 * TLN_SetRenderTarget()
 */
uint8_t *TLN_GetRenderTarget(void) {
#pragma EXPORT_FUNC
  TLN_SetLastError(TLN_ERR_OK);
  return engine->framebuffer.data;
}

/*!
 * \brief
 * Gets the pitch (bytes per scanline) of the currently set render target
 * 
 * \see
 * TLN_SetRenderTarget()
 */
int TLN_GetRenderTargetPitch(void) {
#pragma EXPORT_FUNC
  TLN_SetLastError(TLN_ERR_OK);
  return engine->framebuffer.pitch;
}

/* Starts active rendering of the current frame */
static void BeginFrame(int frame) {
  /* update active animations */
  int index;

  /* autoincrement if 0 */
  if (frame != 0) {
    engine->frame = frame;
  }
  else {
    engine->frame += 1;
  }

  /* frame callback */
  engine->line = 0;
  if (engine->cb_frame) {
    engine->cb_frame(engine->frame);
  }
}

/*!
 * \brief
 * Draws the frame to the previously specified render target
 *
 * \param frame Optional frame number. Set to 0 to autoincrement from previous value
 *
 * \see
 * TLN_SetRenderTarget()
 */
void TLN_UpdateFrame(int frame) {
#pragma EXPORT_FUNC
  BeginFrame(frame);
  while (DrawScanline()) {}
  TLN_SetLastError(TLN_ERR_OK);
}

/*!
 * \brief
 * Returns the number of layers specified during initialisation
 * 
 * \see
 * TLN_Init()
 */
int TLN_GetNumLayers(void) {
#pragma EXPORT_FUNC
  TLN_SetLastError(TLN_ERR_OK);
  return engine->numlayers;
}

/*!
 * \brief
 * Returns the number of sprites specified during initialisation
 * 
 * \see
 * TLN_Init()
 */

int TLN_GetNumSprites(void) {
#pragma EXPORT_FUNC
  TLN_SetLastError(TLN_ERR_OK);
  return engine->numsprites;
}

/*!
 * \brief
 * Specifies the address of the funcion to call for each drawn scanline
 * 
 * \param callback
 * Address of the function to call
 * 
 * Tilengine renders its output line by line, just as the 2D graphics chips did. The
 * raster callback is a way to simulate the "horizontal blanking interrupt" of those systems,
 * where many parameters of the rendering can be modified per line.
 *
 * \remarks
 * Setting a raster callback is optional, but much of the fun of using Tilengine comes from
 * the use of raster effects
 */
void TLN_SetRasterCallback(void (*callback)(int)) {
#pragma EXPORT_FUNC
  TLN_SetLastError(TLN_ERR_OK);
  engine->cb_raster = callback;
}

/*!
 * \brief
 * Specifies the address of the funcion to call for each drawn frame
 * 
 * \param callback
 * Address of the function to call
 */
void TLN_SetFrameCallback(void (*callback)(int)) {
#pragma EXPORT_FUNC
  TLN_SetLastError(TLN_ERR_OK);
  engine->cb_frame = callback;
}

/*!
 * \brief
 * Sets the background color
 * 
 * \param r
 * red component (0-255)
 * 
 * \param g
 * green component (0-255)
 * 
 * \param b
 * blue component (0-255)
 * 
 * The background color is the color of the pixel when there isn't any layer or sprite at
 * that position.
 * 
 * \remarks
 * This funcion can be called during a raster callback to create gradient backgrounds
 */
void TLN_SetBGColor(uint8_t r, uint8_t g, uint8_t b) {
#pragma EXPORT_FUNC
  engine->bgcolor = PackRGB32 (r, g, b);
}

/*!
 * \brief
 * Sets the background color from a Tilemap defined color
 * 
 * \param tilemap
 * Reference to the tilemap with the background color to set
 */
bool TLN_SetBGColorFromTilemap(TLN_Tilemap tilemap) {
#pragma EXPORT_FUNC
  if (CheckBaseObject(tilemap, OT_TILEMAP)) {
    engine->bgcolor = tilemap->bgcolor | 0xFF000000;
    TLN_SetLastError(TLN_ERR_OK);
    return true;
  }
  else {
    return false;
  }
}

/*!
 * \brief
 * Disales background color rendering. If you know that the last background layer will always
 * cover the entire screen, you can disable it to gain some performance
 * \see
 * TLN_SetBGColor()
 */
void TLN_DisableBGColor(void) {
#pragma EXPORT_FUNC
  engine->bgcolor = 0;
}

/*!
 * \brief
 * Sets custom blend function to use when BLEND_CUSTOM mode is selected
 * \param blend_function
 * pointer to a user-provided function that takes two parameters: source component intensity,
 * destination component intensity, and returns the desired intensity. This function is
 * called for each RGB component when blending is enabled
 * \remarks
 * This function is not called in realtime, but its result is precomputed into a look-up table
 * when TLN_SetCustomBlendFunction() is called, so the performance impact is minimal, just as low
 * as the other built-in blending modes
 * \see
 * TLN_SetSpriteBlendMode()|TLN_SetLayerBlendMode()
 */
void TLN_SetCustomBlendFunction(uint8_t (*blend_function)(uint8_t src, uint8_t dst)) {
#pragma EXPORT_FUNC
  uint8_t *table = SelectBlendTable(BLEND_CUSTOM);
  int a, b;

  if (blend_function == NULL) {
    return;
  }

  /* rellena tabla */
  for (a = 0; a < 256; a++) {
    for (b = 0; b < 256; b++) {
      table[(a << 8) + b] = blend_function(a, b);
    }
  }
}

/*!
 * \brief
 * Returns the number of objets used by the engine so far
 * 
 * \remarks
 * The objects is the total amount of tilesets, tilemaps, spritesets, palettes or sequences combined
 *
 * \see
 * TLN_GetUsedMemory()
 */
uint32_t TLN_GetNumObjects(void) {
#pragma EXPORT_FUNC
  TLN_SetLastError(TLN_ERR_OK);
  return GetNumObjects();
}

/*!
 * \brief
 * Returns the total amount of memory used by the objects
 * 
 * \see
 * TLN_GetNumObjects()
 */
uint32_t TLN_GetUsedMemory(void) {
#pragma EXPORT_FUNC
  TLN_SetLastError(TLN_ERR_OK);
  return GetNumBytes();
}

const char *const errornames[] =
        {
                "No error",
                "Not enough memory",
                "Layer index out of range",
                "Sprite index out of range",
                "Animation index out of range",
                "Picture or tile index out of range",
                "Invalid Tileset reference",
                "Invalid Tilemap reference",
                "Invalid Spriteset reference",
                "Invalid Palette reference",
                "Invalid SequencePack reference",
                "Invalid Sequence reference",
                "Invalid Bitmap reference",
                "Null pointer as required argument",
                "Resource file not found",
                "Resource file has invalid format",
                "A width or height parameter is invalid",
                "Unsupported function",
                "Invalid ObjectList reference"
        };

/*!
 * \brief
 * Sets the global error code of tilengine. Useful for custom loaders that need to set the error state.
 * 
 * \param error
 * Error code to set
 *
 * \see
 * TLN_GetLastError()
 */
void TLN_SetLastError(TLN_Error error) {
#pragma EXPORT_FUNC
  if (check_context(engine)) {
    engine->error = error;
    if (error != TLN_ERR_OK) {
      tln_trace(TLN_LOG_ERRORS, errornames[error]);
    }
  }
}

/*!
 * \brief
 * Returns the last error after an invalid operation
 * 
 * \see
 * TLN_Error
 */
TLN_Error TLN_GetLastError(void) {
#pragma EXPORT_FUNC
  if (check_context(engine)) {
    return engine->error;
  }
  else {
    return TLN_ERR_NULL_POINTER;
  }
}

/*!
 * \brief
 * Returns the string description of the specified error code
 * 
 * \param error
 * Error code to get description
 *
 * \see
 * TLN_GetLastError()
 */
const char *TLN_GetErrorString(TLN_Error error) {
#pragma EXPORT_FUNC
  if (error < TLN_MAX_ERR) {
    return errornames[error];
  }
  else {
    return "Invalid error code";
  }
}

/* outputs trace message */
void tln_trace(TLN_LogLevel log_level, const char *format, ...) {
  if (engine != NULL && engine->log_level >= log_level) {
    char line[255];
    va_list ap;

            va_start(ap, format);
    vsprintf(line, format, ap);
            va_end(ap);

    printf("Tilengine: %s\n", line);
  }
}
