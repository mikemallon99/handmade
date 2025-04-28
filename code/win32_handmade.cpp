// Oh shit, Windows platform layer time

#include "handmade.h"

// NOTE: include windows code below handmade.cpp so it doesnt interfere with game code
#include <windows.h>
#include <stdio.h>
#include <xinput.h>
#include <dsound.h>

#include "win32_handmade.h"


// TODO: this is a global for now
global_variable bool32 GlobalRunning;
global_variable bool32 GlobalPause;
global_variable win32_offscreen_buffer GlobalBackbuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;
global_variable int64 GlobalPerfCountFrequency;


// Support for XInputGetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub) 
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_


// Support for XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);


DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeFileMemory)
{
    if (Memory)
    {
        VirtualFree(Memory, 0, MEM_RELEASE);
    }
}


DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile)
{
    debug_read_file_result Result = {};

    HANDLE FileHandle = CreateFileA(Filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (FileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER FileSize;
        if (GetFileSizeEx(FileHandle, &FileSize))
        {
            uint32 FileSize32 = SafeTruncateUInt64(FileSize.QuadPart);
            Result.Contents = VirtualAlloc(0, FileSize32, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            if (Result.Contents)
            {
                DWORD BytesRead = 0;
                if (ReadFile(FileHandle, Result.Contents, FileSize32, &BytesRead, 0) && 
                    (FileSize32 == BytesRead))
                {
                    // NOTE: file read successfully
                    Result.ContentsSize = FileSize32;
                }
                else
                {
                    DEBUGPlatformFreeFileMemory(Thread, Result.Contents);
                    Result.Contents = 0;
                }
            }
            else
            {
                // TODO: Logging
            }
        }
        else
        {
            // TODO: Logging
        }

        CloseHandle(FileHandle);
    }
    else
    {
        // TODO: Logging
    }

    return Result;
}


DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUGPlatformWriteEntireFile)
{
    bool32 Result = 0;

    HANDLE FileHandle = CreateFileA(Filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if (FileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD BytesWritten = 0;
        if (WriteFile(FileHandle, Memory, MemorySize, &BytesWritten, 0))
        {
            // NOTE: file read successfully
            Result = (MemorySize == BytesWritten);
        }
        else
        {
            // TODO: Logging
        }

        CloseHandle(FileHandle);
    }
    else
    {
        // TODO: Logging
    }

    return Result;
}


internal void
Win32GetEXEFileName(win32_state *State)
{
    DWORD SizeOfFilename = GetModuleFileNameA(0, State->EXEFileName, sizeof(State->EXEFileName));
    State->OnePastLastEXEFileNameSlash = State->EXEFileName;
    for (char *Scan = State->EXEFileName;
        *Scan;
        Scan++)
    {
        if (*Scan == '\\')
        {
            State->OnePastLastEXEFileNameSlash = Scan + 1;
        }
    }
}

internal int
StringLength(char *String)
{
    int Count = 0;
    while (*String++)
    {
        Count++;
    }
    return Count;
}

void
CatStrings(uint64 SourceACount, char *SourceA,
           uint64 SourceBCount, char *SourceB,
           uint64 DestCount, char *Dest)
{
    for (int Index = 0;
        Index < SourceACount;
        Index++)
    {
        *Dest++ = SourceA[Index];
    }
    for (int Index = 0;
        Index < SourceBCount;
        Index++)
    {
        *Dest++ = SourceB[Index];
    }

    *Dest++ = 0;
}

internal void
Win32BuildEXEPathFileName(win32_state *State, char *FileName,
                          int DestCount, char *Dest)
{
    CatStrings(State->OnePastLastEXEFileNameSlash - State->EXEFileName, State->EXEFileName, 
               StringLength(FileName), FileName,
               DestCount, Dest);
}
internal void
Win32GetInputFileLocation(win32_state *State, bool32 InputStream, 
                          int SlotIndex, int DestCount, char *Dest)
{
    char Name[64];
    wsprintf(Name, "loop_edit_%d_%s.hmi", SlotIndex,
                 InputStream ? "input" : "state");
    Win32BuildEXEPathFileName(State, Name, DestCount, Dest);
}

internal win32_replay_buffer *
Win32GetReplayBuffer(win32_state *State, int ReplayIndex)
{
    Assert(ReplayIndex < ArrayCount(State->ReplayBuffers));
    return &State->ReplayBuffers[ReplayIndex];
}

internal void
Win32BeginRecordingInput(win32_state *State, int InputRecordingIndex)
{
    win32_replay_buffer *ReplayBuffer = Win32GetReplayBuffer(State, InputRecordingIndex);
    if (ReplayBuffer->MemoryBlock)
    {
        State->InputRecordingIndex = InputRecordingIndex;
        char Filename[WIN32_STATE_FILE_NAME_COUNT];
        Win32GetInputFileLocation(State, true, InputRecordingIndex, sizeof(Filename), Filename);
        State->RecordingHandle = CreateFileA(Filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);

#if 0
        LARGE_INTEGER FilePosition;
        FilePosition.QuadPart = State->TotalSize;
        SetFilePointerEx(State->RecordingHandle, FilePosition, 0, FILE_BEGIN);
#endif
        CopyMemory(ReplayBuffer->MemoryBlock, State->GameMemoryBlock, (size_t)State->TotalSize);
    }
}

internal void
Win32EndRecordingInput(win32_state *State)
{
    CloseHandle(State->RecordingHandle);
    State->InputRecordingIndex = 0;
}

internal void
Win32BeginInputPlayBack(win32_state *State, int InputPlayingIndex)
{
    win32_replay_buffer *ReplayBuffer = Win32GetReplayBuffer(State, InputPlayingIndex);
    if (ReplayBuffer->MemoryBlock)
    {
        State->InputPlayingIndex = InputPlayingIndex;

        char Filename[WIN32_STATE_FILE_NAME_COUNT];
        Win32GetInputFileLocation(State, true, InputPlayingIndex, sizeof(Filename), Filename);
        State->PlaybackHandle = CreateFileA(Filename, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);

#if 0
        LARGE_INTEGER FilePosition;
        FilePosition.QuadPart = State->TotalSize;
        SetFilePointerEx(State->PlaybackHandle, FilePosition, 0, FILE_BEGIN);
#endif
        CopyMemory(State->GameMemoryBlock, ReplayBuffer->MemoryBlock, (size_t)State->TotalSize);
    }
}

internal void
Win32EndInputPlayBack(win32_state *State)
{
    CloseHandle(State->PlaybackHandle);
    State->InputPlayingIndex = 0;
}

internal void
Win32RecordInput(win32_state *State, game_input *NewInput)
{
    DWORD BytesWritten;
    WriteFile(State->RecordingHandle, NewInput, sizeof(*NewInput), &BytesWritten, 0);
}

internal void
Win32PlayBackInput(win32_state *State, game_input *NewInput)
{
    DWORD BytesRead;
    ReadFile(State->PlaybackHandle, NewInput, sizeof(*NewInput), &BytesRead, 0);
    if(BytesRead)
    {
        // NOTE: Theres still input left
    }
    else
    {
        // NOTE: end of stream, go back to beginning
        int PlayingIndex = State->InputPlayingIndex;
        Win32EndInputPlayBack(State);
        Win32BeginInputPlayBack(State, PlayingIndex);
    }
}

inline FILETIME
GetFileLastWriteTime(char *Filename)
{
    FILETIME LastWriteTime = {};

    WIN32_FILE_ATTRIBUTE_DATA Data;
    if (GetFileAttributesEx(Filename, GetFileExInfoStandard, &Data))
    {
        LastWriteTime = Data.ftLastWriteTime;
    }

    return LastWriteTime;
}

internal win32_game_code
Win32LoadGameCode(char *SourceFilename, char *TempFilename)
{
    win32_game_code Result = {};

    Result.LastDLLWriteTime = GetFileLastWriteTime(SourceFilename);
    CopyFile(SourceFilename, TempFilename, FALSE);
    
    Result.GameCodeDLL = LoadLibraryA(TempFilename);
    if (Result.GameCodeDLL)
    {
        Result.UpdateAndRender = (game_update_and_render *)GetProcAddress(Result.GameCodeDLL, 
                                                                          "GameUpdateAndRender");
        Result.GetSoundSamples = (game_get_sound_samples *)GetProcAddress(Result.GameCodeDLL, 
                                                                          "GameGetSoundSamples");

        Result.IsValid = (Result.UpdateAndRender && Result.GetSoundSamples);
    }

    if (!Result.IsValid)
    {
        Result.UpdateAndRender = 0;
        Result.GetSoundSamples = 0;
    }

    return Result;
}


internal void
Win32UnloadGameCode(win32_game_code *GameCode)
{
    if (GameCode->IsValid)
    {
        FreeLibrary(GameCode->GameCodeDLL);
    }
    GameCode->IsValid = false;
    GameCode->GameCodeDLL = 0;
    GameCode->UpdateAndRender = 0;
    GameCode->GetSoundSamples = 0;
}


internal void
Win32LoadXInput(void)
{
    // TODO: test this on windows 8
    HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
    if (!XInputLibrary)
    {
        XInputLibrary = LoadLibraryA("xinput1_3.dll");
    }

    if (XInputLibrary) 
    {
        XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
        XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
    }
}


internal void
Win32InitDSound(HWND Window, int32 SamplesPerSecond, int BufferSize)
{
    // NOTE: load the library
    HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");

    if (DSoundLibrary)
    {
        // NOTE: get a directsound object
        direct_sound_create *DirectSoundCreate = (direct_sound_create *)
            GetProcAddress(DSoundLibrary, "DirectSoundCreate");

        // TODO: double check that this works on XP - directsound8 or 7?
        LPDIRECTSOUND DirectSound;
        if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
        {
            WAVEFORMATEX WaveFormat = {};
            WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
            WaveFormat.nChannels = 2;
            WaveFormat.nSamplesPerSec = SamplesPerSecond;
            WaveFormat.wBitsPerSample = 16;
            WaveFormat.nBlockAlign = (WaveFormat.nChannels*WaveFormat.wBitsPerSample) / 8;
            WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec*WaveFormat.nBlockAlign;
            WaveFormat.cbSize = 0;

            // NOTE: create a primary buffer
            if (SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
            {
                DSBUFFERDESC BufferDescription = {};
                BufferDescription.dwSize = sizeof(BufferDescription);
                BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

                LPDIRECTSOUNDBUFFER PrimaryBuffer;
                if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0)))
                {
                    HRESULT Error = PrimaryBuffer->SetFormat(&WaveFormat);
                    if (SUCCEEDED(Error))
                    {
                        // NOTE: We have set the format
                        OutputDebugStringA("Primary buffer format was set\n");
                    }
                    else
                    {
                        // TODO: diagnostic
                    }
                }
                else 
                {
                }
            }
            else
            {
                // TODO: diagnostic
            }

            // NOTE: create a secondary buffer
            DSBUFFERDESC BufferDescription = {};
            BufferDescription.dwSize = sizeof(BufferDescription);
            BufferDescription.dwFlags = 0;
            BufferDescription.dwBufferBytes = BufferSize;
            BufferDescription.lpwfxFormat = &WaveFormat;
            HRESULT Error = DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0);
            if (SUCCEEDED(Error))
            {
                OutputDebugStringA("Secondary buffer was created\n");
            }
            else 
            {
                // TODO: diagnostic
            }

            // NOTE: start it playing
        }
        else
        {
            // TODO: diagnostic
        }
    }
    else 
    {
        // TODO: diagnostic
    }
}


