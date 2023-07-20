#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/prctl.h>

#include "sample_comm_nnie.h"
#include "sample_media_ai.h"
#include "ai_infer_process.h"
#include "vgs_img.h"
#include "ive_img.h"
#include "posix_help.h"
#include "audio_aac_adp.h"
#include "base_interface.h"
#include "osd_img.h"
#include "rock_classify.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* End of #ifdef __cplusplus */

#define MODEL_FILE_PATH "/userdata/models/rock_classify/resnet18-rock-classify_inst.wk"
#define SCORE_MAX 4096
#define DETECT_OBJ_MAX 32
#define RET_NUM_MAX 4
#define THRESH_MIN 30

#define FRM_WIDTH 256
#define FRM_HEIGHT 256
#define TXT_BEGX 20
#define TXT_BEGY 20

    static int g_num = 108;
    static int g_count = 0;

#define AUDIO_CASE_TWO 2
#define AUDIO_SCORE 80
#define AUDIO_FRAME 14

#define MULTIPLE_OF_EXPANSION 100
#define UNKOWN_ROCK 7
#define BUFFER_SIZE 16
#define MIN_OF_BOX 16
#define MAX_OF_BOX 240

    static HI_BOOL g_bAudioProcessStopSignal = HI_FALSE;
    static pthread_t g_audioProcessThread = 0;
    static OsdSet *g_osdsRock = NULL;
    static HI_S32 g_osd0Rock = -1;

    static SkPair g_stmChn = {
        .in = -1,
        .out = -1};

    static HI_VOID PlayAudio(const RecogNumInfo items)
    {
        if (g_count < AUDIO_FRAME)
        {
            g_count++;
            return;
        }

        const RecogNumInfo *item = &items;
        uint32_t score = item->score * MULTIPLE_OF_EXPANSION / SCORE_MAX;
        if ((score > AUDIO_SCORE) && (g_num != item->num))
        {
            g_num = item->num;
            if (g_num != UNKOWN_ROCK)
            {
                AudioTest(g_num, -1);
            }
        }
        g_count = 0;
    }

    static HI_VOID *GetAudioFileName(HI_VOID *arg)
    {
        RecogNumInfo resBuf = {0};
        int ret;

        while (g_bAudioProcessStopSignal == false)
        {
            ret = FdReadMsg(g_stmChn.in, &resBuf, sizeof(RecogNumInfo));
            if (ret == sizeof(RecogNumInfo))
            {
                PlayAudio(resBuf);
            }
        }

        return NULL;
    }

    HI_S32 RockClassifyLoadModel(uintptr_t *model, OsdSet *osds)
    {
        SAMPLE_SVP_NNIE_CFG_S *self = NULL;
        HI_S32 ret;
        HI_CHAR audioThreadName[BUFFER_SIZE] = {0};

        ret = OsdLibInit();
        HI_ASSERT(ret == HI_SUCCESS);

        g_osdsRock = osds;
        HI_ASSERT(g_osdsRock);
        g_osd0Rock = OsdsCreateRgn(g_osdsRock);
        HI_ASSERT(g_osd0Rock >= 0);

        ret = CnnCreate(&self, MODEL_FILE_PATH);
        *model = ret < 0 ? 0 : (uintptr_t)self;
        SAMPLE_PRT("load rock classify model, ret:%d\n", ret);

        if (GetCfgBool("audio_player:support_audio", true))
        {
            ret = SkPairCreate(&g_stmChn);
            HI_ASSERT(ret == 0);
            if (snprintf_s(audioThreadName, BUFFER_SIZE, BUFFER_SIZE - 1, "AudioProcess") < 0)
            {
                HI_ASSERT(0);
            }
            prctl(PR_SET_NAME, (unsigned long)audioThreadName, 0, 0, 0);
            ret = pthread_create(&g_audioProcessThread, NULL, GetAudioFileName, NULL);
            if (ret != 0)
            {
                SAMPLE_PRT("audio proccess thread creat fail:%s\n", strerror(ret));
                return ret;
            }
        }

        return ret;
    }

    HI_S32 RockClassifyUnloadModel(uintptr_t model)
    {
        CnnDestroy((SAMPLE_SVP_NNIE_CFG_S *)model);
        SAMPLE_PRT("unload rock classify model success\n");
        OsdsClear(g_osdsRock);

        if (GetCfgBool("audio_player:support_audio", true))
        {
            SkPairDestroy(&g_stmChn);
            SAMPLE_PRT("SkPairDestroy success\n");
            g_bAudioProcessStopSignal = HI_TRUE;
            pthread_join(g_audioProcessThread, NULL);
            g_audioProcessThread = 0;
        }

        return HI_SUCCESS;
    }

    static HI_S32 RockClassifyFlag(const RecogNumInfo items[], HI_S32 itemNum, HI_CHAR *buf, HI_S32 size)
    {
        HI_S32 offset = 0;
        HI_CHAR *rockName = NULL;

        offset += snprintf_s(buf + offset, size - offset, size - offset - 1, "rock classify: {");
        for (HI_U32 i = 0; i < itemNum; i++)
        {
            const RecogNumInfo *item = &items[i];
            uint32_t score = item->score * HI_PER_BASE / SCORE_MAX;
            if (score < THRESH_MIN)
            {
                break;
            }
            SAMPLE_PRT("----rock item flag----num:%d, score:%d\n", item->num, score);
            switch (item->num)
            {
            case 0u:
                rockName = "Igneous->Basalt";
                break;
            case 1u:
                rockName = "Igneous->Granite";
                break;
            case 2u:
                rockName = "Metamorphic->Marble";
                break;
            case 3u:
                rockName = "Metamorphic->Quartzite";
                break;
            case 4u:
                rockName = "Sedimentary->Coal";
                break;
            case 5u:
                rockName = "Sedimentary->Limestone";
                break;
            case 6u:
                rockName = "Sedimentary->Sandstone";
                break;
            default:
                rockName = "Unkown Rock";
                break;
            }
            offset += snprintf_s(buf + offset, size - offset, size - offset - 1,
                                 "%s%s %u:%u%%", (i == 0 ? " " : ", "), rockName, (int)item->num, (int)score);
            HI_ASSERT(offset < size);
        }
        offset += snprintf_s(buf + offset, size - offset, size - offset - 1, " }");
        HI_ASSERT(offset < size);
        return HI_SUCCESS;
    }

    HI_S32 RockClassifyCal(uintptr_t model, VIDEO_FRAME_INFO_S *srcFrm, VIDEO_FRAME_INFO_S *resFrm)
    {
        SAMPLE_PRT("begin RockClassifyCal\n");
        SAMPLE_SVP_NNIE_CFG_S *self = (SAMPLE_SVP_NNIE_CFG_S *)model; // reference to SDK sample_comm_nnie.h Line 99
        IVE_IMAGE_S img;                                              // referece to SDK hi_comm_ive.h Line 143
        RectBox cnnBoxs[DETECT_OBJ_MAX] = {0};
        VIDEO_FRAME_INFO_S resizeFrm; // Meet the input frame of the plug
        static HI_CHAR prevOsd[NORM_BUF_SIZE] = "";
        HI_CHAR osdBuf[NORM_BUF_SIZE] = "";

        RecogNumInfo resBuf[RET_NUM_MAX] = {0};
        HI_S32 resLen = 0;
        HI_S32 ret;
        IVE_IMAGE_S imgIn;

        cnnBoxs[0].xmin = MIN_OF_BOX;
        cnnBoxs[0].xmax = MAX_OF_BOX;
        cnnBoxs[0].ymin = MIN_OF_BOX;
        cnnBoxs[0].ymax = MAX_OF_BOX;

        ret = MppFrmResize(srcFrm, &resizeFrm, FRM_WIDTH, FRM_HEIGHT); // resize 256*256
        SAMPLE_CHECK_EXPR_RET(ret != HI_SUCCESS, ret, "for resize FAIL, ret=%x\n", ret);

        ret = FrmToOrigImg(&resizeFrm, &img);
        SAMPLE_CHECK_EXPR_RET(ret != HI_SUCCESS, ret, "for Frm2Img FAIL, ret=%x\n", ret);

        ret = ImgYuvCrop(&img, &imgIn, &cnnBoxs[0]); // Crop the image to classfication network
        SAMPLE_CHECK_EXPR_RET(ret < 0, ret, "ImgYuvCrop FAIL, ret=%x\n", ret);

        ret = CnnCalImg(self, &imgIn, resBuf, sizeof(resBuf) / sizeof((resBuf)[0]), &resLen);
        SAMPLE_CHECK_EXPR_RET(ret < 0, ret, "cnn cal FAIL, ret=%x\n", ret);

        HI_ASSERT(resLen <= sizeof(resBuf) / sizeof(resBuf[0]));
        ret = RockClassifyFlag(resBuf, resLen, osdBuf, sizeof(osdBuf));
        SAMPLE_CHECK_EXPR_RET(ret < 0, ret, "RockClassifyFlag cal FAIL, ret=%x\n", ret);

        if (GetCfgBool("audio_player:support_audio", true))
        {
            if (FdWriteMsg(g_stmChn.out, &resBuf[0], sizeof(RecogNumInfo)) != sizeof(RecogNumInfo))
            {
                SAMPLE_PRT("FdWriteMsg FAIL\n");
            }
        }

        /*
         * 仅当计算结果与之前计算发生变化时，才重新打OSD输出文字
         * Only when the calculation result changes from the previous calculation, re-print the OSD output text
         */
        if (strcmp(osdBuf, prevOsd) != 0)
        {
            HiStrxfrm(prevOsd, osdBuf, sizeof(prevOsd));
            HI_OSD_ATTR_S rgn;
            TxtRgnInit(&rgn, osdBuf, TXT_BEGX, TXT_BEGY, ARGB1555_YELLOW2); // font width and heigt use default 40
            OsdsSetRgn(g_osdsRock, g_osd0Rock, &rgn);
            /*
             * 用户向VPSS发送数据
             * User sends data to VPSS
             */
            ret = HI_MPI_VPSS_SendFrame(0, 0, srcFrm, 0);
            if (ret != HI_SUCCESS)
            {
                SAMPLE_PRT("Error(%#x), HI_MPI_VPSS_SendFrame failed!\n", ret);
            }
        }

        IveImgDestroy(&imgIn);
        MppFrmDestroy(&resizeFrm);

        return ret;
    }

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */