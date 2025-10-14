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
                    *MemoryPixels++ = Pixel;
                }
            }
        }
        else if (BMPFile.BitsPerPixel == 24)
        {
            uint32 PixelCount = BMPFile.Width*BMPFile.Height;
            uint32 *MemoryPixels = PushArray(Arena, PixelCount, uint32);
            BMPFile.Pixels = MemoryPixels;
            uint8 *CopyPixel;
            // NOTE: It goes top to bottom, i wanna reverse it so the image pixels always starts at the top left corner
            for (uint32 RowIdx = BMPFile.Height; 
                RowIdx > 0; 
                RowIdx--)
            {
                CopyPixel = (uint8 *)FilePixels + BMPFile.Width*(RowIdx-1)*3;
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
                    *MemoryPixels++ = Pixel;
                }
            }
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

            uint32 SourceX = BMPTile->X + ColIdx;
            uint32 SourceY = BMPTile->Y + RowIdx;

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