win32_window_dimension
Win32GetWindowDimension(HWND Window)
{
    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    win32_window_dimension Dimension;
    Dimension.Width = ClientRect.right - ClientRect.left;
    Dimension.Height = ClientRect.bottom - ClientRect.top;
    return Dimension;
}


internal void 
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height) 
{
    // TODO: bulletproof this
    // Dont free first, free after, then free first if that fails
    
    if (Buffer->Memory) {
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }

    Buffer->Width = Width;
    Buffer->Height = Height;

    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;

    Buffer->BytesPerPixel = 4;
    int MemorySize = Buffer->BytesPerPixel * (Buffer->Width * Buffer->Height);
    Buffer->Memory = VirtualAlloc(0, MemorySize, MEM_COMMIT, PAGE_READWRITE);

    Buffer->Pitch = Buffer->Width*Buffer->BytesPerPixel;
}


internal void
Win32DebugDrawVertical(win32_offscreen_buffer *Backbuffer, 
                       int X, int Top, int Bottom, uint32 Color)
{
    if (Top <= 0)
    {
        Top = 0;
    }

    if (Bottom > Backbuffer->Height)
    {
        Bottom = Backbuffer->Height;
    }

    if ((X >= 0) && (X < Backbuffer->Width))
    {
        uint8 *Pixel = ((uint8 *)Backbuffer->Memory + 
                        X*Backbuffer->BytesPerPixel + 
                        Top*Backbuffer->Pitch);
        for(int Y = Top;
            Y < Bottom;
            Y++)
        {
            *(uint32 *)Pixel = Color;
            Pixel += Backbuffer->Pitch;
        }
    }
}


