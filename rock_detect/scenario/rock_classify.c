#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "sample_comm_nnie.h"
#include "sample_media_ai.h"
#include "ai_infer_process.h"
#include "yolov2_rock_detect.h"
#include "vgs_img.h"
#include "ive_img.h"
#include "misc_util.h"
#include "hisignalling.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* End of #ifdef __cplusplus */

#define ROCK_FRM_WIDTH 640
#define ROCK_FRM_HEIGHT 384
#define DETECT_OBJ_MAX 32
#define RET_NUM_MAX 4
#define DRAW_RETC_THICK 2 // Draw the width of the line
#define WIDTH_LIMIT 32
#define HEIGHT_LIMIT 32
#define IMAGE_WIDTH 224 // The resolution of the model IMAGE sent to the classification is 224*224
#define IMAGE_HEIGHT 224
#define MODEL_FILE_GESTURE "/userdata/models/rock_classify/resnet18-rock-classify_inst.wk" // darknet framework wk model

    static int biggestBoxIndex;
    static IVE_IMAGE_S img;
    static DetectObjInfo objs[DETECT_OBJ_MAX] = {0};
    static RectBox boxs[DETECT_OBJ_MAX] = {0};
    static RectBox objBoxs[DETECT_OBJ_MAX] = {0};
    static RectBox remainingBoxs[DETECT_OBJ_MAX] = {0};
    static RectBox cnnBoxs[DETECT_OBJ_MAX] = {0}; // Store the results of the classification network
    static RecogNumInfo numInfo[RET_NUM_MAX] = {0};
    static IVE_IMAGE_S imgIn;
    static IVE_IMAGE_S imgDst;
    static VIDEO_FRAME_INFO_S frmIn;
    static VIDEO_FRAME_INFO_S frmDst;
    int uartFd = 0;

    /*
     * 加载岩石检测和分类模型
     * Load rock detect and classify model
     */
    HI_S32 Yolo2RockDetectResnetClassifyLoad(uintptr_t *model)
    {
        SAMPLE_SVP_NNIE_CFG_S *self = NULL;
        HI_S32 ret;

        ret = CnnCreate(&self, MODEL_FILE_GESTURE);
        *model = ret < 0 ? 0 : (uintptr_t)self;
        RockDetectInit(); // Initialize the rock detection model
        SAMPLE_PRT("Load rock detect claasify model success\n");
        /*
         * Uart串口初始化
         * Uart open init
         */
        uartFd = UartOpenInit();
        if (uartFd < 0)
        {
            printf("uart1 open failed\r\n");
        }
        else
        {
            printf("uart1 open successed\r\n");
        }
        return ret;
    }

    /*
     * 卸载岩石检测和分类模型
     * Unload rock detect and classify model
     */
    HI_S32 Yolo2RockDetectResnetClassifyUnload(uintptr_t model)
    {
        CnnDestroy((SAMPLE_SVP_NNIE_CFG_S *)model);
        RockDetectExit(); // Uninitialize the rock detection model
        close(uartFd);
        SAMPLE_PRT("Unload rock detect claasify model success\n");

        return 0;
    }

    /*
     * 获得最大的岩石
     * Get the maximum rock
     */
    static HI_S32 GetBiggestRockIndex(RectBox boxs[], int detectNum)
    {
        HI_S32 rockIndex = 0;
        HI_S32 biggestBoxIndex = rockIndex;
        HI_S32 biggestBoxWidth = boxs[rockIndex].xmax - boxs[rockIndex].xmin + 1;
        HI_S32 biggestBoxHeight = boxs[rockIndex].ymax - boxs[rockIndex].ymin + 1;
        HI_S32 biggestBoxArea = biggestBoxWidth * biggestBoxHeight;

        for (rockIndex = 1; rockIndex < detectNum; rockIndex++)
        {
            HI_S32 boxWidth = boxs[rockIndex].xmax - boxs[rockIndex].xmin + 1;
            HI_S32 boxHeight = boxs[rockIndex].ymax - boxs[rockIndex].ymin + 1;
            HI_S32 boxArea = boxWidth * boxHeight;
            if (biggestBoxArea < boxArea)
            {
                biggestBoxArea = boxArea;
                biggestBoxIndex = rockIndex;
            }
            biggestBoxWidth = boxs[biggestBoxIndex].xmax - boxs[biggestBoxIndex].xmin + 1;
            biggestBoxHeight = boxs[biggestBoxIndex].ymax - boxs[biggestBoxIndex].ymin + 1;
        }

        if ((biggestBoxWidth == 1) || (biggestBoxHeight == 1) || (detectNum == 0))
        {
            biggestBoxIndex = -1;
        }

        return biggestBoxIndex;
    }

    /*
     * 岩石识别信息
     * rock gesture recognition info
     */
    static void RockDetectFlag(const RecogNumInfo resBuf)
    {
        HI_CHAR *rockName = NULL;
        switch (resBuf.num)
        {
        case 0u:
            rockName = "Igneous->Basalt";
            UartSendRead(uartFd, IgneousBasalt); // 火成岩玄武岩
            SAMPLE_PRT("----rock name----:%s\n", rockName);
            break;
        case 1u:
            rockName = "Igneous->Granite";
            UartSendRead(uartFd, IgneousGranite); // 火成岩花岗岩
            SAMPLE_PRT("----rock name----:%s\n", rockName);
            break;
        case 2u:
            rockName = "Metamorphic->Marble";
            UartSendRead(uartFd, MetamorphicMarble); // 变质岩大理石
            SAMPLE_PRT("----rock name----:%s\n", rockName);
            break;
        case 3u:
            rockName = "Metamorphic->Quartzite";
            UartSendRead(uartFd, MetamorphicQuartzite); // 变质岩石英岩
            SAMPLE_PRT("----rock name----:%s\n", rockName);
            break;
        case 4u:
            rockName = "Sedimentary->Coal";
            UartSendRead(uartFd, SedimentaryCoal); // 沉积岩煤
            SAMPLE_PRT("----rock name----:%s\n", rockName);
            break;
        case 5u:
            rockName = "Sedimentary->Limestone";
            UartSendRead(uartFd, SedimentaryLimestone); // 沉积岩石灰岩
            SAMPLE_PRT("----rock name----:%s\n", rockName);
            break;
        case 6u:
            rockName = "Sedimentary->Sandstone";
            UartSendRead(uartFd, SedimentarySandstone); // 沉积岩 砂岩
            SAMPLE_PRT("----rock name----:%s\n", rockName);
            break;
        default:
            rockName = "rock others";
            UartSendRead(uartFd, Invalidrock); // 无效值
            SAMPLE_PRT("----rock name----:%s\n", rockName);
            break;
        }
        SAMPLE_PRT("rock rock success\n");
    }

    /*
     * 岩石检测和分类推理
     * rock detect and classify calculation
     */
    HI_S32 Yolo2RockDetectResnetClassifyCal(uintptr_t model, VIDEO_FRAME_INFO_S *srcFrm, VIDEO_FRAME_INFO_S *dstFrm)
    {
        SAMPLE_SVP_NNIE_CFG_S *self = (SAMPLE_SVP_NNIE_CFG_S *)model;
        HI_S32 resLen = 0;
        int objNum;
        int ret;
        int num = 0;

        ret = FrmToOrigImg((VIDEO_FRAME_INFO_S *)srcFrm, &img);
        SAMPLE_CHECK_EXPR_RET(ret != HI_SUCCESS, ret, "rock detect for YUV Frm to Img FAIL, ret=%#x\n", ret);

        objNum = RockDetectCal(&img, objs); // Send IMG to the detection net for reasoning
        for (int i = 0; i < objNum; i++)
        {
            cnnBoxs[i] = objs[i].box;
            RectBox *box = &objs[i].box;
            RectBoxTran(box, ROCK_FRM_WIDTH, ROCK_FRM_HEIGHT,
                        dstFrm->stVFrame.u32Width, dstFrm->stVFrame.u32Height);
            SAMPLE_PRT("yolo2_out: {%d, %d, %d, %d}\n", box->xmin, box->ymin, box->xmax, box->ymax);
            boxs[i] = *box;
        }
        biggestBoxIndex = GetBiggestRockIndex(boxs, objNum);
        SAMPLE_PRT("biggestBoxIndex:%d, objNum:%d\n", biggestBoxIndex, objNum);
        RectBox *box = &objs[biggestBoxIndex].box;
        UartSendReadInt(uartFd, box->xmin, box->ymin, box->xmax, box->ymax);

        /*
         * 当检测到对象时，在DSTFRM中绘制一个矩形
         * When an object is detected, a rectangle is drawn in the DSTFRM
         */
        if (biggestBoxIndex >= 0)
        {
            objBoxs[0] = boxs[biggestBoxIndex];
            MppFrmDrawRects(dstFrm, objBoxs, 1, RGB888_GREEN, DRAW_RETC_THICK); // Target rock objnum is equal to 1

            for (int j = 0; (j < objNum) && (objNum > 1); j++)
            {
                if (j != biggestBoxIndex)
                {
                    remainingBoxs[num++] = boxs[j];
                    /*
                     * 其他手objnum等于objnum -1
                     * Others rock objnum is equal to objnum -1
                     */
                    MppFrmDrawRects(dstFrm, remainingBoxs, objNum - 1, RGB888_RED, DRAW_RETC_THICK);
                }
            }

            /*
             * 裁剪出来的图像通过预处理送分类网进行推理
             * The cropped image is preprocessed and sent to the classification network for inference
             */
            ret = ImgYuvCrop(&img, &imgIn, &cnnBoxs[biggestBoxIndex]);
            SAMPLE_CHECK_EXPR_RET(ret < 0, ret, "ImgYuvCrop FAIL, ret=%#x\n", ret);

            if ((imgIn.u32Width >= WIDTH_LIMIT) && (imgIn.u32Height >= HEIGHT_LIMIT))
            {
                COMPRESS_MODE_E enCompressMode = srcFrm->stVFrame.enCompressMode;
                ret = OrigImgToFrm(&imgIn, &frmIn);
                frmIn.stVFrame.enCompressMode = enCompressMode;
                SAMPLE_PRT("crop u32Width = %d, img.u32Height = %d\n", imgIn.u32Width, imgIn.u32Height);
                ret = MppFrmResize(&frmIn, &frmDst, IMAGE_WIDTH, IMAGE_HEIGHT);
                ret = FrmToOrigImg(&frmDst, &imgDst);
                ret = CnnCalImg(self, &imgDst, numInfo, sizeof(numInfo) / sizeof((numInfo)[0]), &resLen);
                SAMPLE_CHECK_EXPR_RET(ret < 0, ret, "CnnCalImg FAIL, ret=%#x\n", ret);
                HI_ASSERT(resLen <= sizeof(numInfo) / sizeof(numInfo[0]));
                RockDetectFlag(numInfo[0]);
                MppFrmDestroy(&frmDst);
            }
            IveImgDestroy(&imgIn);
        }

        return ret;
    }

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
