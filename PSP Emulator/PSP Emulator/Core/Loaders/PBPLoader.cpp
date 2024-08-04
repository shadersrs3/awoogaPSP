#include <Core/Loaders/PBPLoader.h>

namespace Core::Loader {
PBPLoader::PBPLoader() {

}

PBPLoader::~PBPLoader() {

}

int PBPLoader::loadProgram() {
    return getELFLoader()->loadProgram();
}

ParamSFO *PBPLoader::getParamSFO() {
    return nullptr;
}

PNGData *PBPLoader::getPNG(int index) {
    return nullptr;
}

MPEGPlayer *PBPLoader::getMPEGPlayer() {
    return nullptr;
}

AT3Player *PBPLoader::getAT3Player() {
    return nullptr;
}
}