inline void
Win32DrawSoundBufferMarker(win32_offscreen_buffer *Backbuffer, 
                           win32_sound_output *SoundOutput,
                           real32 C, int PadX, int Top, int Bottom,
                           DWORD Value, uint32 Color)
{
    int X = PadX + (int)(C * (real32)(Value));
    Win32DebugDrawVertical(Backbuffer, X, Top, Bottom, Color);
}


internal void 
Win32DebugSyncDisplay(win32_offscreen_buffer *Backbuffer, 
                      int MarkerCount, win32_debug_time_marker *Markers,
                      int CurrentMarkerIndex,
                      win32_sound_output *SoundOutput, real32 TargetSecondsPerFrame)
{
    int PadX = 16;
    int PadY = 16;

    int LineHeight = 64;

    real32 C = (real32)(Backbuffer->Width + 2*PadX) / (real32)SoundOutput->SecondaryBufferSize;
    for(int MarkerIndex = 0;
        MarkerIndex < MarkerCount;
        MarkerIndex++)
    {
        win32_debug_time_marker *ThisMarker = &Markers[MarkerIndex];

        Assert(ThisMarker->OutputPlayCursor < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->OutputWriteCursor < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->OutputLocation < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->OutputByteCount < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->FlipPlayCursor < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->FlipWriteCursor < SoundOutput->SecondaryBufferSize);

        DWORD PlayColor = 0xFFFFFFFF;
        DWORD WriteColor = 0xFFFF0000;
        DWORD ExpectedFlipColor = 0xFFFFFF00;

        int Top = PadY;
        int Bottom = PadY + LineHeight;

        if (MarkerIndex == CurrentMarkerIndex)
        {
            Top += LineHeight + PadY;
            Bottom += LineHeight + PadY;
            Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, Top, Bottom, 
                                       ThisMarker->OutputPlayCursor, PlayColor);
            Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, Top, Bottom, 
                                       ThisMarker->OutputWriteCursor, WriteColor);
            Top += LineHeight + PadY;
            Bottom += LineHeight + PadY;
            Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, Top, Bottom, 
                                       ThisMarker->OutputLocation, PlayColor);
            Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, Top, Bottom, 
                                       ThisMarker->OutputLocation + ThisMarker->OutputByteCount, WriteColor);
            Top += LineHeight + PadY;
            Bottom += LineHeight + PadY;
            Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, PadY, Bottom, 
                                       ThisMarker->ExpectedFlipPlayCursor, ExpectedFlipColor);
        }

        Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, Top, Bottom, 
                                   ThisMarker->FlipPlayCursor, PlayColor);
        Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, Top, Bottom, 
                                   ThisMarker->FlipWriteCursor, WriteColor);
    }
}


internal void 
Win32DisplayBufferInWindow(win32_offscreen_buffer *Buffer, HDC DeviceContext, 
                           int WindowWidth, int WindowHeight)
{
    int OffsetX = 10;
    int OffsetY = 10;

    PatBlt(DeviceContext, 0, 0, WindowWidth, OffsetY, BLACKNESS);
    PatBlt(DeviceContext, 0, OffsetY + Buffer->Height, WindowWidth, WindowHeight, BLACKNESS);
    PatBlt(DeviceContext, 0, 0, OffsetX, WindowHeight, BLACKNESS);
    PatBlt(DeviceContext, OffsetX + Buffer->Width, 0, WindowWidth, WindowHeight, BLACKNESS);

    // NOTE: for prototyping purposes, dont stretch the window
    // so we can see the pixels 1:1 when testing the renderer
    StretchDIBits(
        DeviceContext,
        //X, Y, Width, Height,
        //X, Y, Width, Height,
        OffsetX, OffsetY, Buffer->Width, Buffer->Height,
        0, 0, Buffer->Width, Buffer->Height,
        Buffer->Memory,
        &Buffer->Info,
        DIB_RGB_COLORS, SRCCOPY);
}


LRESULT CALLBACK Win32MainWindowCallback(
  HWND Window,
  UINT Message,
  WPARAM WParam,
  LPARAM LParam)
{
    LRESULT Result = 0;

    switch(Message)
    {
        case WM_SIZE:
        {
            OutputDebugStringA("WM_SIZE\n");
        } break;

        case WM_DESTROY:
        {
            // TODO: handle this as an error & recreate the window
            GlobalRunning = false;
        } break;

        case WM_CLOSE:
        {
            // TODO: handle this with a message to the user
            GlobalRunning = false;
        } break;

        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            Assert(!"Keyboard came in through non dispatch message");
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            int X = Paint.rcPaint.left;
            int Y = Paint.rcPaint.top;

            win32_window_dimension Dimension = Win32GetWindowDimension(Window);
            Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext,
                                       Dimension.Width, Dimension.Height);
            EndPaint(Window, &Paint);
        } break;

        default:
        {
            Result = DefWindowProc(Window, Message, WParam, LParam);
        } break;
    }

    return Result;
}


