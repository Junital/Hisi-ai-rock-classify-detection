#include <iostream>
#include "unistd.h"
#include "sdk.h"
#include "sample_media_ai.h"
#include "sample_media_opencv.h"

using namespace std;

/*
 * 函数：rock detect主函数
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
    return s32Ret;
}
