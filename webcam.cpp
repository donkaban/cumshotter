#include "webcam.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <chrono>
#include <sstream>
#include <fcntl.h>           
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

#define CLEAR(x) memset(&(x), 0, sizeof(x))
static void TERMINATE(const std::string &mes)
{
    throw std::runtime_error("error: " + mes + "; --> " +strerror(errno));
}
static int IO_CTL(int fh, int request, void *arg)
{
    int ret;
    do 
    {
        ret = ioctl(fh, request, arg);
    } 
    while (ret < 0 && EINTR == errno);
    return ret;
}

static std::string generateName()
{
    auto p = std::chrono::high_resolution_clock::now();
    std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(p.time_since_epoch());    std::stringstream res;
    std::chrono::seconds s = std::chrono::duration_cast<std::chrono::seconds>(ms);
    std::time_t t = s.count();
    res << std::ctime(&t) << ms.count() % 1000 << ".jpg";    
    return res.str();     
}

webcam::webcam(uint w, uint h, const std::string & device) : 
    width(w),
    height(h),
    device(device)
   
{
    struct stat st;
    if(stat(device.c_str(), &st) < 0) TERMINATE("can't find " + device);
    if(!S_ISCHR(st.st_mode)) TERMINATE(device + " is no device"); 
    fd = open(device.c_str(), O_RDWR | O_NONBLOCK, 0);
    if (fd < 0) TERMINATE ("can't open " + device);
    
    v4l2_capability cap;
    v4l2_cropcap    cropcap;
    v4l2_crop       crop;
    v4l2_format     fmt;

    if (IO_CTL(fd, VIDIOC_QUERYCAP, &cap) < 0)        TERMINATE("QUERYCAR request error");
    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) TERMINATE(device + " is not capture device");
    if (!(cap.capabilities & V4L2_CAP_STREAMING))     TERMINATE(device + " does not support streaming i/o");

    CLEAR(cropcap);
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (IO_CTL(fd, VIDIOC_CROPCAP, &cropcap) == 0) 
    {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect;
        IO_CTL(fd, VIDIOC_S_CROP, &crop);
    }    
    CLEAR(fmt);
    fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = width;
    fmt.fmt.pix.height      = height;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    fmt.fmt.pix.field       = V4L2_FIELD_ANY;
    if (IO_CTL(fd, VIDIOC_S_FMT, &fmt) < 0) TERMINATE("can't set capture format");

    v4l2_requestbuffers req;
    CLEAR(req);

    req.count   = 4;
    req.type    = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory  = V4L2_MEMORY_MMAP;

    if(IO_CTL(fd, VIDIOC_REQBUFS, &req) < 0) TERMINATE("memory mapping not supported"); 
    if(req.count < 2) TERMINATE("insufficient buffer memory ");

    buffers = static_cast<buffer *>(calloc(req.count, sizeof(*buffers)));
    if(!buffers) TERMINATE("out of memory!");
    for(n_buffers = 0; n_buffers < req.count; ++n_buffers) 
    {
        v4l2_buffer buf;
        CLEAR(buf);

        buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory      = V4L2_MEMORY_MMAP;
        buf.index       = n_buffers;

        if(IO_CTL(fd, VIDIOC_QUERYBUF, &buf) < 0) TERMINATE("can't create buffers");
        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start = mmap(NULL,buf.length,PROT_READ|PROT_WRITE,MAP_SHARED,fd, buf.m.offset);
        if (MAP_FAILED == buffers[n_buffers].start) TERMINATE("mmap");
    }
}

webcam::~webcam()
{
    for (uint i = 0; i < n_buffers; ++i)
        if (munmap(buffers[i].start, buffers[i].length) < 0) TERMINATE("can't unmap memory");
    free(buffers);
    if(close(fd) < 0) TERMINATE("close device problem");
}

void webcam::saveImage(const void *buff)
{
    std::string name = shotsPath + generateName();
    FILE * outfile = std::fopen(name.c_str(), "wb");
    if(!outfile) 
        throw std::runtime_error("can't create file " + name);
    fwrite (buff , 1 , width*height ,outfile);
    std::fclose(outfile);
}

void webcam::readFrame()
{
    struct v4l2_buffer buf;
    CLEAR(buf);
    buf.type    = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory  = V4L2_MEMORY_MMAP;
    if (IO_CTL(fd, VIDIOC_DQBUF, &buf) < 0 && errno != EAGAIN) TERMINATE("read frame problem");
    assert(buf.index < n_buffers);
    saveImage(buffers[buf.index].start);
    if (IO_CTL(fd, VIDIOC_QBUF, &buf) < 0) TERMINATE("VIDIOC_QBUF");
}

void webcam::shot(uint count)
{
    if(!isEnabled) return;
    while (count-- > 0) 
    {
        fd_set  fds;
        timeval tv;
        
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        tv.tv_sec  = 2;
        tv.tv_usec = 0;
        int r = select(fd + 1, &fds, NULL, NULL, &tv);
        if (r < 0) 
        {
            if (errno == EINTR) continue;
            TERMINATE("select");
        }
        if (r == 0) TERMINATE ("select timeout error"); 
        readFrame();
    }    
}

void webcam::cameraOff()
{
    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(IO_CTL(fd, VIDIOC_STREAMOFF, &type) < 0) TERMINATE("can't stop streaming");
    isEnabled = false;
}

void webcam::cameraOn()
{
    enum v4l2_buf_type type;
    for (uint i = 0; i < n_buffers; ++i) 
    {
        v4l2_buffer buf;
        CLEAR(buf);
        buf.type    = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory  = V4L2_MEMORY_MMAP;
        buf.index   = i;
        if(IO_CTL(fd, VIDIOC_QBUF, &buf) < 0) TERMINATE("buffer exchange error");
    }
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(IO_CTL(fd, VIDIOC_STREAMON, &type) < 0) TERMINATE("can't start streaming");
    isEnabled = true;
}




