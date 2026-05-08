#ifndef WIN32_HANDMADE_H
#define WIN32_HANDMADE_H


struct win32_offscreen_buffer
{
    void* Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
};


struct win32_window_transform
{
    int OffsetX;
    int OffsetY;
    real32 BufferScale;
};


struct win32_window_dimension 
{
    int Width;
    int Height;
};


#endif