internal void
Win32ClearBuffer(win32_sound_output *SoundOutput)
{
    VOID *Region1;
    DWORD Region1Size;
    VOID *Region2;
    DWORD Region2Size;
    if (SUCCEEDED(GlobalSecondaryBuffer->Lock(0, SoundOutput->SecondaryBufferSize,
                                              &Region1, &Region1Size,
                                              &Region2, &Region2Size,
                                              0)))
    {
        uint8 *DestSample = (uint8 *)Region1;
        for (DWORD ByteIndex = 0;
             ByteIndex < Region1Size;
             ByteIndex++)
        {
            *DestSample++ = 0;
        }
        DestSample = (uint8 *)Region2;
        for (DWORD ByteIndex = 0;
             ByteIndex < Region2Size;
             ByteIndex++)
        {
            *DestSample++ = 0;
        }

        GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
    }
}


internal void
Win32FillSoundBuffer(win32_sound_output *SoundOutput, DWORD ByteToLock, DWORD BytesToWrite, 
                     game_sound_output_buffer *SourceBuffer)
{
    VOID *Region1;
    DWORD Region1Size;
    VOID *Region2;
    DWORD Region2Size;
    if (SUCCEEDED(GlobalSecondaryBuffer->Lock(ByteToLock, BytesToWrite,
                                              &Region1, &Region1Size,
                                              &Region2, &Region2Size,
                                              0)))
    {
        // TODO: assert region1 and 2 sizes are valid
        DWORD Region1SampleCount = Region1Size/SoundOutput->BytesPerSample;
        int16 *DestSample = (int16 *)Region1;
        int16 *SourceSample = (int16 *)SourceBuffer->Samples;
        // Remaining part of the buffer
        for (DWORD SampleIndex = 0;
             SampleIndex < Region1SampleCount;
             SampleIndex++)
        {
            *DestSample++ = *SourceSample++;
            *DestSample++ = *SourceSample++;
            SoundOutput->RunningSampleIndex++;
        }
        DWORD Region2SampleCount = Region2Size/SoundOutput->BytesPerSample;
        DestSample = (int16 *)Region2;
        // Wrapping around to the beginning of circular buffer, up until play cursor 
        for (DWORD SampleIndex = 0;
             SampleIndex < Region2SampleCount;
             SampleIndex++)
        {
            *DestSample++ = *SourceSample++;
            *DestSample++ = *SourceSample++;
            SoundOutput->RunningSampleIndex++;
        }

        GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
    }
}


internal void
Win32ProcessXInputDigitalButton(DWORD XInputButtonState, game_button_state *OldState, 
                                DWORD ButtonBit, game_button_state *NewState)
{
    NewState->EndedDown = ((XInputButtonState & ButtonBit) == ButtonBit);
    NewState->HalfTransitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
}


internal void
Win32ProcessKeyboardMessage(game_button_state *NewState, bool32 IsDown)
{
    // TODO: This assert doesnt work cuz of toggling between workspaces
    if (NewState->EndedDown != IsDown)
    {
        NewState->EndedDown = IsDown;
        NewState->HalfTransitionCount++;
    }
}


internal void
Win32ProcessPendingMessages(win32_state *State, game_controller_input *KeyboardController)
{
    // If theres no message, it will keep running instead of blocking
    MSG Message;

    while(PeekMessage(
        &Message,
        0, // 0 retrieves messages for any window that belongs to thread
        0, // Returns all available messages
        0, // Returns all available messages
        PM_REMOVE // Removes messages from the queue when processing them
        )) 
    {
        if (Message.message == WM_QUIT) {
            GlobalRunning = false;
        }

        switch (Message.message)
        {
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            {

                uint32 VKCode = (uint32)Message.wParam;
                bool32 WasDown = ((Message.lParam & (1 << 30)) != 0);
                bool32 IsDown = ((Message.lParam & (1 << 31)) == 0);

                if (IsDown != WasDown)
                {
                    if (VKCode == 'W') 
                    {
                        Win32ProcessKeyboardMessage(
                                &KeyboardController->MoveUp,
                                IsDown);
                    }
                    // dvorak
                    if (VKCode == VK_OEM_COMMA) 
                    {
                        Win32ProcessKeyboardMessage(
                                &KeyboardController->MoveUp,
                                IsDown);
                    }
                    else if (VKCode == 'A') 
                    {
                        Win32ProcessKeyboardMessage(
                                &KeyboardController->MoveLeft,
                                IsDown);
                    }
                    else if (VKCode == 'S') 
                    {
                        Win32ProcessKeyboardMessage(
                                &KeyboardController->MoveDown,
                                IsDown);
                    }
                    // DVORAK
                    else if (VKCode == 'O') 
                    {
                        Win32ProcessKeyboardMessage(
                                &KeyboardController->MoveDown,
                                IsDown);
                    }
                    else if (VKCode == 'D') 
                    {
                        Win32ProcessKeyboardMessage(
                                &KeyboardController->MoveRight,
                                IsDown);
                    }
                    // DVORAK
                    else if (VKCode == 'E') 
                    {
                        Win32ProcessKeyboardMessage(
                                &KeyboardController->MoveRight,
                                IsDown);
                    }
                    else if (VKCode == 'Q') 
                    {
                        Win32ProcessKeyboardMessage(
                                &KeyboardController->LeftShoulder, 
                                IsDown);
                    }
                    else if (VKCode == 'E') 
                    {
                        Win32ProcessKeyboardMessage(
                                &KeyboardController->RightShoulder, 
                                IsDown);
                    }
                    else if (VKCode == VK_UP) 
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->ActionUp, 
                                                    IsDown);
                    }
                    else if (VKCode == VK_DOWN) 
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->ActionDown, 
                                                    IsDown);
                    }
                    else if (VKCode == VK_LEFT) 
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->ActionLeft, 
                                                    IsDown);
                    }
                    else if (VKCode == VK_RIGHT) 
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->ActionRight, 
                                                    IsDown);
                    }
                    else if (VKCode == VK_ESCAPE) 
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->Start, 
                                                    IsDown);
                    }
                    else if (VKCode == VK_SPACE) 
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->Back, 
                                                    IsDown);
                    }
