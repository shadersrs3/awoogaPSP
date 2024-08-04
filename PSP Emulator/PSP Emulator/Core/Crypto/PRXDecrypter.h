// Copyright (c) 2012- PPSSPP Project.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 2.0.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License 2.0 for more details.

// A copy of the GPL 2.0 should have been included with the program.
// If not, see http://www.gnu.org/licenses/

// Official git repository and contact information can be found at
// https://github.com/hrydgard/ppsspp and http://www.ppsspp.org/.

#pragma once

#include <cstdint>

namespace Core::Crypto {

#ifdef _MSC_VER
#pragma pack(push, 1)
#endif
typedef struct
{
    uint32_t signature;
    uint16_t attribute;
    uint16_t comp_attribute;
    uint8_t module_ver_lo;
    uint8_t module_ver_hi;
    char modname[28];
    uint8_t version;
    uint8_t nsegments;
    uint32_t elf_size;
    uint32_t psp_size;
    uint32_t entry;
    uint32_t modinfo_offset;
    int32_t bss_size;
    uint16_t seg_align[4];
    uint32_t seg_address[4];
    int32_t seg_size[4];
    uint32_t reserved[5];
    uint32_t devkitversion;
    uint32_t decrypt_mode;
    uint8_t key_data0[0x30];
    int32_t comp_size;
    int32_t _80;
    int32_t reserved2[2];
    uint8_t key_data1[0x10];
    uint32_t tag;
    uint8_t scheck[0x58];
    uint32_t key_data2;
    uint32_t oe_tag;
    uint8_t key_data3[0x1C];
#ifdef _MSC_VER
} PSP_Header;
#else
} __attribute__((packed)) PSP_Header;
#endif

#ifdef _MSC_VER
#pragma pack(pop)
#endif

int decrypt(const uint8_t *inbuf, uint8_t *outbuf, uint32_t size, const uint8_t *seed = nullptr, bool verbose = false);
}