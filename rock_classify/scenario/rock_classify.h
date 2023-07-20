#ifndef ROCK_CLASSIFY_H
#define ROCK_CLASSIFY_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "hi_comm_video.h"
#include "osd_img.h"

#if __cplusplus
extern "C"
{
#endif

#define TINY_BUF_SIZE 64
#define SMALL_BUF_SIZE 128
#define NORM_BUF_SIZE 256
#define LARGE_BUF_SIZE 1024
#define HUGE_BUF_SIZE 9120

#define ARGB1555_RED 0xFC00     // 1 11111 00000 00000
#define ARGB1555_GREEN 0x83E0   // 1 00000 11111 00000
#define ARGB1555_BLUE 0x801F    // 1 00000 00000 11111
#define ARGB1555_YELLOW 0xFFE0  // 1 11111 11111 00000
#define ARGB1555_YELLOW2 0xFF00 // 1 11111 11111 00000
#define ARGB1555_WHITE 0xFFFF   // 1 11111 11111 11111
#define ARGB1555_BLACK 0x8000   // 1 00000 00000 00000

    HI_S32 RockClassifyLoadModel(uintptr_t *model, OsdSet *osds);

    HI_S32 RockClassifyUnloadModel(uintptr_t model);

    HI_S32 RockClassify(uintptr_t model, VIDEO_FRAME_INFO_S *resFrm);

#ifdef __cplusplus
}
#endif
#endif