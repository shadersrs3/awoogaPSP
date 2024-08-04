#pragma once

enum ge_VType
{
    GE_VTYPE_TRANSFORM      = (0<<23),
    GE_VTYPE_THROUGH        = (1<<23),
    GE_VTYPE_THROUGH_MASK   = (1<<23),

    GE_VTYPE_TC_NONE        = (0<<0),
    GE_VTYPE_TC_8BIT        = (1<<0),
    GE_VTYPE_TC_16BIT       = (2<<0),
    GE_VTYPE_TC_FLOAT       = (3<<0),
    GE_VTYPE_TC_MASK        = (3<<0),
    GE_VTYPE_TC_SHIFT       = 0,

    GE_VTYPE_COL_NONE       = (0<<2),
    GE_VTYPE_COL_565        = (4<<2),
    GE_VTYPE_COL_5551       = (5<<2),
    GE_VTYPE_COL_4444       = (6<<2),
    GE_VTYPE_COL_8888       = (7<<2),
    GE_VTYPE_COL_MASK       = (7<<2),
    GE_VTYPE_COL_SHIFT      = 2,

    GE_VTYPE_NRM_NONE       = (0<<5),
    GE_VTYPE_NRM_8BIT       = (1<<5),
    GE_VTYPE_NRM_16BIT      = (2<<5),
    GE_VTYPE_NRM_FLOAT      = (3<<5),
    GE_VTYPE_NRM_MASK       = (3<<5),
    GE_VTYPE_NRM_SHIFT      = 5,

    GE_VTYPE_POSITION_NONE  = (0<<7),
    GE_VTYPE_POS_8BIT       = (1<<7),
    GE_VTYPE_POS_16BIT      = (2<<7),
    GE_VTYPE_POS_FLOAT      = (3<<7),
    GE_VTYPE_POS_MASK       = (3<<7),
    GE_VTYPE_POS_SHIFT      = 7,

    GE_VTYPE_WEIGHT_NONE    = (0<<9),
    GE_VTYPE_WEIGHT_8BIT    = (1<<9),
    GE_VTYPE_WEIGHT_16BIT   = (2<<9),
    GE_VTYPE_WEIGHT_FLOAT   = (3<<9),
    GE_VTYPE_WEIGHT_MASK    = (3<<9),
    GE_VTYPE_WEIGHT_SHIFT   = 9,

    GE_VTYPE_SKIN_MASK      = (7<<14),
    GE_VTYPE_SKIN_SHIFT     = 14,

    GE_VTYPE_MORPH_MASK     = (7<<19),
    GE_VTYPE_MORPH_SHIFT    = 19,

    GE_VTYPE_IDX_NONE       = (0<<11),
    GE_VTYPE_IDX_8BIT       = (1<<11),
    GE_VTYPE_IDX_16BIT      = (2<<11),
    GE_VTYPE_IDX_MASK       = (3<<11),
    GE_VTYPE_IDX_SHIFT      = 11,

    GE_VTYPE_ALL_MASK       = GE_VTYPE_THROUGH_MASK|GE_VTYPE_TC_MASK|GE_VTYPE_COL_MASK|GE_VTYPE_NRM_MASK|GE_VTYPE_POS_MASK|GE_VTYPE_WEIGHT_MASK|GE_VTYPE_SKIN_MASK
};

enum {
    GE_TFMT_5650 = 0,
    GE_TFMT_5551 = 1,
    GE_TFMT_4444 = 2,
    GE_TFMT_8888 = 3,
    GE_TFMT_CLUT4 = 4,
    GE_TFMT_CLUT8 = 5,
    GE_TFMT_CLUT16 = 6,
    GE_TFMT_CLUT32 = 7,
    GE_TFMT_DXT1 = 8,
    GE_TFMT_DXT3 = 9,
    GE_TFMT_DXT5 = 10,
    GE_TFMT_RESERVED0 = 11,
    GE_TFMT_RESERVED1 = 12,
    GE_TFMT_RESERVED2 = 13,
    GE_TFMT_RESERVED3 = 14,
    GE_TFMT_RESERVED4 = 15,
};

// stencil test
#define STST_FUNCTION_NEVER_PASS_STENCIL_TEST        0x00
#define STST_FUNCTION_ALWAYS_PASS_STENCIL_TEST       0x01
#define STST_FUNCTION_PASS_TEST_IF_MATCHES           0x02
#define STST_FUNCTION_PASS_TEST_IF_DIFFERS           0x03
#define STST_FUNCTION_PASS_TEST_IF_LESS              0x04
#define STST_FUNCTION_PASS_TEST_IF_LESS_OR_EQUAL     0x05
#define STST_FUNCTION_PASS_TEST_IF_GREATER           0x06
#define STST_FUNCTION_PASS_TEST_IF_GREATER_OR_EQUAL  0x07

