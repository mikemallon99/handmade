#ifndef HANDMADE_SPRITE_H
#define HANDMADE_SPRITE_H

enum overworld_tile_id {
    OW_Null,   // 0

    OW_Floor,  // 1
    OW_Floor_Dusty, // 2

    OW_Wall_TopLeft, // 3
    OW_Wall_TopMid, // 4
    OW_Wall_TopRight, // 5
    OW_Wall_BotLeft, // 6
    OW_Wall_BotMid, // 7
    OW_Wall_BotRight, // 8

    OW_Bush, // 9
    OW_Entrance, // 10

    OW_TileCount
};

// NOTE: future unified image format (AARRGGBB)
struct bmp_file
{
    uint32 Width;
    uint32 Height;
    uint16 BitsPerPixel;
    uint32 ImageSize;
    uint32* Pixels;
};

struct bmp_tile
{
    bmp_file *Tileset;
    uint32 X;
    uint32 Y;
    uint32 AnchorX;
    uint32 AnchorY;
    uint32 Width;
    uint32 Height;
    bool32 FlipX;
    bool32 FlipY;
};

struct overworld_tileset
{
    bmp_file *BaseBMP;

    // NOTE: Changes based on OverworldSprite enum
    bmp_tile Tiles[OW_TileCount];
};

struct text_tileset
{
    bmp_file *BaseBMP;

    bmp_tile Tiles[48];
};

struct hud_tileset
{
    bmp_file *BaseBMP;

    bmp_tile Hearts[3];
};

struct link_sprites
{
    bmp_file *BaseBMP;
    bmp_tile Front[2];
    bmp_tile Back[2];
    bmp_tile Left[2];
    bmp_tile Right[2];
    bmp_tile SwordFront[4];
    bmp_tile SwordRight[4];
    bmp_tile SwordBack[4];
    bmp_tile SwordLeft[4];
};

struct octorok_sprites
{
    bmp_file *BaseBMP;
    bmp_tile Front[2];
    bmp_tile Back[2];
    bmp_tile Left[2];
    bmp_tile Right[2];
    bmp_tile Projectile;
};

struct moblin_sprites
{
    bmp_file *BaseBMP;
    bmp_tile Front[2];
    bmp_tile Back[2];
    bmp_tile Left[2];
    bmp_tile Right[2];
    bmp_tile ArrowFront;
    bmp_tile ArrowBack;
    bmp_tile ArrowLeft;
    bmp_tile ArrowRight;
};

#endif