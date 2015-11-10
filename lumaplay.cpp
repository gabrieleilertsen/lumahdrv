/**
 * This file is part of the LumaHDRv package.
 * -----------------------------------------------------------------------------
 * Copyright (c) 2015, The LumaHDRv authors.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software 
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 * -----------------------------------------------------------------------------
 *
 * \author Gabriel Eilertsen, gabriel.eilertsen@liu.se
 *
 * \date Oct 7 2015
 */

#include <cstdio>
#include <math.h>
#include <fstream>

#include <GL/glew.h>

#ifdef __APPLE__
#include <glut.h>
#else
#include <GL/glut.h>
#endif

#include <luma_decoder.h>
#include "luma_exception.h"
#include "arg_parser.h"

#include "config.h"

const char* shader_src[] = {
#include "lumaplay_frag.h"
};

struct GlutState
{
    GlutState() : pausePlayback(0), doTmo(0), seek(0), showTimings(0), ldrSim(0),
        dispTimeTot(0.0f), totTimeTot(0.0f), exposure(1.0f), fps(-1.0f), frameDuration(40.0f), frameNr(0)
    {}
    
    bool hbd, pausePlayback, doTmo, seek, showTimings, ldrSim;
    float dispTimeTot, totTimeTot, exposure, fps, frameDuration;
    unsigned int windNr, stride[3], width[3], height[3], frameNr;
    timeval start1, start2, stop;
    
    GLfloat strideRatio;
    LumaDecoder decoder;
    
    GLuint program, texC1, texC2, texC3, texM;
} glutState;

bool getFrame()
{
    if (!glutState.decoder.run())
    {
        glutState.decoder.seekToTime(0);
        if (!glutState.decoder.run())
            return 0;
    }
    
    glutState.decoder.getFrame();
    glutState.frameDuration = glutState.decoder.getReader()->getFrameDuration();
    
    return 1;
}

