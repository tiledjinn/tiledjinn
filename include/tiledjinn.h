/*
 * Tilengine - The 2D retro graphics engine with raster effects
 * Copyright(C) 2015-2019 Marc Palacios Domenech <mailto:megamarc@hotmail.com>
 * Copyright (C) 2022 TileDjinn Contributors
 * All rights reserved
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 * */

#ifndef _TILENGINE_H
#define _TILENGINE_H

/* Common data types */
/* Tilengine shared */
#if defined _MSC_VER

#include <windows.h>

#ifdef LIB_EXPORTS
#define TLNAPI WINAPI
#else
#define TLNAPI WINAPI
#endif

#pragma warning(disable : 4200)

#if _MSC_VER >= 1600  /* Visual C++ 2010? */

#include <stdint.h>

#else
typedef char			int8_t;		/* signed 8-bit wide data */
typedef short			int16_t;	/* signed 16-bit wide data */
typedef int				int32_t;	/* signed 32-bit wide data */
typedef unsigned char	uint8_t;	/* unsigned 8-bit wide data */
typedef unsigned short	uint16_t;	/* unsigned 16-bit wide data */
typedef unsigned int	uint32_t;	/* unsigned 32-bit wide data */
#endif

#if _MSC_VER >= 1800  /* Visual C++ 2013? */

#include <stdbool.h>

#else
typedef unsigned char bool;		/* C++ bool type for C language */
#define false	0
#define true	1
#endif

#define EXPORT_FUNC comment(linker, "/EXPORT:" __FUNCTION__ "=" __FUNCDNAME__)
#else
#ifdef LIB_EXPORTS
#define TLNAPI __attribute__((visibility("default")))
#else
#define TLNAPI
#endif
#include <stdint.h>
#include <stdbool.h>
#define EXPORT_FUNC
#endif

#include <stdio.h>

/* version */
#define TILENGINE_VER_MAJ  1
#define TILENGINE_VER_MIN  0
#define TILENGINE_VER_REV  0
#define TILENGINE_HEADER_VERSION ((TILENGINE_VER_MAJ << 16) | (TILENGINE_VER_MIN << 8) | TILENGINE_VER_REV)

/* tile/sprite flags. Can be none or a combination of the following: */
typedef enum {
    FLAG_NONE = 0,           /* no flags */
    FLAG_FLIPX = 0x8000,     /* horizontal flip */
    FLAG_FLIPY = 0x4000,     /* vertical flip */
    FLAG_ROTATE = 0x2000,    /* row/column flip(unsupported, Tiled compatibility) */
    FLAG_PRIORITY = 0x1000,  /* tile goes in front of sprite layer */
    FLAG_MASKED = 0x0800,    /* sprite won't be drawn inside masked region */
    FLAG_PALETTES = 0x00FF,  /* palette index */
} TLN_TileFlags;

/*
 * layer blend modes. Must be one of these and are mutually exclusive:
 */
typedef enum {
    BLEND_NONE,    /* blending disabled */
    BLEND_MIX25,   /* color averaging 1 */
    BLEND_MIX50,   /* color averaging 2 */
    BLEND_MIX75,   /* color averaging 3 */
    BLEND_ADD,     /* color is always brighter(simulate light effects) */
    BLEND_SUB,     /* color is always darker(simulate shadow effects) */
    BLEND_MOD,     /* color is always darker(simulate shadow effects) */
    BLEND_CUSTOM,  /* user provided blend function with TLN_SetCustomBlendFunction() */
    MAX_BLEND,
    BLEND_MIX = BLEND_MIX50
} TLN_Blend;

/* Affine transformation parameters */
typedef struct {
    float angle;  /* rotation in degrees */
    float dx;    /* horizontal translation */
    float dy;    /* vertical translation */
    float sx;    /* horizontal scaling */
    float sy;    /* vertical scaling */
} TLN_Affine;

/* Tile item for Tilemap access methods */
typedef union Tile {
    uint32_t value;
    struct {
        uint16_t index;    /* tile index */
        uint16_t flags;    /* attributes(FLAG_FLIPX, FLAG_FLIPY, FLAG_PRIORITY) | palettes */
    };
} Tile;

