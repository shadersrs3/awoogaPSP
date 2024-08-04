#pragma once

namespace Core::GPU {
int sceDisplayWaitVblankStartCB();
int sceDisplayWaitVblankStart();
int sceDisplayWaitVblank();
int sceDisplayIsVblank();
int sceDisplayGetCurrentHcount();
int sceDisplayGetVcount();
}