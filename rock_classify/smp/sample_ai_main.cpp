/*
 * Copyright (c) 2022 HiSilicon (Shanghai) Technologies CO., LIMITED.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * 该cpp文件为ai sample的主函数文件，通过编译ai sample会生成一个可执行文件，可通过
 * ./可执行文件+index方式可以选择运行相关场景。index 0代表运行垃圾分类，index 1
 * 代表运行手部检测+手势识别，index 2代表网球检测。可通过enter键进行退出。
 *
 * The cpp file is the main function file of the ai sample.
 * After compiling the ai sample, an executable file will be generated.
 * You can choose to run related scenarios through the ./executable file+index method.
 * Index 0 means running trash classification, index 1 means running hand detection + gesture recognition,
 * index 2 means tennis ball detection. It can be exited by pressing the enter key.
 */

#include <iostream>
#include "unistd.h"
#include "sdk.h"
#include "sample_media_ai.h"
#include "sample_media_opencv.h"

using namespace std;

/*
 * 函数：ai sample主函数
 * function : ai sample main function
 */
int main(int argc, char *argv[])
{
    HI_S32 s32Ret = HI_FAILURE;
    sample_media_opencv mediaOpencv;

    sdk_init();
    /*
     * MIPI为GPIO55，开启液晶屏背光
     * MIPI is GPIO55, Turn on the backlight of the LCD screen
     */
    system("cd /sys/class/gpio/;echo 55 > export;echo out > gpio55/direction;echo 1 > gpio55/value");

    SAMPLE_MEDIA_ROCK_CLASSIFY();

    sdk_exit();
    SAMPLE_PRT("\nsdk exit success\n");

    s32Ret = HI_SUCCESS;

    return s32Ret;
}
