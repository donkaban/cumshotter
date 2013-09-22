#include "webcam.h"

#include <cstdio>
#include <jpeglib.h>

void saveJPEG(unsigned char * buffer, const std::string &filename, int w, int h)
{
    FILE * outfile = std::fopen(filename.c_str(), "wb");
    if(!outfile) 
        throw std::runtime_error("can't create file " + filename);
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, outfile);
    cinfo.image_width       = w;
    cinfo.image_height      = h;
    cinfo.input_components  = 3;
    cinfo.in_color_space    = JCS_RGB;
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, 75, true);
    jpeg_start_compress(&cinfo, true);
    JSAMPROW row_pointer[1];
    auto row_stride = cinfo.image_width * 3;
    while (cinfo.next_scanline < cinfo.image_height) 
    {
        row_pointer[0] = static_cast<JSAMPLE *>(buffer + cinfo.next_scanline * row_stride);
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
}



#ifdef TESTING

int main(void) 
{

    int width  = 800;
    int height = 600;
    unsigned char *scr=static_cast<unsigned char *>(malloc(width*height*3));

    for (int i=0;i<widt * height)
        for (int j=0;j<height;j++) 
        {
            unsigned char *p=scr+j*width*3+i*3;
            *p=i+j;p++;
            *p=i-j;p++;
            *p=i*j;
        }

    saveJPEG(scr,"test2.jpg", width, height);
    free(scr);
    return 0;
  }

#endif