#if HANDMADE_INTERNAL
                    else if (VKCode == 'P') 
                    {
                        if (IsDown)
                        {
                            GlobalPause = !GlobalPause;
                        }
                    }
                    else if (VKCode == 'L') 
                    {
                        if (IsDown)
                        {
                            if (State->InputPlayingIndex == 0)
                            {
                                if (State->InputRecordingIndex == 0)
                                {
                                    Win32BeginRecordingInput(State, 1);
                                }
                                else
                                {
                                    Win32EndRecordingInput(State);
                                    Win32BeginInputPlayBack(State, 1);
                                }
                            }
                            else 
                            {
                                Win32EndInputPlayBack(State);
                            }
                        }

                    }

#endif
                }

                bool32 AltKeyWasDown = ((Message.lParam & (1 << 29)) != 0);
                if ((VKCode == VK_F4) && AltKeyWasDown) 
                {
                    GlobalRunning = false;
                }
            } break;

            default:
            {
                // Gets message ready to send out
                TranslateMessage(&Message);
                // Actually sends it out
                DispatchMessage(&Message);
            } break;
        }
    }
}


internal real32
Win32ProcessXInputStickValue(SHORT Value, SHORT DeadZoneThreshold)
{
    real32 Result = 0;
    if (Value < -DeadZoneThreshold)
    {
        Result = (real32)(Value) / 32768.0f;
    }
    else if (Value > DeadZoneThreshold)
    {
        Result = (real32)(Value) / 32767.0f;
    }
    return Result;
}


inline real32
Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
    real32 Result = (((real32)(End.QuadPart - Start.QuadPart)) / 
                                        (real32)GlobalPerfCountFrequency);

    return Result;
}

inline LARGE_INTEGER
Win32GetWallClock()
{
    LARGE_INTEGER Result;
    QueryPerformanceCounter(&Result);
    return Result;
}

