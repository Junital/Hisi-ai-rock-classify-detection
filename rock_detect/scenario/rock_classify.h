#ifndef ROCK_CLASSIFY_H
#define ROCK_CLASSIFY_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "hi_comm_video.h"

#if __cplusplus
extern "C" {
#endif

/*
 * 加载岩石检测和分类模型
 * Load Rock detect and classify model
 */
HI_S32 Yolo2RockDetectResnetClassifyLoad(uintptr_t* model);

/*
 * 卸载岩石检测和分类模型
 * Unload rock detect and classify model
 */
HI_S32 Yolo2RockDetectResnetClassifyUnload(uintptr_t model);

/*
 * 岩石检测和分类推理
 * rock detect and classify calculation
 */
HI_S32 Yolo2RockDetectResnetClassifyCal(uintptr_t model, VIDEO_FRAME_INFO_S *srcFrm, VIDEO_FRAME_INFO_S *dstFrm);

#ifdef __cplusplus
}
#endif
#endif
