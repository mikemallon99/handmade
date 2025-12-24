internal int32
GetBitShift(uint32 Value)
{
    if (Value == 0xFF) 
    {
        return 0;
    }
    else if (Value == 0xFF00) 
    {
        return 8;
    }
    else if (Value == 0xFF0000) 
    {
        return 16;
    }
    else if (Value == 0xFF000000) 
    {
        return 24;
    }
    return 0;
}


internal bmp_file
LoadBMPFile(memory_arena *Arena, game_memory *Memory, thread_context *Thread, char *Filename)
{
    debug_read_file_result File = Memory->DEBUGPlatformReadEntireFile(Thread, Filename);
    bmp_file BMPFile = {};
    if (File.Contents)
    {
        uint32 BMPSize = *((uint32 *)((uint8 *)File.Contents + 2));
        uint32 Offset = *((uint32 *)((uint8 *)File.Contents + 10));
        uint32 *FilePixels = (uint32 *)((uint8 *)File.Contents + Offset);

        BMPFile.Width = *((int32 *)((uint8 *)File.Contents + 18));
        BMPFile.Height = *((int32 *)((uint8 *)File.Contents + 22));
        BMPFile.BitsPerPixel = *((int16 *)((uint8 *)File.Contents + 28));
        BMPFile.ImageSize = *((uint32 *)((uint8 *)File.Contents + 34));
        uint32 CompressionMethod = *((uint32 *)((uint8 *)File.Contents + 30));

        uint32 TransparentColor1 = 0xFF747474;
        uint32 TransparentColor2 = 0xFF008000;

        // NOTE: These values arent tested, ive only tested using the bitmask
        uint32 RedMask = 0xFF000000;
        uint32 GreenMask = 0xFF0000;
        uint32 BlueMask = 0xFF00;
        uint32 AlphaMask = 0xFF;
        // TODO: Fix this copy paste or it will bite me in butthole
        if (BMPFile.BitsPerPixel == 32)
        {
            if (CompressionMethod == 3)
            {
                RedMask = *((uint32 *)((uint8 *)File.Contents + 0x36));
                GreenMask = *((uint32 *)((uint8 *)File.Contents + 0x3A));
                BlueMask = *((uint32 *)((uint8 *)File.Contents + 0x3E));
                AlphaMask = *((uint32 *)((uint8 *)File.Contents + 0x42));
            }

            int32 RedShift = GetBitShift(RedMask);
            int32 GreenShift = GetBitShift(GreenMask);
            int32 BlueShift = GetBitShift(BlueMask);
            int32 AlphaShift = GetBitShift(AlphaMask);

            uint32 PixelCount = BMPFile.Width*BMPFile.Height;
            uint32 *MemoryPixels = PushArray(Arena, PixelCount, uint32);
            BMPFile.Pixels = MemoryPixels;
            uint32 *CopyPixel;
            // NOTE: It goes top to bottom, i wanna reverse it so the image pixels always starts at the top left corner
            for (uint32 RowIdx = BMPFile.Height; 
                RowIdx > 0; 
                RowIdx--)
            {
                CopyPixel = FilePixels + BMPFile.Width*(RowIdx-1);
                for (uint32 ColIdx = 0; 
                    ColIdx < BMPFile.Width; 
                    ColIdx++)
                {
                    // NOTE: Should this pixel flipping happen at load or at draw ?
                    uint32 Pixel = *CopyPixel++;
                    uint32 R = 0xFF & (Pixel >> RedShift);
                    uint32 G = 0xFF & (Pixel >> GreenShift);
                    uint32 B = 0xFF & (Pixel >> BlueShift);
                    uint32 A = 0xFF & (Pixel >> AlphaShift);
                    Pixel = (A << 24) | (R << 16) | (G << 8) | (B);
                    if (Pixel == TransparentColor1 || Pixel == TransparentColor2)
                    {
                        Pixel = 0;
                    }
                    *MemoryPixels++ = Pixel;
                }
            }
        }
        else if (BMPFile.BitsPerPixel == 24)
        {
            uint32 PixelCount = BMPFile.Width*BMPFile.Height;
            uint32 *MemoryPixels = PushArray(Arena, PixelCount, uint32);
            BMPFile.Pixels = MemoryPixels;
            uint32 RowSize = ((BMPFile.Width * 3 + 3) / 4) * 4;
            uint8 *CopyPixel;
            // NOTE: It goes top to bottom, i wanna reverse it so the image pixels always starts at the top left corner
            for (uint32 RowIdx = BMPFile.Height; 
                RowIdx > 0; 
                RowIdx--)
            {
                CopyPixel = (uint8 *)FilePixels + RowSize*(RowIdx-1);
                for (uint32 ColIdx = 0; 
                    ColIdx < BMPFile.Width; 
                    ColIdx++)
                {
                    // NOTE: Should this pixel flipping happen at load or at draw ?
                    uint32 B = 0xFF & (uint32)*CopyPixel++;
                    uint32 G = 0xFF & (uint32)*CopyPixel++;
                    uint32 R = 0xFF & (uint32)*CopyPixel++;
                    uint32 A = 0xFF;
                    uint32 Pixel = (A << 24) | (R << 16) | (G << 8) | (B);
                    if (Pixel == TransparentColor1 || Pixel == TransparentColor2)
                    {
                        Pixel = 0;
                    }
                    *MemoryPixels++ = Pixel;
                }
            }
        }
        else
        {
            // Have not implemented importer for this format
            Assert(0);
        }

        Memory->DEBUGPlatformFreeFileMemory(Thread, File.Contents);
    }
    return BMPFile;
}