/* Tile information returned by TLN_GetLayerTile() */
typedef struct {
    uint16_t index;  /* tile index */
    uint16_t flags;  /* attributes(FLAG_FLIPX, FLAG_FLIPY, FLAG_PRIORITY) */
    int row;    /* row number in the tilemap */
    int col;    /* col number in the tilemap */
    int xoffset;  /* horizontal position inside the title */
    int yoffset;  /* vertical position inside the title */
    uint8_t color;  /* color index at collision point */
    uint8_t type;  /* tile type */
    bool empty;    /* cell is empty*/
} TLN_TileInfo;

/* Tileset attributes for TLN_CreateTileset() */
typedef struct {
    uint8_t type;    /* tile type */
    bool priority;  /* priority flag set */
} TLN_TileAttributes;

/* overlays for CRT effect */
typedef enum {
    TLN_OVERLAY_NONE,    /* no overlay */
    TLN_OVERLAY_SHADOWMASK,  /* Shadow mask pattern */
    TLN_OVERLAY_APERTURE,  /* Aperture grille pattern */
    TLN_OVERLAY_SCANLINES,  /* Scanlines pattern */
    TLN_OVERLAY_CUSTOM,    /* User-provided when calling TLN_CreateWindow() */
    TLN_MAX_OVERLAY
} TLN_Overlay;

/* pixel mapping for TLN_SetLayerPixelMapping() */
typedef struct {
    int16_t dx;    /* horizontal pixel displacement */
    int16_t dy;    /* vertical pixel displacement */
} TLN_PixelMap;

typedef struct Engine *TLN_Engine;      /* Engine context */
typedef union Tile *TLN_Tile;        /* Tile reference */
typedef struct Tileset *TLN_Tileset;      /* Opaque tileset reference */
typedef struct Tilemap *TLN_Tilemap;      /* Opaque tilemap reference */
typedef uint8_t TLN_PaletteId;      /* Opaque palette reference */

/* Sprite state */
typedef struct {
    int x;            /* Screen position x */
    int y;            /* Screen position y */
    int w;            /* Actual width in screen(after scaling) */
    int h;            /* Actual height in screen(after scaling) */
    uint32_t flags;        /* flags */
    int index;          /* graphic index inside spriteset */
    bool enabled;        /* enabled or not */
    bool collision;        /* per-pixel collision detection enabled or not */
} TLN_SpriteState;

/* callbacks */
/* void argument here is SDL_Event. void to remove dependency and open up for other back ends */
typedef void(*TLN_SDLCallback)(void *);
typedef void(*TLN_VideoCallback)(int scanline);
typedef uint8_t(*TLN_BlendFunction)(uint8_t src, uint8_t dst);

/* Player index for input assignment functions */
typedef enum {
    PLAYER1,  /* Player 1 */
    PLAYER2,  /* Player 2 */
    PLAYER3,  /* Player 3 */
    PLAYER4,  /* Player 4 */
} TLN_Player;

/* Standard inputs query for TLN_GetInput() */
typedef enum {
    INPUT_NONE,    /* no input */
    INPUT_UP,    /* up direction */
    INPUT_DOWN,    /* down direction */
    INPUT_LEFT,    /* left direction */
    INPUT_RIGHT,  /* right direction */
    INPUT_BUTTON1,  /* 1st action button */
    INPUT_BUTTON2,  /* 2nd action button */
    INPUT_BUTTON3,  /* 3th action button */
    INPUT_BUTTON4,  /* 4th action button */
    INPUT_BUTTON5,  /* 5th action button */
    INPUT_BUTTON6,  /* 6th action button */
    INPUT_START,  /* Start button */

    /* ... up to 32 unique inputs */

    INPUT_P1 = (PLAYER1 << 5),  /* request player 1 input(default) */
    INPUT_P2 = (PLAYER2 << 5),  /* request player 2 input */
    INPUT_P3 = (PLAYER3 << 5),  /* request player 3 input */
    INPUT_P4 = (PLAYER4 << 5),  /* request player 4 input */
} TLN_Input;