int loadTexture(GLuint t)
{
    GLuint object;
    glGenTextures(1,&object);
    glBindTexture(t, object);
    glTexParameterf(t,GL_TEXTURE_MIN_FILTER,GL_LINEAR); //GL_NEAREST);
    glTexParameterf(t,GL_TEXTURE_MAG_FILTER,GL_LINEAR); //GL_NEAREST);
    glTexParameterf(t,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameterf(t,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    return object;
}

void init()
{
    glClearColor(0.0,0.0,0.0,0.0);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0,100,100,1.0,-1.0,1.0);
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    
    glutState.texC1 = loadTexture(GL_TEXTURE_2D);
    glutState.texC2 = loadTexture(GL_TEXTURE_2D);
    glutState.texC3 = loadTexture(GL_TEXTURE_2D);
    glutState.texM = loadTexture(GL_TEXTURE_1D);
    
    glBindTexture(GL_TEXTURE_1D,glutState.texM);
    glTexImage1D(GL_TEXTURE_1D,0,GL_R32F,glutState.decoder.getQuantizer()->getSize(),0,GL_RED,GL_FLOAT,glutState.decoder.getQuantizer()->getMapping());
}

bool setupShader(GLuint &shader, float gammaVal, float userScaling)
{
    shader = glCreateShader(GL_FRAGMENT_SHADER);
    
    /*
    std::ifstream t;
    char str[1000];
    snprintf(str, 999, "%s/src/dequantizer.frag", SOURCE_DIR);
    t.open(str);
    if (!t)
    {
        snprintf(str, 999, "Unable to load dequantization shader from '%s/src/dequantizer.frag'", SOURCE_DIR);
        throw LumaException(str);
    }
    t.seekg(0, std::ios::end);
    int length = t.tellg();
    t.seekg(0, std::ios::beg);
    char *buffer = new char[length];
    t.read(buffer, length);
    t.close();
    */
    
    glShaderSource(shader, 1, (const GLchar**)(&shader_src), NULL);
    glCompileShader(shader);
    
    //delete[] buffer;
    
    GLint flag;
    
    glGetShaderiv(shader, GL_COMPILE_STATUS, &flag);
    
    if (!flag)
    {
        fprintf(stderr, "Error! Failed to compile shader.\n");
        
        GLint length;
        GLchar* log;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        log = (GLchar*)malloc(length);
        glGetShaderInfoLog(shader, length, &length, log);
        fprintf(stderr, "\n%s\n", log);
        
        return 0;
    }
    
    glutState.program = glCreateProgram();
    
    glAttachShader(glutState.program, shader);
    glLinkProgram(glutState.program);
    
    glGetProgramiv(glutState.program, GL_LINK_STATUS, &flag);
    
    if (!flag)
    {
        fprintf(stderr, "Error! Failed to link program.\n");
        
        GLint length;
        GLchar* log;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        log = (GLchar*)malloc(length);
        glGetShaderInfoLog(shader, length, &length, log);
        fprintf(stderr, "\n%s\n", log);
        
        return 0;
    }
    
    glUseProgram(glutState.program);
    
    glUniform1i(glGetUniformLocation(glutState.program, "texC1"), 0);
    glUniform1i(glGetUniformLocation(glutState.program, "texC2"), 1);
    glUniform1i(glGetUniformLocation(glutState.program, "texC3"), 2);
    glUniform1i(glGetUniformLocation(glutState.program, "texM"), 3);
    
    LumaDecoderParams params = glutState.decoder.getParams();
    
    float normalization = glutState.hbd ? 65535.0f : 255.0f;
    float maxVal = glutState.decoder.getQuantizer()->getSize() / normalization;
    float maxValColor = (pow(2, params.colorBitDepth) - 1) / normalization;
    unsigned int colorSpaces[] = {LumaQuantizer::CS_LUV, LumaQuantizer::CS_RGB, LumaQuantizer::CS_YCBCR, LumaQuantizer::CS_XYZ};
    unsigned int colorSpace = params.colorSpace;
    
    glUniform1f(glGetUniformLocation(glutState.program, "maxVal"), maxVal);
    glUniform1f(glGetUniformLocation(glutState.program, "maxValColor"), maxValColor);
    glUniform1f(glGetUniformLocation(glutState.program, "exposure"), glutState.exposure);
    glUniform1f(glGetUniformLocation(glutState.program, "gamma"), gammaVal);
    glUniform1f(glGetUniformLocation(glutState.program, "scaling"), params.preScaling / userScaling);
    glUniform1iv(glGetUniformLocation(glutState.program, "colorSpaces"), 4, (GLint*)(colorSpaces));
    glUniform1i(glGetUniformLocation(glutState.program, "colorSpace"), colorSpace);
    glUniform1i(glGetUniformLocation(glutState.program, "doTmo"), glutState.doTmo);
    glUniform1i(glGetUniformLocation(glutState.program, "ldrSim"), glutState.ldrSim);
    
    return 1;
}

void display()
{
    if (glutState.frameNr > 0 && !glutState.pausePlayback)
    {
        gettimeofday(&glutState.stop, NULL);
        
        float dispTime = (glutState.stop.tv_sec-glutState.start2.tv_sec)*1000.0f + (glutState.stop.tv_usec-glutState.start2.tv_usec)/1000.0f;
        float totTime = (glutState.stop.tv_sec-glutState.start1.tv_sec)*1000.0f + (glutState.stop.tv_usec-glutState.start1.tv_usec)/1000.0f;
        
        glutState.dispTimeTot = (glutState.dispTimeTot*glutState.frameNr +  dispTime)/ (glutState.frameNr+1);
        glutState.totTimeTot = (glutState.totTimeTot*glutState.frameNr +  totTime)/ (glutState.frameNr+1);
        
        if (glutState.showTimings)
        {
            fprintf(stderr, "DISPLAY TIME: %f (avg = %f)\n", dispTime, glutState.dispTimeTot);
            fprintf(stderr, "TOTAL TIME:   %f (avg = %f), %f fps\n", totTime, glutState.totTimeTot, 1000.0f/totTime);
        }
        
        float sleepTime = glutState.frameDuration-totTime, t = 0.0f;
        
        if (glutState.fps > 0.0f)
            sleepTime = 1000.0f/glutState.fps-totTime;
        
        fprintf(stderr, "st = %f\n", sleepTime);
        while (sleepTime > t)
        {
            gettimeofday(&glutState.start1, NULL);
            t = (glutState.start1.tv_sec-glutState.stop.tv_sec)*1000.0f + (glutState.start1.tv_usec-glutState.stop.tv_usec)/1000.0f;
        }
    }
    
    gettimeofday(&glutState.start1, NULL);
    
    bool gotFrame = 0;
    
    if (!glutState.pausePlayback || glutState.seek)
    {
        gotFrame = getFrame();
        glutState.seek = false;
    }
    
    gettimeofday(&glutState.start2, NULL);
    
    glClear(GL_COLOR_BUFFER_BIT);
    
    if (gotFrame)
    {
        glutState.frameNr++;
        if (glutState.hbd)
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D,glutState.texC1);
            glTexImage2D(GL_TEXTURE_2D,0,GL_R16,glutState.stride[0]/2,glutState.height[0],0,GL_RED,GL_UNSIGNED_SHORT,(unsigned short*)(glutState.decoder.getBuffer()[0]));
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D,glutState.texC2);
            glTexImage2D(GL_TEXTURE_2D,0,GL_R16,glutState.stride[1]/2,glutState.height[1],0,GL_RED,GL_UNSIGNED_SHORT,(unsigned short*)(glutState.decoder.getBuffer()[1]));
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D,glutState.texC3);
            glTexImage2D(GL_TEXTURE_2D,0,GL_R16,glutState.stride[2]/2,glutState.height[2],0,GL_RED,GL_UNSIGNED_SHORT,(unsigned short*)(glutState.decoder.getBuffer()[2]));
        }
        else
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D,glutState.texC1);
            glTexImage2D(GL_TEXTURE_2D,0,GL_R8,glutState.stride[0],glutState.height[0],0,GL_RED,GL_UNSIGNED_BYTE,glutState.decoder.getBuffer()[0]);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D,glutState.texC2);
            glTexImage2D(GL_TEXTURE_2D,0,GL_R8,glutState.stride[1],glutState.height[1],0,GL_RED,GL_UNSIGNED_BYTE,glutState.decoder.getBuffer()[1]);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D,glutState.texC3);
            glTexImage2D(GL_TEXTURE_2D,0,GL_R8,glutState.stride[2],glutState.height[2],0,GL_RED,GL_UNSIGNED_BYTE,glutState.decoder.getBuffer()[2]);
        }
        
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_1D,glutState.texM);
    }
    
    glBegin(GL_QUADS);
        glTexCoord2f(0,0);
        glVertex2f(0,1);
        glTexCoord2f(1,0);
        glVertex2f(100*glutState.strideRatio,1);
        glTexCoord2f(1,1);
        glVertex2f(100*glutState.strideRatio,100);
        glTexCoord2f(0,1);
        glVertex2f(0,100);
    glEnd();
    //glFlush();
    glutSwapBuffers();
    
    if (gotFrame && !glutState.pausePlayback)
        glutPostRedisplay();
}