internal void
WritePixelValue(uint32 *Source, uint32 *Dest)
{
    uint32 AlphaMask = 0xFF000000;
    uint32 Alpha = (*Source & AlphaMask) >> 24;
    // No color data
    if (Alpha == 0)
    {
    }
    // All Color data
    else if (Alpha == 0xFF)
    {
        *Dest = *Source;
    }
    // Alpha blending
    else 
    {
        real32 AlphaRatio = (real32)Alpha / (real32)0xFF;

        // uint32 BufferA = (0xFF000000 & *BufferPixel) >> 24;
        uint32 DestR = (0xFF0000 & *Dest) >> 16;
        uint32 DestG = (0xFF00 & *Dest) >> 8;
        uint32 DestB = (0xFF & *Dest) >> 0;

        // uint32 ImageA = (0xFF000000 & *ImagePixel) >> 24;
        uint32 SourceR = (0xFF0000 & *Source) >> 16;
        uint32 SourceG = (0xFF00 & *Source) >> 8;
        uint32 SourceB = (0xFF & *Source) >> 0;

        uint32 A = 0xFF;
        uint32 R = (uint32)(((real32)DestR * (1.0f - AlphaRatio)) + ((real32)SourceR * (AlphaRatio)));
        uint32 G = (uint32)(((real32)DestG * (1.0f - AlphaRatio)) + ((real32)SourceG * (AlphaRatio)));
        uint32 B = (uint32)(((real32)DestB * (1.0f - AlphaRatio)) + ((real32)SourceB * (AlphaRatio)));
        
        *Source = (A << 24) | (R << 16) | (G << 8) | (B << 0);
    }
}

internal void
DrawBMPFile(bmp_file *BMPFile, game_offscreen_buffer *Buffer,
            real32 RealMinX, real32 RealMinY)
{
    int32 MinX = RoundReal32ToInt32(RealMinX);
    int32 MinY = RoundReal32ToInt32(RealMinY);

    // Should bottom to top drawing happen at load or at draw?
    // TODO: Probably move this stuff to the load once we add in new image formats, then make a unified image struct
    for (int32 RowIdx = 0;
         RowIdx < (int32)BMPFile->Height;
         RowIdx++
        )
    {
        for (int32 ColIdx = 0;
            ColIdx < (int32)BMPFile->Width;
            ColIdx++
            )
        {
            int32 DestX = MinX + ColIdx;
            if (DestX < 0)
            {
                continue;
            }
            if (DestX >= (int32)Buffer->Width)
            {
                continue;
            }

            int32 DestY = MinY + RowIdx;
            if (DestY < 0)
            {
                continue;
            }
            if (DestY >= (int32)Buffer->Height)
            {
                continue;
            }

            uint32 SourceX = ColIdx;
            uint32 SourceY = RowIdx;

            uint32 ImageOffset = SourceY*BMPFile->Width + SourceX;
            uint32 *ImagePixel = BMPFile->Pixels + ImageOffset;
            uint32 BufferOffset = DestY*Buffer->Width + DestX;
            uint32 *BufferPixel = (uint32 *)Buffer->Memory + BufferOffset;
            WritePixelValue(ImagePixel, BufferPixel);
        }
    }
}


internal void
DrawBMPTile(bmp_tile *BMPTile, game_offscreen_buffer *Buffer,
            real32 RealMinX, real32 RealMinY)
{
    int32 MinX = RoundReal32ToInt32(RealMinX);
    int32 MinY = RoundReal32ToInt32(RealMinY);

    int32 OffsetX = 0;
    if (BMPTile->AnchorX)
    {
        if (BMPTile->FlipX)
        {
            OffsetX = (BMPTile->X + BMPTile->Width) - BMPTile->AnchorX;
        }
        else
        {
            OffsetX = BMPTile->AnchorX - BMPTile->X;
        }
    }
    int32 OffsetY = 0;
    if (BMPTile->AnchorY)
    {
        OffsetY = BMPTile->AnchorY - BMPTile->Y;
    }

    // Should bottom to top drawing happen at load or at draw?
    // TODO: Probably move this stuff to the load once we add in new image formats, then make a unified image struct
    bmp_file *BMPBaseFile = BMPTile->Tileset;
    for (int32 RowIdx = 0;
         RowIdx < (int32)BMPTile->Height;
         RowIdx++
        )
    {
        for (int32 ColIdx = 0;
            ColIdx < (int32)BMPTile->Width;
            ColIdx++
            )
        {
            int32 DestX = MinX + ColIdx;
            DestX -= OffsetX;

            if (DestX < 0)
            {
                continue;
            }
            if (DestX >= (int32)Buffer->Width)
            {
                continue;
            }

            int32 DestY = MinY + RowIdx;
            DestY -= OffsetY;
            if (DestY < 0)
            {
                continue;
            }
            if (DestY >= (int32)Buffer->Height)
            {
                continue;
            }

            uint32 SourceX = BMPTile->X + ColIdx;
            // NOTE: If X is flipped, then grab from buffer in reverse order
            if (BMPTile->FlipX)
            {
                SourceX = BMPTile->X + BMPTile->Width - ColIdx;
            }
            // NOTE: If Y is flipped, then grab from buffer in reverse order
            uint32 SourceY = BMPTile->Y + RowIdx;
            if (BMPTile->FlipY)
            {
                SourceY = BMPTile->Y + BMPTile->Height - RowIdx;
            }

            uint32 ImageOffset = SourceY*BMPBaseFile->Width + SourceX;
            uint32 *ImagePixel = BMPBaseFile->Pixels + ImageOffset;
            uint32 BufferOffset = DestY*Buffer->Width + DestX;
            uint32 *BufferPixel = (uint32 *)Buffer->Memory + BufferOffset;
            WritePixelValue(ImagePixel, BufferPixel);
        }
    }
}

