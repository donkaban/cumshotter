#include <iostream>
#include <stdexcept>

struct buffer 
{
    void   *start;
    size_t  length;
};


class webcam
{
private:
    int     fd = -1;
    buffer *buffers;
    uint    n_buffers;
    uint width;
    uint height;
    bool isEnabled = false;

    std::string device;
    std::string shotsPath = "./shots/";
      
	void saveImage(const void *buff);
    void readFrame();
    
public:
    webcam(uint, uint,const std::string & device = "/dev/video0"); 
    ~webcam(); 
    void shot(uint count = 1);
    void cameraOn();
    void cameraOff();
};

extern void saveJPEG(unsigned char * , const std::string &, int, int);

