#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "sample_comm_nnie.h"
#include "ai_infer_process.h"
#include "sample_media_ai.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* End of #ifdef __cplusplus */

#define MODEL_FILE_ROCK "/userdata/models/rock_detect/rock_detect_inst_2.wk" // darknet framework wk model
#define PIRIOD_NUM_MAX 49                                                    // Logs are printed when the number of targets is detected
#define DETECT_OBJ_MAX 32                                                    // detect max obj

    static uintptr_t g_rockModel = 0;

    static HI_S32 Yolo2FdLoad(uintptr_t *model)
    {
        SAMPLE_SVP_NNIE_CFG_S *self = NULL;
        HI_S32 ret;

        ret = Yolo2Create(&self, MODEL_FILE_ROCK);
        *model = ret < 0 ? 0 : (uintptr_t)self;
        SAMPLE_PRT("Yolo2FdLoad ret:%d\n", ret);

        return ret;
    }

    HI_S32 RockDetectInit()
    {
        return Yolo2FdLoad(&g_rockModel);
    }

    static HI_S32 Yolo2FdUnload(uintptr_t model)
    {
        Yolo2Destory((SAMPLE_SVP_NNIE_CFG_S *)model);
        return 0;
    }

    HI_S32 RockDetectExit()
    {
        return Yolo2FdUnload(g_rockModel);
    }

    static HI_S32 RockDetect(uintptr_t model, IVE_IMAGE_S *srcYuv, DetectObjInfo boxs[])
    {
        SAMPLE_SVP_NNIE_CFG_S *self = (SAMPLE_SVP_NNIE_CFG_S *)model;
        int objNum;
        int ret = Yolo2CalImg(self, srcYuv, boxs, DETECT_OBJ_MAX, &objNum);
        if (ret < 0)
        {
            SAMPLE_PRT("Rock detect Yolo2CalImg FAIL, for cal FAIL, ret:%d\n", ret);
            return ret;
        }

        return objNum;
    }

    HI_S32 RockDetectCal(IVE_IMAGE_S *srcYuv, DetectObjInfo resArr[])
    {
        int ret = RockDetect(g_rockModel, srcYuv, resArr);
        return ret;
    }

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