internal void
LoadOverworldTileset(overworld_tileset *OverworldTileset, bmp_file *BaseBMP)
{
        OverworldTileset->BaseBMP = BaseBMP;

        OverworldTileset->Tiles[OW_Floor].Tileset = OverworldTileset->BaseBMP;
        OverworldTileset->Tiles[OW_Floor].X = 1;
        OverworldTileset->Tiles[OW_Floor].Y = 154;
        OverworldTileset->Tiles[OW_Floor].Width = 16;
        OverworldTileset->Tiles[OW_Floor].Height = 16;

        OverworldTileset->Tiles[OW_Floor_Dusty].Tileset = OverworldTileset->BaseBMP;
        OverworldTileset->Tiles[OW_Floor_Dusty].X = 1;
        OverworldTileset->Tiles[OW_Floor_Dusty].Y = 171;
        OverworldTileset->Tiles[OW_Floor_Dusty].Width = 16;
        OverworldTileset->Tiles[OW_Floor_Dusty].Height = 16;

        OverworldTileset->Tiles[OW_Floor_Black].Tileset = OverworldTileset->BaseBMP;
        OverworldTileset->Tiles[OW_Floor_Black].X = 137;
        OverworldTileset->Tiles[OW_Floor_Black].Y = 205;
        OverworldTileset->Tiles[OW_Floor_Black].Width = 16;
        OverworldTileset->Tiles[OW_Floor_Black].Height = 16;

        OverworldTileset->Tiles[OW_Wall_TopLeft].Tileset = OverworldTileset->BaseBMP;
        OverworldTileset->Tiles[OW_Wall_TopLeft].X = 18;
        OverworldTileset->Tiles[OW_Wall_TopLeft].Y = 154;
        OverworldTileset->Tiles[OW_Wall_TopLeft].Width = 16;
        OverworldTileset->Tiles[OW_Wall_TopLeft].Height = 16;

        OverworldTileset->Tiles[OW_Wall_TopMid].Tileset = OverworldTileset->BaseBMP;
        OverworldTileset->Tiles[OW_Wall_TopMid].X = 35;
        OverworldTileset->Tiles[OW_Wall_TopMid].Y = 154;
        OverworldTileset->Tiles[OW_Wall_TopMid].Width = 16;
        OverworldTileset->Tiles[OW_Wall_TopMid].Height = 16;

        OverworldTileset->Tiles[OW_Wall_TopRight].Tileset = OverworldTileset->BaseBMP;
        OverworldTileset->Tiles[OW_Wall_TopRight].X = 52;
        OverworldTileset->Tiles[OW_Wall_TopRight].Y = 154;
        OverworldTileset->Tiles[OW_Wall_TopRight].Width = 16;
        OverworldTileset->Tiles[OW_Wall_TopRight].Height = 16;

        OverworldTileset->Tiles[OW_Wall_BotLeft].Tileset = OverworldTileset->BaseBMP;
        OverworldTileset->Tiles[OW_Wall_BotLeft].X = 18;
        OverworldTileset->Tiles[OW_Wall_BotLeft].Y = 171;
        OverworldTileset->Tiles[OW_Wall_BotLeft].Width = 16;
        OverworldTileset->Tiles[OW_Wall_BotLeft].Height = 16;

        OverworldTileset->Tiles[OW_Wall_BotMid].Tileset = OverworldTileset->BaseBMP;
        OverworldTileset->Tiles[OW_Wall_BotMid].X = 35;
        OverworldTileset->Tiles[OW_Wall_BotMid].Y = 171;
        OverworldTileset->Tiles[OW_Wall_BotMid].Width = 16;
        OverworldTileset->Tiles[OW_Wall_BotMid].Height = 16;

        OverworldTileset->Tiles[OW_Wall_BotRight].Tileset = OverworldTileset->BaseBMP;
        OverworldTileset->Tiles[OW_Wall_BotRight].X = 52;
        OverworldTileset->Tiles[OW_Wall_BotRight].Y = 171;
        OverworldTileset->Tiles[OW_Wall_BotRight].Width = 16;
        OverworldTileset->Tiles[OW_Wall_BotRight].Height = 16;

        OverworldTileset->Tiles[OW_Bush].Tileset = OverworldTileset->BaseBMP;
        OverworldTileset->Tiles[OW_Bush].X = 18;
        OverworldTileset->Tiles[OW_Bush].Y = 188;
        OverworldTileset->Tiles[OW_Bush].Width = 16;
        OverworldTileset->Tiles[OW_Bush].Height = 16;

        OverworldTileset->Tiles[OW_Entrance].Tileset = OverworldTileset->BaseBMP;
        OverworldTileset->Tiles[OW_Entrance].X = 137;
        OverworldTileset->Tiles[OW_Entrance].Y = 205;
        OverworldTileset->Tiles[OW_Entrance].Width = 16;
        OverworldTileset->Tiles[OW_Entrance].Height = 16;
}