void reshape(int w, int h)
{
    int sx = w, sy = h;
    if ((float)w/h > (float)glutState.width[0]/glutState.height[0])
        sx = (h*glutState.width[0])/glutState.height[0];
    else
        sy = (w*glutState.height[0])/glutState.width[0];
    
    int x0 = (w-sx)/2;
    int y0 = (h-sy)/2;
        
    glViewport(x0,y0,sx,sy);
}

void keyboard(unsigned char key, int x, int)
{
    //fprintf(stderr, "%d, (%f, %f)\n", key, float(x)/glutState.width[0], float(y)/glutState.height[0]);
    
    switch(key)
    {
    case 27:
    case 17:
    case 23:
        glutDestroyWindow(glutState.windNr);
        exit(0);
        break;
    case ' ':
        glutState.pausePlayback = !glutState.pausePlayback;
        glutPostRedisplay();
        break;
    case '+':
    case '=':
        glutState.exposure *= 1.5f;
        glUniform1f(glGetUniformLocation(glutState.program, "exposure"), glutState.exposure);
        glutPostRedisplay();
        break;
    case '-':
        glutState.exposure /= 1.5f;
        glUniform1f(glGetUniformLocation(glutState.program, "exposure"), glutState.exposure);
        glutPostRedisplay();
        break;
    case 't':
        glutState.doTmo = !glutState.doTmo;
        glUniform1i(glGetUniformLocation(glutState.program, "doTmo"), glutState.doTmo);
        glutPostRedisplay();
        break;
    case 'l':
        glutState.ldrSim = !glutState.ldrSim;
        glUniform1i(glGetUniformLocation(glutState.program, "ldrSim"), glutState.ldrSim);
        glutPostRedisplay();
        break;
    case 's':
        glutState.decoder.seekToTime(float(x)/glutState.width[0]);
        glutState.seek = true;
        glutPostRedisplay();
        break;
    case 'z':
        glutState.frameNr = 0;
        glutState.showTimings = !glutState.showTimings;
        break;
    }
}