/* CreateWindow flags. Can be none or a combination of the following: */
enum {
    CWF_FULLSCREEN = (1 << 0),  /* create a fullscreen window */
    CWF_VSYNC = (1 << 1),  /* sync frame updates with vertical retrace */
    CWF_S1 = (1 << 2),  /* create a window the same size as the framebuffer */
    CWF_S2 = (2 << 2),  /* create a window 2x the size the framebuffer */
    CWF_S3 = (3 << 2),  /* create a window 3x the size the framebuffer */
    CWF_S4 = (4 << 2),  /* create a window 4x the size the framebuffer */
    CWF_S5 = (5 << 2),  /* create a window 5x the size the framebuffer */
    CWF_NEAREST = (1 << 6),  /*<! unfiltered upscaling */
};

/* Error codes */
typedef enum {
    TLN_ERR_OK,        /* No error */
    TLN_ERR_OUT_OF_MEMORY,  /* Not enough memory */
    TLN_ERR_IDX_LAYER,    /* Layer index out of range */
    TLN_ERR_IDX_SPRITE,    /* Sprite index out of range */
    TLN_ERR_IDX_ANIMATION,  /* Animation index out of range */
    TLN_ERR_IDX_PICTURE,  /* Picture or tile index out of range */
    TLN_ERR_REF_TILESET,  /* Invalid TLN_Tileset reference */
    TLN_ERR_REF_TILEMAP,  /* Invalid TLN_Tilemap reference */
    TLN_ERR_REF_SPRITESET,  /* Invalid TLN_Spriteset reference */
    TLN_ERR_REF_PALETTE,  /* Invalid TLN_Palette reference */
    TLN_ERR_REF_SEQUENCE,  /* Invalid TLN_Sequence reference */
    TLN_ERR_REF_SEQPACK,  /* Invalid TLN_SequencePack reference */
    TLN_ERR_REF_BITMAP,    /* Invalid TLN_Bitmap reference */
    TLN_ERR_NULL_POINTER,  /* Null pointer as argument */
    TLN_ERR_FILE_NOT_FOUND,  /* Resource file not found */
    TLN_ERR_WRONG_FORMAT,  /* Resource file has invalid format */
    TLN_ERR_WRONG_SIZE,    /* A width or height parameter is invalid */
    TLN_ERR_UNSUPPORTED,  /* Unsupported function */
    TLN_ERR_REF_LIST,    /* Invalid TLN_ObjectList reference */
    TLN_MAX_ERR,
} TLN_Error;

/* Debug level */
typedef enum {
    TLN_LOG_NONE,    /* Don't print anything(default) */
    TLN_LOG_ERRORS,    /* Print only runtime errors */
    TLN_LOG_VERBOSE,  /* Print everything */
} TLN_LogLevel;