int CALLBACK WinMain(
    HINSTANCE Instance, 
    HINSTANCE PrevInstance, 
    LPSTR CmdLine, 
    int ShowCode) 
{
    win32_state State = {};

    LARGE_INTEGER PerfCountFrequencyResult;
    QueryPerformanceFrequency(&PerfCountFrequencyResult);
    GlobalPerfCountFrequency = PerfCountFrequencyResult.QuadPart;

    Win32GetEXEFileName(&State);

    char SourceGameCodeDLLFullPath[WIN32_STATE_FILE_NAME_COUNT];
    Win32BuildEXEPathFileName(&State, "handmade.dll",
                              sizeof(SourceGameCodeDLLFullPath), SourceGameCodeDLLFullPath);

    char TempGameCodeDLLFullPath[WIN32_STATE_FILE_NAME_COUNT];
    Win32BuildEXEPathFileName(&State, "handmade_temp.dll",
                              sizeof(TempGameCodeDLLFullPath), TempGameCodeDLLFullPath);

    // NOTE: set the windows scheduler granularity to 1ms
    // So that our Sleep() can be more granular
    UINT DesiredSchedulerMs = 1;
    bool32 SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMs) == TIMERR_NOERROR);

    Win32LoadXInput();

    WNDCLASSA WindowClass = {};

    /*Win32ResizeDIBSection(&GlobalBackbuffer, 1920, 1080);*/
    Win32ResizeDIBSection(&GlobalBackbuffer, 960, 540);

    WindowClass.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    /*WindowClass.hIcon = ;*/
    WindowClass.lpszClassName = "BussinWindowClass";

    if (RegisterClassA(&WindowClass)) {
        HWND Window = CreateWindowEx(
            0,
            WindowClass.lpszClassName,
            "BussinGameFr",
            WS_OVERLAPPEDWINDOW|WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            0,
            0,
            Instance,
            0
        );

        if (Window) {
            // We should only need one DC because we specified OWN_DC
            HDC DeviceContext = GetDC(Window);

            int MonitorRefreshHz = 60;
            int Win32RefreshRate = GetDeviceCaps(DeviceContext, VREFRESH);
            if (Win32RefreshRate > 1)
            {
                MonitorRefreshHz = Win32RefreshRate;
            }

            real32 GameUpdateHz = (MonitorRefreshHz / 1.0f);
            real32 TargetSecondsPerFrame = 1.0f / GameUpdateHz;

            // NOTE: Graphics test
            win32_sound_output SoundOutput = {};

            SoundOutput.SamplesPerSecond = 48000;
            SoundOutput.RunningSampleIndex = 0;
            SoundOutput.BytesPerSample = sizeof(int16)*2;
            SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond*SoundOutput.BytesPerSample;
            // TODO: Actually compute this variance and see 
            // what the lowest reasonable value is.
            SoundOutput.SafetyBytes = (int)((((real32)(SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample)) 
                                            / GameUpdateHz) / 3.0f);

            Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
            Win32ClearBuffer(&SoundOutput);
            GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

            // TODO: Pool with bitmap virtual alloc
            int16 *Samples = (int16 *)VirtualAlloc(0, SoundOutput.SecondaryBufferSize, 
                                                   MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

#if HANDMADE_INTERNAL
            LPVOID BaseAddress = (LPVOID)Terabytes((uint64)2);
#else
            LPVOID BaseAddress = 0;
#endif

            game_memory GameMemory = {};
            GameMemory.PermanentStorageSize = Megabytes(64);
            GameMemory.TransientStorageSize = Gigabytes(1);
            GameMemory.DEBUGPlatformReadEntireFile = DEBUGPlatformReadEntireFile;
            GameMemory.DEBUGPlatformFreeFileMemory = DEBUGPlatformFreeFileMemory;
            GameMemory.DEBUGPlatformWriteEntireFile = DEBUGPlatformWriteEntireFile;

            State.TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
            // TODO: need to use MEM_LARGE_PAGES to take pressure off TLB
            State.GameMemoryBlock = VirtualAlloc(BaseAddress, (size_t)State.TotalSize, 
                                                       MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            GameMemory.PermanentStorage = State.GameMemoryBlock;
            GameMemory.TransientStorage = ((uint8 *)GameMemory.PermanentStorage + 
                                            GameMemory.PermanentStorageSize);
            for (int ReplayIndex = 0;
                ReplayIndex < ArrayCount(State.ReplayBuffers);
                ReplayIndex++)
            {
                win32_replay_buffer *ReplayBuffer = &State.ReplayBuffers[ReplayIndex];
                Win32GetInputFileLocation(&State, false, ReplayIndex,
                              sizeof(ReplayBuffer->Filename), ReplayBuffer->Filename);
                ReplayBuffer->FileHandle = 
                    CreateFileA(ReplayBuffer->Filename, GENERIC_READ | GENERIC_WRITE, 
                                0, 0, CREATE_ALWAYS, 0, 0);
                LARGE_INTEGER MaxSize;
                MaxSize.QuadPart = State.TotalSize + 1024;
                ReplayBuffer->MemoryMap = CreateFileMapping(ReplayBuffer->FileHandle, 0, 
                                                            PAGE_READWRITE,
                                                            MaxSize.HighPart, MaxSize.LowPart, 0);
                ReplayBuffer->MemoryBlock = MapViewOfFile(ReplayBuffer->MemoryMap, 
                                                          FILE_MAP_ALL_ACCESS, 
                                                          0, 0, (size_t)State.TotalSize);

                Assert(ReplayBuffer->MemoryBlock);
            }


            if (Samples && GameMemory.PermanentStorage && GameMemory.TransientStorage)
            {
                game_input Input[2] = {};
                game_input *NewInput = &Input[0];
                game_input *OldInput = &Input[1];

                LARGE_INTEGER LastCounter = Win32GetWallClock();
                LARGE_INTEGER FlipWallClock = Win32GetWallClock();

                int DebugTimeMarkerIndex = 0;
                win32_debug_time_marker DebugTimeMarkers[30] = {};

                bool32 SoundIsValid = false;
                DWORD AudioLatencyBytes = 0;
                real32 AudioLatencySeconds = 0;

                win32_game_code Game = Win32LoadGameCode(SourceGameCodeDLLFullPath,
                                                         TempGameCodeDLLFullPath);

                thread_context Thread = {};

                int LoadCounter = 0;

                // Apparently this is a more optimized infinite loop
                uint64 LastCycleCount = __rdtsc();
                GlobalRunning = true;
                while (GlobalRunning) {
                    FILETIME NewDLLWriteTime = GetFileLastWriteTime(SourceGameCodeDLLFullPath);
                    if (CompareFileTime(&NewDLLWriteTime, &Game.LastDLLWriteTime))
                    {
                        Win32UnloadGameCode(&Game);
                        Game = Win32LoadGameCode(SourceGameCodeDLLFullPath,
                                                 TempGameCodeDLLFullPath);
                        LoadCounter = 0;
                    }

                    game_controller_input *OldKeyboardController = GetController(OldInput, 0);
                    game_controller_input *NewKeyboardController = GetController(NewInput, 0);
                    game_controller_input ZeroController = {};
                    *NewKeyboardController = ZeroController;
                    NewKeyboardController->IsConnected = true;
                    for (int ButtonIndex=0; 
                         ButtonIndex < ArrayCount(NewKeyboardController->Buttons);
                         ButtonIndex++)
                    {
                        NewKeyboardController->Buttons[ButtonIndex].EndedDown = 
                            OldKeyboardController->Buttons[ButtonIndex].EndedDown;
                    }

                    Win32ProcessPendingMessages(&State, NewKeyboardController);

                    if (!GlobalPause)
                    {
                        NewInput->dtForFrame = TargetSecondsPerFrame;

                        POINT MouseP;
                        GetCursorPos(&MouseP);
                        ScreenToClient(Window, &MouseP);
                        NewInput->MouseX = MouseP.x;
                        NewInput->MouseY = MouseP.y;
                        // TODO: support mouse wheel
                        NewInput->MouseZ = 0;
                        Win32ProcessKeyboardMessage(&NewInput->MouseButtons[0],
                                                    GetKeyState(VK_LBUTTON) & (1 << 15));
                        Win32ProcessKeyboardMessage(&NewInput->MouseButtons[1],
                                                    GetKeyState(VK_MBUTTON) & (1 << 15));
                        Win32ProcessKeyboardMessage(&NewInput->MouseButtons[2],
                                                    GetKeyState(VK_RBUTTON) & (1 << 15));
                        Win32ProcessKeyboardMessage(&NewInput->MouseButtons[3],
                                                    GetKeyState(VK_XBUTTON1) & (1 << 15));
                        Win32ProcessKeyboardMessage(&NewInput->MouseButtons[4],
                                                    GetKeyState(VK_XBUTTON2) & (1 << 15));

                        // TODO: should we poll more frequently?
                        DWORD MaxControllerCount = XUSER_MAX_COUNT;
                        if (MaxControllerCount > ArrayCount(NewInput->Controllers) - 1)
                        {
                            MaxControllerCount = ArrayCount(NewInput->Controllers) - 1;
                        }

                        for (DWORD ControllerIndex = 0;
                             ControllerIndex < MaxControllerCount;
                             ControllerIndex++)
                        {
                            DWORD OurControllerIndex = ControllerIndex + 1;

                            game_controller_input *OldController = GetController(OldInput, OurControllerIndex);
                            game_controller_input *NewController = GetController(NewInput, OurControllerIndex);

                            XINPUT_STATE ControllerState; 
                            if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
                            {
                                NewController->IsConnected = true;

                                // This controller is plugged in
                                // TODO: see if ControllerState.dwPacketNumber advanrces too rapidly

                                XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

                                NewController->IsAnalog = OldController->IsAnalog;
                                NewController->StickAverageX = Win32ProcessXInputStickValue(
                                                            Pad->sThumbLX, 
                                                            XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

                                NewController->StickAverageY = Win32ProcessXInputStickValue(
                                                            Pad->sThumbLY, 
                                                            XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

                                if ((NewController->StickAverageX != 0.0f) ||
                                    (NewController->StickAverageY != 0.0f))
                                {
                                    NewController->IsAnalog = true;
                                }

                                if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP)
                                {
                                    NewController->StickAverageY = 1.0f;
                                    NewController->IsAnalog = false;
                                }
                                if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
                                {
                                    NewController->StickAverageY = -1.0f;
                                    NewController->IsAnalog = false;
                                }
                                if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
                                {
                                    NewController->StickAverageX = 1.0f;
                                    NewController->IsAnalog = false;
                                }
                                if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT)
                                {
                                    NewController->StickAverageX = -1.0f;
                                    NewController->IsAnalog = false;
                                }

                                real32 Threshold = 0.5f;
                                Win32ProcessXInputDigitalButton(
                                    (NewController->StickAverageX < -Threshold ? 1 : 0),
                                    &OldController->MoveLeft, 1,
                                    &NewController->MoveLeft);
                                Win32ProcessXInputDigitalButton(
                                    (NewController->StickAverageX > Threshold ? 1 : 0),
                                    &OldController->MoveRight, 1,
                                    &NewController->MoveRight);
                                Win32ProcessXInputDigitalButton(
                                    (NewController->StickAverageY < -Threshold ? 1 : 0),
                                    &OldController->MoveDown, 1,
                                    &NewController->MoveDown);
                                Win32ProcessXInputDigitalButton(
                                    (NewController->StickAverageY > Threshold ? 1 : 0),
                                    &OldController->MoveUp, 1,
                                    &NewController->MoveUp);

                                Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                                &OldController->ActionDown, 
                                                                XINPUT_GAMEPAD_A,
                                                                &NewController->ActionDown);
                                Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                                &OldController->ActionRight, 
                                                                XINPUT_GAMEPAD_B,
                                                                &NewController->ActionRight);
                                Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                                &OldController->ActionLeft, 
                                                                XINPUT_GAMEPAD_X,
                                                                &NewController->ActionLeft);
                                Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                                &OldController->ActionUp, 
                                                                XINPUT_GAMEPAD_Y,
                                                                &NewController->ActionUp);
                                Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                                &OldController->LeftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER,
                                                                &NewController->LeftShoulder);
                                Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                                &OldController->RightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER,
                                                                &NewController->RightShoulder);

                                Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                                &OldController->Start, XINPUT_GAMEPAD_RIGHT_SHOULDER,
                                                                &NewController->Start);
                                Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                                &OldController->Back, XINPUT_GAMEPAD_RIGHT_SHOULDER,
                                                                &NewController->Back);
                            }
                            else 
                            {
                                // This controller is not available
                                NewController->IsConnected = false;
                            }
                        }

                        /*XINPUT_VIBRATION Vibration;*/
                        /*Vibration.wLeftMotorSpeed = 60000;*/
                        /*Vibration.wRightMotorSpeed = 60000;*/
                        /*XInputSetState(0, &Vibration);*/

                        game_offscreen_buffer VideoBuffer = {};
                        VideoBuffer.Memory = GlobalBackbuffer.Memory;
                        VideoBuffer.Width = GlobalBackbuffer.Width;
                        VideoBuffer.Height = GlobalBackbuffer.Height;
                        VideoBuffer.Pitch = GlobalBackbuffer.Pitch;
                        VideoBuffer.BytesPerPixel = GlobalBackbuffer.BytesPerPixel;

                        if (State.InputRecordingIndex)
                        {
                            Win32RecordInput(&State, NewInput);
                        }

                        if (State.InputPlayingIndex)
                        {
                            Win32PlayBackInput(&State, NewInput);
                        }
                        if (Game.UpdateAndRender)
                        {
                            Game.UpdateAndRender(&Thread, &GameMemory, NewInput, &VideoBuffer);
                        }

                        LARGE_INTEGER AudioWallClock = Win32GetWallClock();
                        real32 FromBeginToAudioSeconds = Win32GetSecondsElapsed(FlipWallClock, AudioWallClock);

                        DWORD PlayCursor;
                        DWORD WriteCursor;
                        if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
                        {
                            /* NOTE(casey):
       
                               Here is how sound output computation works.
                             
                               We define a safety value that is the number 
                               of samples we think our game update loop 
                               may vary by (let's say up to 2ms). 
                             
                               When we wake up to write audio, we will look
                               and see what the play cursor position is and we
                               will forecast ahead where we think the
                               play cursor will be on the next frame boundary.
                             
                               We will then look to see if the write cursor is
                               before that by at least our safety value. If it is, the
                               target fill position is that frame boundary
                               plus one frame. This gives us perfect audio
                               sync in the case of a card that has low enough
                               latency.
                             
                               If the write cursor is _after_ that safety 
                               margin, then we assume we can never sync the
                               audio perfectly, so we will write one frame's
                               worth of audio plus the safety margin's worth
                               of guard samples.
                            */

                            if (!SoundIsValid)
                            {
                                SoundOutput.RunningSampleIndex = WriteCursor / SoundOutput.BytesPerSample;
                                SoundIsValid = true;
                            }

                            DWORD ByteToLock = ((SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample)
                                                % SoundOutput.SecondaryBufferSize);

                            DWORD ExpectedSoundBytesPerFrame = (DWORD)(((real32)SoundOutput.SamplesPerSecond * (real32)SoundOutput.BytesPerSample) / GameUpdateHz);
                            real32 SecondsLeftUntilFlip = TargetSecondsPerFrame - FromBeginToAudioSeconds;
                            DWORD ExpectedBytesUntilFlip = (DWORD)((SecondsLeftUntilFlip / TargetSecondsPerFrame) * (real32)ExpectedSoundBytesPerFrame);
                            DWORD ExpectedFrameBoundaryByte = PlayCursor + ExpectedBytesUntilFlip;
                            DWORD SafeWriteCursor = WriteCursor;
                            if (SafeWriteCursor < PlayCursor)
                            {
                                SafeWriteCursor += SoundOutput.SecondaryBufferSize;
                            }
                            Assert(SafeWriteCursor >= PlayCursor);
                            SafeWriteCursor += SoundOutput.SafetyBytes;

                            bool32 AudioCardIsLowLatency = SafeWriteCursor < ExpectedFrameBoundaryByte;

                            DWORD TargetCursor = 0;
                            if (AudioCardIsLowLatency)
                            {
                                TargetCursor = ExpectedFrameBoundaryByte + ExpectedSoundBytesPerFrame;
                            }
                            else
                            {
                                TargetCursor = WriteCursor + ExpectedSoundBytesPerFrame + SoundOutput.SafetyBytes;
                            }
                            TargetCursor = TargetCursor % SoundOutput.SecondaryBufferSize;

                            DWORD BytesToWrite = 0;
                            if (ByteToLock > TargetCursor)
                            {
                                BytesToWrite = SoundOutput.SecondaryBufferSize - ByteToLock;
                                BytesToWrite += TargetCursor;
                            }
                            else
                            {
                                BytesToWrite = TargetCursor - ByteToLock;
                            }

                            game_sound_output_buffer SoundBuffer = {};
                            SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
                            SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
                            SoundBuffer.Samples = Samples;

                            if (Game.GetSoundSamples)
                            {
                                Game.GetSoundSamples(&Thread, &GameMemory, &SoundBuffer);
                            }

                            Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite, &SoundBuffer);
