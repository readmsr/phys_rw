#pragma once
#include <ntifs.h>

typedef struct _MOUSE_INPUT_DATA 
{
    USHORT UnitId;
    USHORT Flags;
    union {
        ULONG Buttons;
        struct {
            USHORT ButtonFlags;
            USHORT ButtonData;
        } s;
    };
    ULONG  RawButtons;
    LONG   LastX;
    LONG   LastY;
    ULONG  ExtraInformation;
} MOUSE_INPUT_DATA, * PMOUSE_INPUT_DATA;

typedef void(NTAPI* pMouseClassServiceCallback)(
    PDEVICE_OBJECT DeviceObject,
    PMOUSE_INPUT_DATA InputDataStart,
    PMOUSE_INPUT_DATA InputDataEnd,
    PULONG InputDataConsumed
    );

namespace mouse
{
    bool init();
    void move(int x, int y, unsigned short flags);
}