#ifdef __cplusplus
extern "C"{
#endif

/* Basic setup and management */
TLN_Engine TLNAPI TLN_Init(int hres, int vres, int numlayers, int numsprites);
void TLNAPI TLN_Deinit(void);
bool TLNAPI TLN_DeleteContext(TLN_Engine context);
bool TLNAPI TLN_SetContex(TLN_Engine context);
TLN_Engine TLNAPI TLN_GetContex(void);
int TLNAPI TLN_GetWidth(void);
int TLNAPI TLN_GetHeight(void);
uint32_t TLNAPI TLN_GetNumObjects(void);
uint32_t TLNAPI TLN_GetUsedMemory(void);
uint32_t TLNAPI TLN_GetVersion(void);
int TLNAPI TLN_GetNumLayers(void);
int TLNAPI TLN_GetNumSprites(void);
void TLNAPI TLN_SetBGColor(uint8_t r, uint8_t g, uint8_t b);
bool TLNAPI TLN_SetBGColorFromTilemap(TLN_Tilemap tilemap);
void TLNAPI TLN_DisableBGColor(void);
void TLNAPI TLN_SetRasterCallback(TLN_VideoCallback);
void TLNAPI TLN_SetFrameCallback(TLN_VideoCallback);
void TLNAPI TLN_SetRenderTarget(uint8_t *data, int pitch);
void TLNAPI TLN_UpdateFrame(int frame);
void TLNAPI TLN_SetCustomBlendFunction(TLN_BlendFunction);
void TLNAPI TLN_SetLogLevel(TLN_LogLevel log_level);

/* Basic setup and management */
void TLNAPI TLN_SetLastError(TLN_Error error);
TLN_Error TLNAPI TLN_GetLastError(void);
const char *TLNAPI TLN_GetErrorString(TLN_Error error);

/* Built-in window and input management */
bool TLNAPI TLN_CreateWindow(const char *overlay, int flags);
bool TLNAPI TLN_CreateWindowThread(const char *overlay, int flags);
void TLNAPI TLN_SetWindowTitle(const char *title);
bool TLNAPI TLN_ProcessWindow(void);
bool TLNAPI TLN_IsWindowActive(void);
bool TLNAPI TLN_GetInput(TLN_Input id);
void TLNAPI TLN_EnableInput(TLN_Player player, bool enable);
void TLNAPI TLN_AssignInputJoystick(TLN_Player player, int index);
void TLNAPI TLN_DefineInputKey(TLN_Player player, TLN_Input input, int32_t keycode);
void TLNAPI TLN_DefineInputButton(TLN_Player player, TLN_Input input, uint8_t joybutton);
void TLNAPI TLN_DrawFrame(int frame);
void TLNAPI TLN_WaitRedraw(void);
void TLNAPI TLN_DeleteWindow(void);

void TLNAPI TLN_EnableBlur(bool mode);
void TLNAPI TLN_EnableCRTEffect(TLN_Overlay overlay, uint8_t overlay_factor, uint8_t threshold, uint8_t v0, uint8_t v1,
                                uint8_t v2, uint8_t v3, bool blur, uint8_t glow_factor);
void TLNAPI TLN_DisableCRTEffect(void);

void TLNAPI TLN_SetSDLCallback(TLN_SDLCallback);

void TLNAPI TLN_Delay(uint32_t msecs);
uint32_t TLNAPI TLN_GetTicks(void);
int TLNAPI TLN_GetWindowWidth(void);
int TLNAPI TLN_GetWindowHeight(void);

/* Tileset resources management for background layers  */
TLN_Tileset TLNAPI TLN_CreateTileset(int numtiles, int width, int height, TLN_TileAttributes *attributes);
TLN_Tileset TLNAPI TLN_CloneTileset(TLN_Tileset src);
bool TLNAPI TLN_SetTilesetPixels(TLN_Tileset tileset, int entry, uint8_t *srcdata, int srcpitch);
const uint8_t * TLN_GetTilesetPixels(TLN_Tileset tileset, int entry);
int TLNAPI TLN_GetTileWidth(TLN_Tileset tileset);
int TLNAPI TLN_GetTileHeight(TLN_Tileset tileset);
int TLNAPI TLN_GetTilesetNumTiles(TLN_Tileset tileset);
bool TLNAPI TLN_DeleteTileset(TLN_Tileset tileset);

/* Tilemap resources management for background layers  */
TLN_Tilemap TLNAPI TLN_CreateTilemap(int rows, int cols, TLN_Tile tiles, uint32_t bgcolor, TLN_Tileset tileset);
TLN_Tilemap TLNAPI TLN_CloneTilemap(TLN_Tilemap src);
int TLNAPI TLN_GetTilemapRows(TLN_Tilemap tilemap);
int TLNAPI TLN_GetTilemapCols(TLN_Tilemap tilemap);
TLN_Tileset TLNAPI TLN_GetTilemapTileset(TLN_Tilemap tilemap);
bool TLNAPI TLN_GetTilemapTile(TLN_Tilemap tilemap, int row, int col, TLN_Tile tile);
bool TLNAPI TLN_SetTilemapTile(TLN_Tilemap tilemap, int row, int col, TLN_Tile tile);
bool TLNAPI TLN_CopyTiles(TLN_Tilemap src, int srcrow, int srccol, int rows, int cols, TLN_Tilemap dst, int dstrow,
                          int dstcol);
bool TLNAPI TLN_DeleteTilemap(TLN_Tilemap tilemap);

/* Color palette resources management for sprites and background layers */
bool TLNAPI TLN_CreatePalette(unsigned char id, int entries);
bool TLNAPI TLN_SetPaletteColor(TLN_PaletteId palette_id, int index, uint8_t r, uint8_t g, uint8_t b);
bool TLNAPI TLN_AddPaletteColor(TLN_PaletteId palette_id, uint8_t r, uint8_t g, uint8_t b, uint8_t start, uint8_t num);
bool TLNAPI TLN_SubPaletteColor(TLN_PaletteId palette, uint8_t r, uint8_t g, uint8_t b, uint8_t start, uint8_t num);
bool TLNAPI TLN_ModPaletteColor(TLN_PaletteId palette, uint8_t r, uint8_t g, uint8_t b, uint8_t start, uint8_t num);
const uint8_t *TLNAPI TLN_GetPaletteData(TLN_PaletteId palette_id, int index);
bool TLNAPI TLN_DeletePalette(TLN_PaletteId palette);

/* Background layers management */
bool TLNAPI TLN_SetLayerTilemap(int nlayer, TLN_Tilemap tilemap);
bool TLNAPI TLN_SetLayerPosition(int nlayer, int hstart, int vstart);
bool TLNAPI TLN_SetLayerScaling(int nlayer, float xfactor, float yfactor);
bool TLNAPI TLN_SetLayerAffineTransform(int nlayer, TLN_Affine *affine);
bool TLNAPI TLN_SetLayerTransform(int layer, float angle, float dx, float dy, float sx, float sy);
bool TLNAPI TLN_SetLayerPixelMapping(int nlayer, TLN_PixelMap *table);
bool TLNAPI TLN_SetLayerBlendMode(int nlayer, TLN_Blend mode);
bool TLNAPI TLN_SetLayerColumnOffset(int nlayer, int *offset);
bool TLNAPI TLN_SetLayerClip(int nlayer, int x1, int y1, int x2, int y2);
bool TLNAPI TLN_DisableLayerClip(int nlayer);
bool TLNAPI TLN_SetLayerMosaic(int nlayer, int width, int height);
bool TLNAPI TLN_DisableLayerMosaic(int nlayer);
bool TLNAPI TLN_ResetLayerMode(int nlayer);
bool TLNAPI TLN_SetLayerPriority(int nlayer, bool enable);
bool TLNAPI TLN_DisableLayer(int nlayer);
bool TLNAPI TLN_EnableLayer(int nlayer);
TLN_Tileset TLNAPI TLN_GetLayerTileset(int nlayer);
TLN_Tilemap TLNAPI TLN_GetLayerTilemap(int nlayer);
bool TLNAPI TLN_GetLayerTile(int nlayer, int x, int y, TLN_TileInfo *info);
int TLNAPI TLN_GetLayerWidth(int nlayer);
int TLNAPI TLN_GetLayerHeight(int nlayer);


/* Sprites management */
bool TLNAPI TLN_EnableSpriteFlag(int nsprite, uint32_t flag, bool enable);
bool TLNAPI TLN_SetSpritePivot(int nsprite, float px, float py);
bool TLNAPI TLN_SetSpritePosition(int nsprite, int x, int y);
bool TLNAPI TLN_SetSpritePicture(int nsprite, TLN_Tileset tileset, int entry);
bool TLNAPI TLN_SetSpritePalette(int nsprite, TLN_PaletteId palette_id);
bool TLNAPI TLN_SetSpriteBlendMode(int nsprite, TLN_Blend mode);
bool TLNAPI TLN_SetSpriteScaling(int nsprite, float sx, float sy);
bool TLNAPI TLN_ResetSpriteScaling(int nsprite);
int TLNAPI TLN_GetSpritePicture(int nsprite);
int TLNAPI TLN_GetAvailableSprite(void);
bool TLNAPI TLN_EnableSpriteCollision(int nsprite, bool enable);
bool TLNAPI TLN_GetSpriteCollision(int nsprite);
bool TLNAPI TLN_GetSpriteState(int nsprite, TLN_SpriteState *state);
void TLNAPI TLN_SetSpritesMaskRegion(int top_line, int bottom_line);
bool TLNAPI TLN_DisableSprite(int nsprite);
bool TLNAPI TLN_EnableSprite(int nsprite);
TLN_PaletteId TLNAPI TLN_GetSpritePalette(int nsprite);

/* World management */
void TLNAPI TLN_SetWorldPosition(int x, int y);
bool TLNAPI TLN_SetLayerParallaxFactor(int nlayer, float x, float y);
bool TLNAPI TLN_SetSpriteWorldPosition(int nsprite, int x, int y);

#ifdef __cplusplus
}
#endif

#endif
