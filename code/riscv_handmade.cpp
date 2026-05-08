#include "handmade.h"

#include "riscv_handmade.h"

// TODO: Is this the right way to do things if I wanna statically link?
extern "C" game_update_and_render GameUpdateAndRender;


// TODO: this is a global for now
global_variable bool32 GlobalRunning;
global_variable bool32 GlobalPause;
global_variable win32_offscreen_buffer GlobalBackbuffer;
global_variable win32_window_transform GlobalWindowXform;
global_variable int64 GlobalPerfCountFrequency;


extern "C" void *memcpy(void *dest, const void *src, size_t n)
{
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;

    for (size_t i = 0; i < n; ++i)
    {
        d[i] = s[i];
    }

    return dest;
}

extern "C" void *memset(void *dest, int c, size_t n)
{
    unsigned char *d = (unsigned char *)dest;
    unsigned char byte = (unsigned char)c;

    for (size_t i = 0; i < n; ++i)
    {
        d[i] = byte;
    }

    return dest;
}


DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeFileMemory)
{
    // TODO: Map this to MMIO
}

typedef struct 
{
	// input
	char *filename;
	char *buffer;
	uint32_t buffer_size;
	// output
	uint32_t *output_size;
} MMIOReadFileInput;

static inline void mmio_read_file(char *filename, char *buffer, uint32_t buffer_size, uint32_t *output_size)
{
    MMIOReadFileInput mmio_input = {0};
    mmio_input.filename = filename;
    mmio_input.buffer = buffer;
    mmio_input.buffer_size = buffer_size;
    mmio_input.output_size = output_size;
    *(volatile unsigned int *)0x10000010 = (unsigned int)&mmio_input;
    return;
}

DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile)
{
    mmio_read_file(Filename, Buffer, BufferSize, OutputSize);
    return;
}

static inline void putc_uart(char c)
{
    *(volatile unsigned int *)0x10000000 = (unsigned int)c;
}

DEBUG_PLATFORM_LOG(DEBUGPlatformLog)
{
    while (*CString != 0)
    {
        putc_uart(*CString);
        CString++;
    }
}

extern "C" int 
main(void)
{
    game_offscreen_buffer VideoBuffer = {};
    VideoBuffer.Memory = (uint8*)(0x80000000 + Megabytes(16));
    VideoBuffer.Width = 256;
    VideoBuffer.Height = 240;
    VideoBuffer.BytesPerPixel = 4;
    VideoBuffer.Pitch = VideoBuffer.BytesPerPixel*VideoBuffer.Width;

    game_memory GameMemory = {};
    GameMemory.PermanentStorageSize = Megabytes(64);
    GameMemory.TransientStorageSize = Megabytes(512);
    GameMemory.DEBUGPlatformReadEntireFile = DEBUGPlatformReadEntireFile;
    GameMemory.DEBUGPlatformFreeFileMemory = DEBUGPlatformFreeFileMemory;
    GameMemory.DEBUGPlatformLog = DEBUGPlatformLog;
    GameMemory.GameUpdateHz = 30;

    // TODO: Where is the ram block located on the risc v machine?
    uint8 *ram_block = 0x80000000 + (uint8*)(Megabytes(64));
    GameMemory.PermanentStorage = ram_block;
    GameMemory.TransientStorage = (ram_block + GameMemory.PermanentStorageSize);

    // Game code not a DLL, how to load it in?
    //   - Statically link
    //   - Put it at a known address and load it here
    // Static link
    //   - compile game to game.o, platform layer to platform.o, link them together

    thread_context Thread = {};

    // TODO: MMIO Interrupt for stopping the program
    bool32 IsRunning = 1;
    while (IsRunning)
    {
        // TODO: Input MMIO stuff
        game_input NewInput = {};

        DEBUGPlatformLog("Test");

        // TODO: How to know how far into this function we are?
        GameUpdateAndRender(&Thread, &GameMemory, &NewInput, &VideoBuffer);
    }

    DEBUGPlatformLog("Rip");

    return 0;
}
