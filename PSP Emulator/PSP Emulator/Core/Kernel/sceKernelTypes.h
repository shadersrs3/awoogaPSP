#pragma once

namespace Core::Kernel {
enum : int {
    WAITTYPE_NONE = 0,
    WAITTYPE_SLEEP = 1,
    WAITTYPE_DELAY = 2,
    WAITTYPE_SEMA = 3,
    WAITTYPE_EVENTFLAG = 4,
    WAITTYPE_MBX = 5,
    WAITTYPE_VPL = 6,
    WAITTYPE_FPL = 7,
    WAITTYPE_MSGPIPE = 8,  // fake
    WAITTYPE_THREADEND = 9,
    WAITTYPE_AUDIOCHANNEL = 10, // this is fake, should be replaced with 8 eventflags   ( ?? )
    WAITTYPE_UMD = 11, // this is fake, should be replaced with 1 eventflag    ( ?? )
    WAITTYPE_VBLANK = 12, // fake
    WAITTYPE_MUTEX = 13,
    WAITTYPE_LWMUTEX = 14,
    WAITTYPE_CTRL = 15,
    WAITTYPE_IO = 16,
    WAITTYPE_GEDRAWSYNC = 17,
    WAITTYPE_GELISTSYNC = 18,
    WAITTYPE_MODULE = 19,
    WAITTYPE_HLEDELAY = 20,
    WAITTYPE_TLSPL = 21,
    WAITTYPE_VMEM = 22,
    WAITTYPE_ASYNCIO = 23,
    WAITTYPE_MICINPUT = 24, // fake
    WAITTYPE_NET = 25, // fake
    WAITTYPE_USB = 26, // fake
    NUM_WAITTYPES
};
}