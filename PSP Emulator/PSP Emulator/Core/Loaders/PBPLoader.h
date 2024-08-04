#pragma once

#include <Core/Loaders/AbstractLoader.h>
#include <Core/Loaders/ELFLoader.h>

namespace Core::Loader {
enum : int {
    PBP_PARAM_SFO,
    PBP_ICON0_PNG,
    PBP_ICON1_PMF,
    PBP_PIC0_PNG,
    PBP_PIC1_PNG,
    PBP_SND0_AT3,
    PBP_EXECUTABLE_PSP,
    PBP_UNKNOWN_PSAR,
};

struct PBPHeader {
    char magic[4];
    uint32_t version;
    uint32_t offsets[8];
};

struct PBPLoader : public AbstractLoader {
private:
public:
    PBPLoader();
    ~PBPLoader();

    ParamSFO *getParamSFO();
    PNGData *getPNG(int index);
    MPEGPlayer *getMPEGPlayer();
    AT3Player *getAT3Player();

    int loadProgram();
};
}