#if HANDMADE_INTERNAL
                            {
                                win32_debug_time_marker *Marker = &DebugTimeMarkers[DebugTimeMarkerIndex];
                                Marker->OutputPlayCursor = PlayCursor;
                                Marker->OutputWriteCursor = WriteCursor;
                                Marker->OutputLocation = ByteToLock;
                                Marker->OutputByteCount = BytesToWrite;
                                Marker->ExpectedFlipPlayCursor = ExpectedFrameBoundaryByte;

                                DWORD UnwrappedWriteCursor = WriteCursor;
                                if (UnwrappedWriteCursor < PlayCursor)
                                {
                                    UnwrappedWriteCursor += SoundOutput.SecondaryBufferSize;
                                }
                                AudioLatencyBytes = UnwrappedWriteCursor - PlayCursor;
                                AudioLatencySeconds = 
                                    (((real32)AudioLatencyBytes / (real32)SoundOutput.BytesPerSample) /
                                     (real32)SoundOutput.SamplesPerSecond);

#if 0
                                char TextBuffer[256];
                                sprintf_s(TextBuffer, 
                                          "BTL:%u TC:%u BTW:%u - PC:%u WC:%u DELTA:%u (%fs)\n", 
                                          ByteToLock, TargetCursor, BytesToWrite,
                                          PlayCursor, WriteCursor, AudioLatencyBytes, AudioLatencySeconds);
                                OutputDebugStringA(TextBuffer);
#endif
                            }
