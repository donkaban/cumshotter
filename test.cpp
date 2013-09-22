
#include "webcam.h"

int main()
{
    webcam w(640,480,"/dev/video0");
    w.cameraOn();
    w.shot(30);
    w.cameraOff();
    return 0;
}