// stencil op
#define SOP_KEEP_STENCIL_VALUE        0x00
#define SOP_ZERO_STENCIL_VALUE        0x01
#define SOP_REPLACE_STENCIL_VALUE     0x02
#define SOP_INVERT_STENCIL_VALUE      0x03
#define SOP_INCREMENT_STENCIL_VALUE   0x04
#define SOP_DECREMENT_STENCIL_VALUE   0x05

enum {
    GE_PRIM_POINTS = 0,
    GE_PRIM_LINES = 1,
    GE_PRIM_LINE_STRIP = 2,
    GE_PRIM_TRIANGLES = 3,
    GE_PRIM_TRIANGLE_STRIP = 4,
    GE_PRIM_TRIANGLE_FAN = 5,
    GE_PRIM_RECTANGLES = 6,
    GE_PRIM_REPEAT = 7,
};

enum {
    GE_LIGHT_TYPE_DIRECTIONAL = 0x00,
    GE_LIGHT_TYPE_POINT = 0x01,
    GE_LIGHT_TYPE_SPOT = 0x02,
};

enum {
    GE_LIGHT_KIND_DIFFUSE_ONLY = 0x00,
    GE_LIGHT_KIND_DIFFUSE_AND_SPECULAR = 0x01,
    GE_LIGHT_KIND_POWERED_DIFFUSE_AND_SPECULAR = 0x02,
};

#define MAX_PATCH_DIVS 64

#define ATST_NEVER_PASS_PIXEL 0
#define ATST_ALWAYS_PASS_PIXEL 1
#define ATST_PASS_PIXEL_IF_MATCHES 2
#define ATST_PASS_PIXEL_IF_DIFFERS 3
#define ATST_PASS_PIXEL_IF_LESS 4
#define ATST_PASS_PIXEL_IF_LESS_OR_EQUAL 5
#define ATST_PASS_PIXEL_IF_GREATER 6
#define ATST_PASS_PIXEL_IF_GREATER_OR_EQUAL 7

#define  TPSM_PIXEL_STORAGE_MODE_16BIT_BGR5650 0
#define  TPSM_PIXEL_STORAGE_MODE_16BIT_ABGR5551 1
#define  TPSM_PIXEL_STORAGE_MODE_16BIT_ABGR4444 2
#define  TPSM_PIXEL_STORAGE_MODE_32BIT_ABGR8888 3
#define  TPSM_PIXEL_STORAGE_MODE_4BIT_INDEXED 4
#define  TPSM_PIXEL_STORAGE_MODE_8BIT_INDEXED 5
#define  TPSM_PIXEL_STORAGE_MODE_16BIT_INDEXED 6
#define  TPSM_PIXEL_STORAGE_MODE_32BIT_INDEXED 7
#define  TPSM_PIXEL_STORAGE_MODE_DXT1 8
#define  TPSM_PIXEL_STORAGE_MODE_DXT3 9
#define  TPSM_PIXEL_STORAGE_MODE_DXT5 10

#define CMODE_FORMAT_16BIT_BGR5650 0
#define CMODE_FORMAT_16BIT_ABGR5551 1
#define CMODE_FORMAT_16BIT_ABGR4444 2
#define CMODE_FORMAT_32BIT_ABGR8888 3


#define TFLT_NEAREST 0
#define TFLT_LINEAR 1
#define TFLT_UNKNOW1 2
#define TFLT_UNKNOW2 3
#define TFLT_NEAREST_MIPMAP_NEAREST 4
#define TFLT_LINEAR_MIPMAP_NEAREST 5
#define TFLT_NEAREST_MIPMAP_LINEAR 6
#define TFLT_LINEAR_MIPMAP_LINEAR 7

#define TWRAP_WRAP_MODE_REPEAT 0
#define TWRAP_WRAP_MODE_CLAMP 1

#define TFUNC_FRAGMENT_DOUBLE_ENABLE_COLOR_UNTOUCHED 0
#define TFUNC_FRAGMENT_DOUBLE_ENABLE_COLOR_DOUBLED 1
#define TFUNC_FRAGMENT_DOUBLE_TEXTURE_COLOR_ALPHA_IS_IGNORED 0
#define TFUNC_FRAGMENT_DOUBLE_TEXTURE_COLOR_ALPHA_IS_READ 1
#define TFUNC_FRAGMENT_DOUBLE_TEXTURE_EFECT_MODULATE 0
#define TFUNC_FRAGMENT_DOUBLE_TEXTURE_EFECT_DECAL 1
#define TFUNC_FRAGMENT_DOUBLE_TEXTURE_EFECT_BLEND 2
#define TFUNC_FRAGMENT_DOUBLE_TEXTURE_EFECT_REPLACE 3
#define TFUNC_FRAGMENT_DOUBLE_TEXTURE_EFECT_ADD 4
#define TFUNC_FRAGMENT_DOUBLE_TEXTURE_EFECT_UNKNOW1 5
#define TFUNC_FRAGMENT_DOUBLE_TEXTURE_EFECT_UNKNOW2 6
#define TFUNC_FRAGMENT_DOUBLE_TEXTURE_EFECT_UNKNOW3 7