internal void
LoadDungeonTileset(dungeon_tileset *DungeonTileset, bmp_file *BaseBMP)
{
        DungeonTileset->BaseBMP = BaseBMP;

        DungeonTileset->Tiles[DN_Floor].Tileset = DungeonTileset->BaseBMP;
        DungeonTileset->Tiles[DN_Floor].X = 984;
        DungeonTileset->Tiles[DN_Floor].Y = 11;
        DungeonTileset->Tiles[DN_Floor].Width = 16;
        DungeonTileset->Tiles[DN_Floor].Height = 16;

        DungeonTileset->Tiles[DN_Block].Tileset = DungeonTileset->BaseBMP;
        DungeonTileset->Tiles[DN_Block].X = 1001;
        DungeonTileset->Tiles[DN_Block].Y = 11;
        DungeonTileset->Tiles[DN_Block].Width = 16;
        DungeonTileset->Tiles[DN_Block].Height = 16;

        DungeonTileset->Tiles[DN_Statue1].Tileset = DungeonTileset->BaseBMP;
        DungeonTileset->Tiles[DN_Statue1].X = 1018;
        DungeonTileset->Tiles[DN_Statue1].Y = 11;
        DungeonTileset->Tiles[DN_Statue1].Width = 16;
        DungeonTileset->Tiles[DN_Statue1].Height = 16;

        DungeonTileset->Tiles[DN_Statue2].Tileset = DungeonTileset->BaseBMP;
        DungeonTileset->Tiles[DN_Statue2].X = 1035;
        DungeonTileset->Tiles[DN_Statue2].Y = 11;
        DungeonTileset->Tiles[DN_Statue2].Width = 16;
        DungeonTileset->Tiles[DN_Statue2].Height = 16;

        DungeonTileset->Tiles[DN_Black].Tileset = DungeonTileset->BaseBMP;
        DungeonTileset->Tiles[DN_Black].X = 984;
        DungeonTileset->Tiles[DN_Black].Y = 28;
        DungeonTileset->Tiles[DN_Black].Width = 16;
        DungeonTileset->Tiles[DN_Black].Height = 16;

        DungeonTileset->Tiles[DN_Gravel].Tileset = DungeonTileset->BaseBMP;
        DungeonTileset->Tiles[DN_Gravel].X = 1001;
        DungeonTileset->Tiles[DN_Gravel].Y = 28;
        DungeonTileset->Tiles[DN_Gravel].Width = 16;
        DungeonTileset->Tiles[DN_Gravel].Height = 16;

        DungeonTileset->Tiles[DN_Water].Tileset = DungeonTileset->BaseBMP;
        DungeonTileset->Tiles[DN_Water].X = 1018;
        DungeonTileset->Tiles[DN_Water].Y = 28;
        DungeonTileset->Tiles[DN_Water].Width = 16;
        DungeonTileset->Tiles[DN_Water].Height = 16;

        DungeonTileset->Tiles[DN_Stairs].Tileset = DungeonTileset->BaseBMP;
        DungeonTileset->Tiles[DN_Stairs].X = 1035;
        DungeonTileset->Tiles[DN_Stairs].Y = 28;
        DungeonTileset->Tiles[DN_Stairs].Width = 16;
        DungeonTileset->Tiles[DN_Stairs].Height = 16;

        DungeonTileset->Tiles[DN_GreyWall].Tileset = DungeonTileset->BaseBMP;
        DungeonTileset->Tiles[DN_GreyWall].X = 984;
        DungeonTileset->Tiles[DN_GreyWall].Y = 45;
        DungeonTileset->Tiles[DN_GreyWall].Width = 16;
        DungeonTileset->Tiles[DN_GreyWall].Height = 16;

        DungeonTileset->Tiles[DN_GreyLadder].Tileset = DungeonTileset->BaseBMP;
        DungeonTileset->Tiles[DN_GreyLadder].X = 1001;
        DungeonTileset->Tiles[DN_GreyLadder].Y = 45;
        DungeonTileset->Tiles[DN_GreyLadder].Width = 16;
        DungeonTileset->Tiles[DN_GreyLadder].Height = 16;

        DungeonTileset->RoomBorder.Tileset = DungeonTileset->BaseBMP;
        DungeonTileset->RoomBorder.X = 521;
        DungeonTileset->RoomBorder.Y = 11;
        DungeonTileset->RoomBorder.Width = 256;
        DungeonTileset->RoomBorder.Height = 176;

        for (int32 i = 0; i < 5; i++)
        {
            DungeonTileset->DoorsTop[i].Tileset = DungeonTileset->BaseBMP;
            DungeonTileset->DoorsTop[i].X = 815 + 17*i;
            DungeonTileset->DoorsTop[i].Y = 11;
            DungeonTileset->DoorsTop[i].Width = 32;
            DungeonTileset->DoorsTop[i].Height = 32;
        }

        for (int32 i = 0; i < 5; i++)
        {
            DungeonTileset->DoorsLeft[i].Tileset = DungeonTileset->BaseBMP;
            DungeonTileset->DoorsLeft[i].X = 815 + 17*i;
            DungeonTileset->DoorsLeft[i].Y = 44;
            DungeonTileset->DoorsLeft[i].Width = 32;
            DungeonTileset->DoorsLeft[i].Height = 32;
        }

        for (int32 i = 0; i < 5; i++)
        {
            DungeonTileset->DoorsRight[i].Tileset = DungeonTileset->BaseBMP;
            DungeonTileset->DoorsRight[i].X = 815 + 17*i;
            DungeonTileset->DoorsRight[i].Y = 77;
            DungeonTileset->DoorsRight[i].Width = 32;
            DungeonTileset->DoorsRight[i].Height = 32;
        }

        for (int32 i = 0; i < 5; i++)
        {
            DungeonTileset->DoorsBottom[i].Tileset = DungeonTileset->BaseBMP;
            DungeonTileset->DoorsBottom[i].X = 815 + 17*i;
            DungeonTileset->DoorsBottom[i].Y = 110;
            DungeonTileset->DoorsBottom[i].Width = 32;
            DungeonTileset->DoorsBottom[i].Height = 32;
        }
}

internal void
LoadTextTileset(text_tileset *TextTileset, bmp_file *BaseBMP)
{
    TextTileset->BaseBMP = BaseBMP;

    uint32 Index = 0;
    // Upper 2 rows
    for (int32 Col = 0;
         Col < 16;
         Col++)
    {
        for (int32 Row = 0;
            Row < 2;
            Row++)
        {
            TextTileset->Tiles[Index].Tileset = TextTileset->BaseBMP;
            TextTileset->Tiles[Index].X = 1 + 8*Col;
            TextTileset->Tiles[Index].Y = 11 + 8*Row;
            TextTileset->Tiles[Index].Width = 8;
            TextTileset->Tiles[Index].Height = 8;
            Index++;
        }
    }
    // Lower 2 rows
    for (int32 Col = 0;
         Col < 8;
         Col++)
    {
        for (int32 Row = 2;
            Row < 4;
            Row++)
        {
            TextTileset->Tiles[Index].Tileset = TextTileset->BaseBMP;
            TextTileset->Tiles[Index].X = 1 + 8*Col;
            TextTileset->Tiles[Index].Y = 11 + 8*Row;
            TextTileset->Tiles[Index].Width = 8;
            TextTileset->Tiles[Index].Height = 8;
            Index++;
        }
    }
}

internal void
LoadHudTileset(hud_tileset *HudTileset, bmp_file *BaseBMP)
{
    HudTileset->BaseBMP = BaseBMP;

    HudTileset->Hearts[0].Tileset = BaseBMP;
    HudTileset->Hearts[0].X = 627;
    HudTileset->Hearts[0].Y = 117;
    HudTileset->Hearts[0].Width = 8;
    HudTileset->Hearts[0].Height = 8;

    HudTileset->Hearts[1].Tileset = BaseBMP;
    HudTileset->Hearts[1].X = 636;
    HudTileset->Hearts[1].Y = 117;
    HudTileset->Hearts[1].Width = 8;
    HudTileset->Hearts[1].Height = 8;

    HudTileset->Hearts[2].Tileset = BaseBMP;
    HudTileset->Hearts[2].X = 645;
    HudTileset->Hearts[2].Y = 117;
    HudTileset->Hearts[2].Width = 8;
    HudTileset->Hearts[2].Height = 8;
}