int main(int argc, char** argv)
{
    std::string inputFile;
    GLuint shader;
    float gammaVal = 2.2f, userScaling = 1.0f;

    // Application usage info
    std::string info = 
        std::string("lumaplay -- Playback a high dynamic range (HDR) video\n\n") +
        std::string("Usage: lumaplay --input <hdr_video>\n\n") +
        std::string("Interaction options:\n") +
        std::string("  Ctr+Q/Ctr+W/Esc\tQuit application\n") +
        std::string("  Space\t\t\tPause video\n") +
        std::string("  s\t\t\tSeek to position indicated by mouse pointers horizontal position.\n") +
        std::string("  \t\t\tLeft border of video is start of video, and right border is end of video.\n") +
        std::string("  +\t\t\tIncrease exposure\n") +
        std::string("  -/=\t\t\tDecrease exposure\n") +
        std::string("  t\t\t\tApply simple sigmoid tone curve\n") +
        std::string("  l\t\t\tSimulate low dynamic range (LDR) video\n") +
        std::string("  z\t\t\tShow timings\n");
            
    std::string postInfo = std::string("\nExample: lumaplay --input hdr_video.mkv -s 0.2 -g 1.8 -fps 25\n\n") +
                           std::string("See man page for more information.");
    
    try
    {
        ArgParser argHolder(info, postInfo);
        
        // Input arguments
        argHolder.add(&inputFile,     "--input",     "-i",   "Input HDR video", 0);
        argHolder.add(&gammaVal,      "--gamma",     "-g",   "Display gamma value", 2.2f);
        argHolder.add(&userScaling,   "--scaling",   "-s",   "Scaling to apply to video", 1.0f);
        argHolder.add(&glutState.fps, "--framerate", "-fps", "Framerate (frames/s)", glutState.fps);
        
        // Parse arguments
        if (!argHolder.read(argc, argv))
            return 0;    
        
        // Initialize decoder    
        if (!glutState.decoder.initialize(inputFile.c_str()))
            return 1;
    
        LumaDecoderParams params = glutState.decoder.getParams();
        
        glutState.hbd = params.highBitDepth;
        glutState.stride[0] = params.stride[0];
        glutState.stride[1] = params.stride[1];
        glutState.stride[2] = params.stride[2];
        glutState.width[0] = params.width[0];
        glutState.width[1] = params.width[1];
        glutState.width[2] = params.width[2];
        glutState.height[0] = params.height[0];
        glutState.height[1] = params.height[1];
        glutState.height[2] = params.height[2];
        
        glutState.strideRatio = ((float)glutState.stride[0])/glutState.width[0];
        if (glutState.hbd) glutState.strideRatio /= 2;
        
        glutInit(&argc, argv);
        glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
        glutInitWindowSize(glutState.width[0], glutState.height[0]);
        glutInitWindowPosition(100, 100);
        glutState.windNr = glutCreateWindow("Luma HDRv player");
        
        init();
        glutDisplayFunc(display);
        glutReshapeFunc(reshape);
        glutKeyboardFunc(keyboard);
        
        fprintf(stderr, "OpenGL version : %s\n",  (char*)glGetString(GL_VERSION));
        
        glewInit();
        
        if (!setupShader(shader, gammaVal, userScaling))
            return 1;
        
        glutMainLoop();
    }
    catch (ParserException &e)
    {
        fprintf(stderr, "\nlumaplay input error: %s\n", e.what());
        return 1;
    }
    catch (LumaException &e)
    {
        fprintf(stderr, "\nlumaplay decoding error: %s\n", e.what());
        return 1;
    }
    catch (std::exception & e)
    {
        fprintf(stderr, "\nlumaplay error: %s\n", e.what());
        return 1;
    }
    
    return 0;
}

