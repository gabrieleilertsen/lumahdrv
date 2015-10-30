#ifndef LUMAFRAME_H
#define LUMAFRAME_H

#include <cstddef>

struct LumaFrame
{
    LumaFrame(unsigned int w = 0, unsigned int h = 0, unsigned int c = 3) : 
        height(h), width(w), channels(c), buffer(NULL)
    {
        if (height && width && channels)
            init();
    }
    
    ~LumaFrame() { clear(); }
    
    void clear()
    {
        if (buffer != NULL)
        {
            delete[] buffer;
            buffer = NULL;
        }
    }
    
    bool init()
    {
        if (!height && !width && !channels)
            return 0;
            
        clear();
        
        buffer = new float[channels*height*width];
        
        return 1;
    }
    
    float* getChannel(unsigned int c)
    {
        return buffer + c*height*width;
    }
    
    unsigned int height, width, channels;
    float *buffer;
};

#endif //LUMAFRAME_H