internal void
LoadOctorokSprites(octorok_sprites *OctorokSprites, bmp_file *BaseBMP)
{
    OctorokSprites->BaseBMP = BaseBMP;

    OctorokSprites->Front[0].Tileset = BaseBMP;
    OctorokSprites->Front[0].X = 1;
    OctorokSprites->Front[0].Y = 11;
    OctorokSprites->Front[0].Width = 16;
    OctorokSprites->Front[0].Height = 16;

    OctorokSprites->Front[1].Tileset = BaseBMP;
    OctorokSprites->Front[1].X = 18;
    OctorokSprites->Front[1].Y = 11;
    OctorokSprites->Front[1].Width = 16;
    OctorokSprites->Front[1].Height = 16;

    OctorokSprites->Back[0].Tileset = BaseBMP;
    OctorokSprites->Back[0].X = 1;
    OctorokSprites->Back[0].Y = 11;
    OctorokSprites->Back[0].Width = 16;
    OctorokSprites->Back[0].Height = 16;
    OctorokSprites->Back[0].FlipY = true;

    OctorokSprites->Back[1].Tileset = BaseBMP;
    OctorokSprites->Back[1].X = 18;
    OctorokSprites->Back[1].Y = 11;
    OctorokSprites->Back[1].Width = 16;
    OctorokSprites->Back[1].Height = 16;
    OctorokSprites->Back[1].FlipY = true;

    OctorokSprites->Left[0].Tileset = BaseBMP;
    OctorokSprites->Left[0].X = 35;
    OctorokSprites->Left[0].Y = 11;
    OctorokSprites->Left[0].Width = 16;
    OctorokSprites->Left[0].Height = 16;

    OctorokSprites->Left[1].Tileset = BaseBMP;
    OctorokSprites->Left[1].X = 52;
    OctorokSprites->Left[1].Y = 11;
    OctorokSprites->Left[1].Width = 16;
    OctorokSprites->Left[1].Height = 16;

    OctorokSprites->Right[0].Tileset = BaseBMP;
    OctorokSprites->Right[0].X = 35;
    OctorokSprites->Right[0].Y = 11;
    OctorokSprites->Right[0].Width = 16;
    OctorokSprites->Right[0].Height = 16;
    OctorokSprites->Right[0].FlipX = true;

    OctorokSprites->Right[1].Tileset = BaseBMP;
    OctorokSprites->Right[1].X = 52;
    OctorokSprites->Right[1].Y = 11;
    OctorokSprites->Right[1].Width = 16;
    OctorokSprites->Right[1].Height = 16;
    OctorokSprites->Right[1].FlipX = true;

    OctorokSprites->Projectile.Tileset = BaseBMP;
    OctorokSprites->Projectile.X = 69;
    OctorokSprites->Projectile.Y = 11;
    OctorokSprites->Projectile.Width = 8;
    OctorokSprites->Projectile.Height = 16;
}

internal void
LoadMoblinSprites(moblin_sprites *MoblinSprites, bmp_file *BaseBMP)
{
    MoblinSprites->BaseBMP = BaseBMP;

    MoblinSprites->Front[0].Tileset = BaseBMP;
    MoblinSprites->Front[0].X = 82;
    MoblinSprites->Front[0].Y = 11;
    MoblinSprites->Front[0].Width = 16;
    MoblinSprites->Front[0].Height = 16;

    MoblinSprites->Front[1].Tileset = BaseBMP;
    MoblinSprites->Front[1].X = 82;
    MoblinSprites->Front[1].Y = 11;
    MoblinSprites->Front[1].Width = 16;
    MoblinSprites->Front[1].Height = 16;
    MoblinSprites->Front[1].FlipX = true;

    MoblinSprites->Back[0].Tileset = BaseBMP;
    MoblinSprites->Back[0].X = 99;
    MoblinSprites->Back[0].Y = 11;
    MoblinSprites->Back[0].Width = 16;
    MoblinSprites->Back[0].Height = 16;

    MoblinSprites->Back[1].Tileset = BaseBMP;
    MoblinSprites->Back[1].X = 99;
    MoblinSprites->Back[1].Y = 11;
    MoblinSprites->Back[1].Width = 16;
    MoblinSprites->Back[1].Height = 16;
    MoblinSprites->Back[1].FlipX = true;

    MoblinSprites->Left[0].Tileset = BaseBMP;
    MoblinSprites->Left[0].X = 116;
    MoblinSprites->Left[0].Y = 11;
    MoblinSprites->Left[0].Width = 16;
    MoblinSprites->Left[0].Height = 16;
    MoblinSprites->Left[0].FlipX = true;

    MoblinSprites->Left[1].Tileset = BaseBMP;
    MoblinSprites->Left[1].X = 133;
    MoblinSprites->Left[1].Y = 11;
    MoblinSprites->Left[1].Width = 16;
    MoblinSprites->Left[1].Height = 16;
    MoblinSprites->Left[0].FlipX = true;

    MoblinSprites->Right[0].Tileset = BaseBMP;
    MoblinSprites->Right[0].X = 116;
    MoblinSprites->Right[0].Y = 11;
    MoblinSprites->Right[0].Width = 16;
    MoblinSprites->Right[0].Height = 16;

    MoblinSprites->Right[1].Tileset = BaseBMP;
    MoblinSprites->Right[1].X = 133;
    MoblinSprites->Right[1].Y = 11;
    MoblinSprites->Right[1].Width = 16;
    MoblinSprites->Right[1].Height = 16;

    MoblinSprites->ArrowFront.Tileset = BaseBMP;
    MoblinSprites->ArrowFront.X = 150;
    MoblinSprites->ArrowFront.Y = 11;
    MoblinSprites->ArrowFront.Width = 8;
    MoblinSprites->ArrowFront.Height = 16;
    MoblinSprites->ArrowFront.FlipY = true;

    MoblinSprites->ArrowBack.Tileset = BaseBMP;
    MoblinSprites->ArrowBack.X = 150;
    MoblinSprites->ArrowBack.Y = 11;
    MoblinSprites->ArrowBack.Width = 8;
    MoblinSprites->ArrowBack.Height = 16;

    MoblinSprites->ArrowLeft.Tileset = BaseBMP;
    MoblinSprites->ArrowLeft.X = 159;
    MoblinSprites->ArrowLeft.Y = 11;
    MoblinSprites->ArrowLeft.Width = 16;
    MoblinSprites->ArrowLeft.Height = 16;
    MoblinSprites->ArrowLeft.FlipX = true;

    MoblinSprites->ArrowRight.Tileset = BaseBMP;
    MoblinSprites->ArrowRight.X = 159;
    MoblinSprites->ArrowRight.Y = 11;
    MoblinSprites->ArrowRight.Width = 16;
    MoblinSprites->ArrowRight.Height = 16;
}