#endif
                        }
                        else
                        {
                            SoundIsValid = false;
                        }

                        LARGE_INTEGER WorkCounter = Win32GetWallClock();
                        real32 WorkSecondsElapsed = Win32GetSecondsElapsed(LastCounter, WorkCounter);

                        // TODO: not tested yet, probably buggy
                        // NOTE: This is where we wait until the next frame to start working again
                        real32 SecondsElapsedForFrame = WorkSecondsElapsed;
                        if (SecondsElapsedForFrame < TargetSecondsPerFrame)
                        {
                            // TODO: figure out how to get this code working in Win11
                            /*if (SleepIsGranular)*/
                            /*{*/
                            /*    DWORD SleepMs = (DWORD)(1000.0f * (TargetSecondsPerFrame - */
                            /*                                SecondsElapsedForFrame));*/
                            /*    if (SleepMs > 0)*/
                            /*    {*/
                            /*        Sleep(SleepMs);*/
                            /*    }*/
                            /*}*/
                            
                            real32 TestSecondsElapsedForFrame = 
                                Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
                            Assert(TestSecondsElapsedForFrame < TargetSecondsPerFrame);

                            while (SecondsElapsedForFrame < TargetSecondsPerFrame)
                            {
                                SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, 
                                                                                Win32GetWallClock());
                            }
                        }
                        else
                        {
                            // TODO: missed frame rate
                            // TODO: Logging
                        }

                        LARGE_INTEGER EndCounter = Win32GetWallClock();
                        real64 MSPerFrame = 1000.0f*SecondsElapsedForFrame;
                        LastCounter = EndCounter;

                        win32_window_dimension Dimension = Win32GetWindowDimension(Window);
#if 0
                        // NOTE: index is wrong on 0th index
                        Win32DebugSyncDisplay(&GlobalBackbuffer, 
                                              ArrayCount(DebugTimeMarkers), DebugTimeMarkers,
                                              DebugTimeMarkerIndex - 1,
                                              &SoundOutput, TargetSecondsPerFrame);
#endif
                        Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext,
                                                   Dimension.Width, Dimension.Height);
                        FlipWallClock = Win32GetWallClock();

                        // NOTE: this is debug code
#if HANDMADE_INTERNAL
                        {
                            DWORD FlipPlayCursor = 0;
                            DWORD FlipWriteCursor = 0;
                            if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&FlipPlayCursor, &FlipWriteCursor)))
                            {
                                Assert(DebugTimeMarkerIndex < ArrayCount(DebugTimeMarkers))
                                win32_debug_time_marker *Marker = &DebugTimeMarkers[DebugTimeMarkerIndex];
                                if (DebugTimeMarkerIndex >= ArrayCount(DebugTimeMarkers))
                                {
                                    DebugTimeMarkerIndex = 0;
                                }
                                Marker->FlipPlayCursor = FlipPlayCursor;
                                Marker->FlipWriteCursor = FlipWriteCursor;
                            }

                            ++DebugTimeMarkerIndex;
                            if (DebugTimeMarkerIndex == ArrayCount(DebugTimeMarkers))
                            {
                                DebugTimeMarkerIndex = 0;
                            }
                        }
#endif

                        game_input *Temp = OldInput;
                        OldInput = NewInput;
                        NewInput = Temp;

#if 1
                        uint64 EndCycleCount = __rdtsc();
                        uint64 CyclesElapsed = EndCycleCount - LastCycleCount;
                        LastCycleCount = EndCycleCount;

                        real64 Fps = 0.0f;
                        real64 Mcpf = ((real64)(CyclesElapsed) / (1000.0f * 1000.0f));

                        char FPSBuffer[256];
                        sprintf_s(FPSBuffer, "%.02fms/f, %.02ff/s, %.02fmc/f\n", MSPerFrame, Fps, Mcpf);
                        OutputDebugStringA(FPSBuffer);
#endif
                    }
                }
            }

            ReleaseDC(Window, DeviceContext);

            if (Samples) {
                VirtualFree(Samples, 0, MEM_RELEASE);
            }
        }
    }
    else {
        // TODO: do error 
    }

    return 0;
}
