/*
 * Copyright (c) 2022 HiSilicon (Shanghai) Technologies CO., LIMITED.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>

#include "hi_mipi_tx.h"
#include "sdk.h"
#include "sample_comm.h"
#include "ai_infer_process.h"
#include "vgs_img.h"
#include "base_interface.h"
#include "posix_help.h"
#include "sample_media_ai.h"
#include "sample_media_opencv.h"

using namespace std;

static HI_BOOL s_bOpenCVProcessStopSignal = HI_FALSE;
static pthread_t g_openCVProcessThread = 0;
static int g_opencv = 0;
static AicMediaInfo g_aicTennisMediaInfo = {0};
static AiPlugLib g_tennisWorkPlug = {0};
static HI_CHAR tennisDetectThreadName[16] = {0};

/*
 * 设置VI设备信息
 * Set VI device information
 */