internal void
LoadLinkSprites(link_sprites *LinkSprites, bmp_file *BaseBMP)
{
    LinkSprites->BaseBMP = BaseBMP;

    LinkSprites->Front[0].Tileset = LinkSprites->BaseBMP;
    LinkSprites->Front[0].X = 1;
    LinkSprites->Front[0].Y = 11;
    LinkSprites->Front[0].Width = 16;
    LinkSprites->Front[0].Height = 16;

    LinkSprites->Front[1].Tileset = LinkSprites->BaseBMP;
    LinkSprites->Front[1].X = 18;
    LinkSprites->Front[1].Y = 11;
    LinkSprites->Front[1].Width = 16;
    LinkSprites->Front[1].Height = 16;

    LinkSprites->Right[0].Tileset = LinkSprites->BaseBMP;
    LinkSprites->Right[0].X = 35;
    LinkSprites->Right[0].Y = 11;
    LinkSprites->Right[0].Width = 16;
    LinkSprites->Right[0].Height = 16;

    LinkSprites->Right[1].Tileset = LinkSprites->BaseBMP;
    LinkSprites->Right[1].X = 52;
    LinkSprites->Right[1].Y = 11;
    LinkSprites->Right[1].Width = 16;
    LinkSprites->Right[1].Height = 16;

    LinkSprites->Back[0].Tileset = LinkSprites->BaseBMP;
    LinkSprites->Back[0].X = 69;
    LinkSprites->Back[0].Y = 11;
    LinkSprites->Back[0].Width = 16;
    LinkSprites->Back[0].Height = 16;

    LinkSprites->Back[1].Tileset = LinkSprites->BaseBMP;
    LinkSprites->Back[1].X = 86;
    LinkSprites->Back[1].Y = 11;
    LinkSprites->Back[1].Width = 16;
    LinkSprites->Back[1].Height = 16;

    LinkSprites->Left[0].Tileset = LinkSprites->BaseBMP;
    LinkSprites->Left[0].X = 35;
    LinkSprites->Left[0].Y = 11;
    LinkSprites->Left[0].Width = 16;
    LinkSprites->Left[0].Height = 16;
    LinkSprites->Left[0].FlipX = true;

    LinkSprites->Left[1].Tileset = LinkSprites->BaseBMP;
    LinkSprites->Left[1].X = 52;
    LinkSprites->Left[1].Y = 11;
    LinkSprites->Left[1].Width = 16;
    LinkSprites->Left[1].Height = 16;
    LinkSprites->Left[1].FlipX = true;

    LinkSprites->UseSwordFront[0].Tileset = LinkSprites->BaseBMP;
    LinkSprites->UseSwordFront[0].X = 1;
    LinkSprites->UseSwordFront[0].Y = 47;
    LinkSprites->UseSwordFront[0].Width = 16;
    LinkSprites->UseSwordFront[0].Height = 27;

    LinkSprites->UseSwordFront[1].Tileset = LinkSprites->BaseBMP;
    LinkSprites->UseSwordFront[1].X = 18;
    LinkSprites->UseSwordFront[1].Y = 47;
    LinkSprites->UseSwordFront[1].Width = 16;
    LinkSprites->UseSwordFront[1].Height = 27;

    LinkSprites->UseSwordFront[2].Tileset = LinkSprites->BaseBMP;
    LinkSprites->UseSwordFront[2].X = 35;
    LinkSprites->UseSwordFront[2].Y = 47;
    LinkSprites->UseSwordFront[2].Width = 16;
    LinkSprites->UseSwordFront[2].Height = 27;

    LinkSprites->UseSwordFront[3].Tileset = LinkSprites->BaseBMP;
    LinkSprites->UseSwordFront[3].X = 52;
    LinkSprites->UseSwordFront[3].Y = 47;
    LinkSprites->UseSwordFront[3].Width = 16;
    LinkSprites->UseSwordFront[3].Height = 27;

    LinkSprites->UseSwordRight[0].Tileset = LinkSprites->BaseBMP;
    LinkSprites->UseSwordRight[0].X = 1;
    LinkSprites->UseSwordRight[0].Y = 77;
    LinkSprites->UseSwordRight[0].Width = 16;
    LinkSprites->UseSwordRight[0].Height = 16;

    LinkSprites->UseSwordRight[1].Tileset = LinkSprites->BaseBMP;
    LinkSprites->UseSwordRight[1].X = 18;
    LinkSprites->UseSwordRight[1].Y = 77;
    LinkSprites->UseSwordRight[1].Width = 27;
    LinkSprites->UseSwordRight[1].Height = 16;

    LinkSprites->UseSwordRight[2].Tileset = LinkSprites->BaseBMP;
    LinkSprites->UseSwordRight[2].X = 46;
    LinkSprites->UseSwordRight[2].Y = 77;
    LinkSprites->UseSwordRight[2].Width = 70 - 46;
    LinkSprites->UseSwordRight[2].Height = 16;

    LinkSprites->UseSwordRight[3].Tileset = LinkSprites->BaseBMP;
    LinkSprites->UseSwordRight[3].X = 70;
    LinkSprites->UseSwordRight[3].Y = 77;
    LinkSprites->UseSwordRight[3].Width = 89 - 70;
    LinkSprites->UseSwordRight[3].Height = 16;

    LinkSprites->UseSwordBack[0].Tileset = LinkSprites->BaseBMP;
    LinkSprites->UseSwordBack[0].X = 1;
    LinkSprites->UseSwordBack[0].Y = 97;
    LinkSprites->UseSwordBack[0].AnchorX = 1;
    LinkSprites->UseSwordBack[0].AnchorY = 109;
    LinkSprites->UseSwordBack[0].Width = 16;
    LinkSprites->UseSwordBack[0].Height = 27;

    LinkSprites->UseSwordBack[1].Tileset = LinkSprites->BaseBMP;
    LinkSprites->UseSwordBack[1].X = 18;
    LinkSprites->UseSwordBack[1].Y = 97;
    LinkSprites->UseSwordBack[1].AnchorX = 18;
    LinkSprites->UseSwordBack[1].AnchorY = 109;
    LinkSprites->UseSwordBack[1].Width = 16;
    LinkSprites->UseSwordBack[1].Height = 27;

    LinkSprites->UseSwordBack[2].Tileset = LinkSprites->BaseBMP;
    LinkSprites->UseSwordBack[2].X = 35;
    LinkSprites->UseSwordBack[2].Y = 97;
    LinkSprites->UseSwordBack[2].AnchorX = 35;
    LinkSprites->UseSwordBack[2].AnchorY = 109;
    LinkSprites->UseSwordBack[2].Width = 16;
    LinkSprites->UseSwordBack[2].Height = 27;

    LinkSprites->UseSwordBack[3].Tileset = LinkSprites->BaseBMP;
    LinkSprites->UseSwordBack[3].X = 52;
    LinkSprites->UseSwordBack[3].Y = 97;
    LinkSprites->UseSwordBack[3].AnchorX = 52;
    LinkSprites->UseSwordBack[3].AnchorY = 109;
    LinkSprites->UseSwordBack[3].Width = 16;
    LinkSprites->UseSwordBack[3].Height = 27;

    LinkSprites->UseSwordLeft[0].Tileset = LinkSprites->BaseBMP;
    LinkSprites->UseSwordLeft[0].X = 1;
    LinkSprites->UseSwordLeft[0].Y = 77;
    LinkSprites->UseSwordLeft[0].Width = 16;
    LinkSprites->UseSwordLeft[0].Height = 16;
    LinkSprites->UseSwordLeft[0].AnchorX = 17;
    LinkSprites->UseSwordLeft[0].AnchorY = 77;
    LinkSprites->UseSwordLeft[0].FlipX = true;

    LinkSprites->UseSwordLeft[1].Tileset = LinkSprites->BaseBMP;
    LinkSprites->UseSwordLeft[1].X = 18;
    LinkSprites->UseSwordLeft[1].Y = 77;
    LinkSprites->UseSwordLeft[1].Width = 27;
    LinkSprites->UseSwordLeft[1].Height = 16;
    LinkSprites->UseSwordLeft[1].AnchorX = 34;
    LinkSprites->UseSwordLeft[1].AnchorY = 77;
    LinkSprites->UseSwordLeft[1].FlipX = true;

    LinkSprites->UseSwordLeft[2].Tileset = LinkSprites->BaseBMP;
    LinkSprites->UseSwordLeft[2].X = 46;
    LinkSprites->UseSwordLeft[2].Y = 77;
    LinkSprites->UseSwordLeft[2].Width = 70 - 46;
    LinkSprites->UseSwordLeft[2].Height = 16;
    LinkSprites->UseSwordLeft[2].AnchorX = 62;
    LinkSprites->UseSwordLeft[2].AnchorY = 77;
    LinkSprites->UseSwordLeft[2].FlipX = true;

    LinkSprites->UseSwordLeft[3].Tileset = LinkSprites->BaseBMP;
    LinkSprites->UseSwordLeft[3].X = 70;
    LinkSprites->UseSwordLeft[3].Y = 77;
    LinkSprites->UseSwordLeft[3].Width = 89 - 70;
    LinkSprites->UseSwordLeft[3].Height = 16;
    LinkSprites->UseSwordLeft[3].AnchorX = 86;
    LinkSprites->UseSwordLeft[3].AnchorY = 77;
    LinkSprites->UseSwordLeft[3].FlipX = true;

    LinkSprites->PickUp[0].Tileset = LinkSprites->BaseBMP;
    LinkSprites->PickUp[0].X = 213;
    LinkSprites->PickUp[0].Y = 11;
    LinkSprites->PickUp[0].Width = 16;
    LinkSprites->PickUp[0].Height = 16;

    LinkSprites->PickUp[1].Tileset = LinkSprites->BaseBMP;
    LinkSprites->PickUp[1].X = 230;
    LinkSprites->PickUp[1].Y = 11;
    LinkSprites->PickUp[1].Width = 16;
    LinkSprites->PickUp[1].Height = 16;

    LinkSprites->Sword[0].Tileset = LinkSprites->BaseBMP;
    LinkSprites->Sword[0].X = 1;
    LinkSprites->Sword[0].Y = 154;
    LinkSprites->Sword[0].Width = 8;
    LinkSprites->Sword[0].Height = 16;

    LinkSprites->Sword[1].Tileset = LinkSprites->BaseBMP;
    LinkSprites->Sword[1].X = 10;
    LinkSprites->Sword[1].Y = 154;
    LinkSprites->Sword[1].Width = 16;
    LinkSprites->Sword[1].Height = 16;

    LinkSprites->Sword[2].Tileset = LinkSprites->BaseBMP;
    LinkSprites->Sword[2].X = 27;
    LinkSprites->Sword[2].Y = 154;
    LinkSprites->Sword[2].Width = 8;
    LinkSprites->Sword[2].Height = 16;
}

internal void
LoadBoomerangSprites(boomerang_sprites *BoomerangSprites, bmp_file *BaseBMP)
{
    BoomerangSprites->BaseBMP = BaseBMP;

    BoomerangSprites->Sprites[0].Tileset = BoomerangSprites->BaseBMP;
    BoomerangSprites->Sprites[0].X = 64;
    BoomerangSprites->Sprites[0].Y = 185;
    BoomerangSprites->Sprites[0].Width = 8;
    BoomerangSprites->Sprites[0].Height = 16;

    BoomerangSprites->Sprites[1].Tileset = BoomerangSprites->BaseBMP;
    BoomerangSprites->Sprites[1].X = 73;
    BoomerangSprites->Sprites[1].Y = 185;
    BoomerangSprites->Sprites[1].Width = 8;
    BoomerangSprites->Sprites[1].Height = 16;

    BoomerangSprites->Sprites[2].Tileset = BoomerangSprites->BaseBMP;
    BoomerangSprites->Sprites[2].X = 82;
    BoomerangSprites->Sprites[2].Y = 185;
    BoomerangSprites->Sprites[2].Width = 8;
    BoomerangSprites->Sprites[2].Height = 16;

    BoomerangSprites->Sprites[3].Tileset = BoomerangSprites->BaseBMP;
    BoomerangSprites->Sprites[3].X = 73;
    BoomerangSprites->Sprites[3].Y = 185;
    BoomerangSprites->Sprites[3].Width = 8;
    BoomerangSprites->Sprites[3].Height = 16;
    BoomerangSprites->Sprites[3].FlipX = true;

    BoomerangSprites->Sprites[4].Tileset = BoomerangSprites->BaseBMP;
    BoomerangSprites->Sprites[4].X = 64;
    BoomerangSprites->Sprites[4].Y = 185;
    BoomerangSprites->Sprites[4].Width = 8;
    BoomerangSprites->Sprites[4].Height = 16;
    BoomerangSprites->Sprites[4].FlipX = true;

    BoomerangSprites->Sprites[5].Tileset = BoomerangSprites->BaseBMP;
    BoomerangSprites->Sprites[5].X = 73;
    BoomerangSprites->Sprites[5].Y = 185;
    BoomerangSprites->Sprites[5].Width = 8;
    BoomerangSprites->Sprites[5].Height = 16;
    BoomerangSprites->Sprites[5].FlipX = true;
    BoomerangSprites->Sprites[5].FlipY = true;

    BoomerangSprites->Sprites[6].Tileset = BoomerangSprites->BaseBMP;
    BoomerangSprites->Sprites[6].X = 82;
    BoomerangSprites->Sprites[6].Y = 185;
    BoomerangSprites->Sprites[6].Width = 8;
    BoomerangSprites->Sprites[6].Height = 16;
    BoomerangSprites->Sprites[6].FlipY = true;

    BoomerangSprites->Sprites[7].Tileset = BoomerangSprites->BaseBMP;
    BoomerangSprites->Sprites[7].X = 73;
    BoomerangSprites->Sprites[7].Y = 185;
    BoomerangSprites->Sprites[7].Width = 8;
    BoomerangSprites->Sprites[7].Height = 16;
    BoomerangSprites->Sprites[7].FlipY = true;
}

internal void
LoadNPCSprites(npc_sprites *NPCSprites, bmp_file *BaseBMP)
{
    NPCSprites->BaseBMP = BaseBMP;

    NPCSprites->OldMan[0].Tileset = NPCSprites->BaseBMP;
    NPCSprites->OldMan[0].X = 1;
    NPCSprites->OldMan[0].Y = 11;
    NPCSprites->OldMan[0].Width = 16;
    NPCSprites->OldMan[0].Height = 16;

    NPCSprites->OldMan[1].Tileset = NPCSprites->BaseBMP;
    NPCSprites->OldMan[1].X = 18;
    NPCSprites->OldMan[1].Y = 11;
    NPCSprites->OldMan[1].Width = 16;
    NPCSprites->OldMan[1].Height = 16;

    NPCSprites->Fire[0].Tileset = NPCSprites->BaseBMP;
    NPCSprites->Fire[0].X = 52;
    NPCSprites->Fire[0].Y = 11;
    NPCSprites->Fire[0].Width = 16;
    NPCSprites->Fire[0].Height = 16;

    NPCSprites->Fire[1].Tileset = NPCSprites->BaseBMP;
    NPCSprites->Fire[1].X = 69;
    NPCSprites->Fire[1].Y = 11;
    NPCSprites->Fire[1].Width = 16;
    NPCSprites->Fire[1].Height = 16;
}

internal int32
StringLength(uint8 *String)
{
    int32 Count = 0;
    while (*String++)
    {
        Count++;
    }
    return Count;
}

internal void
DrawString(game_offscreen_buffer *Buffer, 
           text_tileset *TextTileset, uint8 *String, 
           real32 X, real32 Y)
{
    for (int32 StringPos = 0;
         StringPos < StringLength(String);
         StringPos++)
    {
        uint32 CharIndex;
        uint8 CharValue = String[StringPos];
        // This is 0-9
        if (CharValue >= 48 && CharValue < 58)
        {
            CharIndex = CharValue - 48;
        }
        // A-Z
        else if (CharValue >= 65 && CharValue < 65 + 26)
        {
            CharIndex = CharValue - 55;
        }
        else if (CharValue == ',')
        {
            CharIndex = 40;
        }
        else if (CharValue == '!')
        {
            CharIndex = 41;
        }
        else if (CharValue == '\'')
        {
            CharIndex = 42;
        }
        else if (CharValue == '&')
        {
            CharIndex = 43;
        }
        else if (CharValue == '.')
        {
            CharIndex = 44;
        }
        else if (CharValue == '"')
        {
            CharIndex = 45;
        }
        else if (CharValue == '?')
        {
            CharIndex = 46;
        }
        else if (CharValue == '-')
        {
            CharIndex = 47;
        }
        else if (CharValue == ' ')
        {
            CharIndex = 36;
        }
        else
        {
            CharIndex = 0;
            Assert(0);
        }

        DrawBMPTile(&TextTileset->Tiles[CharIndex], Buffer, 
                    X + (real32)StringPos*8.0f, Y);
    }
}

internal void
DrawStringPartial(game_offscreen_buffer *Buffer, 
                 text_tileset *TextTileset, uint8 *String, 
                 int32 MaxChars, real32 X, real32 Y)
{
    int32 StringLen = StringLength(String);
    int32 CharsToDraw = MaxChars < StringLen ? MaxChars : StringLen;
    
    // Create temporary null-terminated string with only the characters we want to draw
    uint8 TempString[256];  // Should be enough for any text we'll display
    for (int32 i = 0; i < CharsToDraw; i++)
    {
        TempString[i] = String[i];
    }
    TempString[CharsToDraw] = 0;  // Null terminator
    
    // Use DrawString to do the actual drawing
    DrawString(Buffer, TextTileset, TempString, X, Y);
}
