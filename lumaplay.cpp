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

#include "config.h"

const char* shader_src[] = {
#include "lumaplay_frag.h"
};

GLuint shader, program, texC1, texC2, texC3, texM;
GLfloat strideRatio;
LumaDecoder decoder;
Frame* frame;

timeval start1, start2, stop;

bool hbd, pausePlayback = 0, doTmo = 0, seek = 0, showTimings = 0, ldrSim = 0;
unsigned int windNr, stride[3], width[3], height[3], frameNr = 0;
float dispTimeTot = 0.0f, totTimeTot = 0.0f, exposure = 1.0f, gammaVal = 2.2f, fps = 25.0f, userScaling = 1.0f;

bool getFrame()
{
    if (!decoder.run())
    {
        decoder.seekToTime(0);
        if (!decoder.run())
            return 0;
    }
    
    frame = decoder.getFrame();
    
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
    
    texC1 = loadTexture(GL_TEXTURE_2D);
    texC2 = loadTexture(GL_TEXTURE_2D);
    texC3 = loadTexture(GL_TEXTURE_2D);
    texM = loadTexture(GL_TEXTURE_1D);
    
    glBindTexture(GL_TEXTURE_1D,texM);
    glTexImage1D(GL_TEXTURE_1D,0,GL_R32F,decoder.getQuantizer()->getSize(),0,GL_RED,GL_FLOAT,decoder.getQuantizer()->getMapping());
}

bool setupShader()
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
    
    program = glCreateProgram();
    
    glAttachShader(program, shader);
    glLinkProgram(program);
    
    glGetProgramiv(program, GL_LINK_STATUS, &flag);
    
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
    
    glUseProgram(program);
    
    glUniform1i(glGetUniformLocation(program, "texC1"), 0);
    glUniform1i(glGetUniformLocation(program, "texC2"), 1);
    glUniform1i(glGetUniformLocation(program, "texC3"), 2);
    glUniform1i(glGetUniformLocation(program, "texM"), 3);
    
    float normalization = hbd ? 65535.0f : 255.0f;
    float maxVal = decoder.getQuantizer()->getSize() / normalization;
    float maxValColor = (pow(2, decoder.getParams()->colorBitDepth) - 1) / normalization;
    unsigned int colorSpaces[] = {LumaQuantizer::CS_LUV, LumaQuantizer::CS_RGB, LumaQuantizer::CS_YCBCR, LumaQuantizer::CS_XYZ};
    unsigned int colorSpace = decoder.getParams()->colorSpace;
    
    glUniform1f(glGetUniformLocation(program, "maxVal"), maxVal);
    glUniform1f(glGetUniformLocation(program, "maxValColor"), maxValColor);
    glUniform1f(glGetUniformLocation(program, "exposure"), exposure);
    glUniform1f(glGetUniformLocation(program, "gamma"), gammaVal);
    glUniform1f(glGetUniformLocation(program, "scaling"), decoder.getParams()->preScaling / userScaling);
    glUniform1iv(glGetUniformLocation(program, "colorSpaces"), 4, (GLint*)(colorSpaces));
    glUniform1i(glGetUniformLocation(program, "colorSpace"), colorSpace);
    glUniform1i(glGetUniformLocation(program, "doTmo"), doTmo);
    glUniform1i(glGetUniformLocation(program, "ldrSim"), ldrSim);
    
    return 1;
}

void display()
{
    if (frameNr > 0 && !pausePlayback)
    {
        gettimeofday(&stop, NULL);
        
        float dispTime = (stop.tv_sec-start2.tv_sec)*1000.0f + (stop.tv_usec-start2.tv_usec)/1000.0f;
        float totTime = (stop.tv_sec-start1.tv_sec)*1000.0f + (stop.tv_usec-start1.tv_usec)/1000.0f;
        
        dispTimeTot = (dispTimeTot*frameNr +  dispTime)/ (frameNr+1);
        totTimeTot = (totTimeTot*frameNr +  totTime)/ (frameNr+1);
        
        if (showTimings)
        {
            fprintf(stderr, "DISPLAY TIME: %f (avg = %f)\n", dispTime, dispTimeTot);
            fprintf(stderr, "TOTAL TIME:   %f (avg = %f), %f fps\n", totTime, totTimeTot, 1000.0f/totTime);
        }
        
        float sleepTime = 1000.0f/fps-totTime, t = 0.0f;
        while (sleepTime > t)
        {
            gettimeofday(&start1, NULL);
            t = (start1.tv_sec-stop.tv_sec)*1000.0f + (start1.tv_usec-stop.tv_usec)/1000.0f;
        }
    }
    
    gettimeofday(&start1, NULL);
    
    bool gotFrame = 0;
    
    if (!pausePlayback || seek)
    {
        gotFrame = getFrame();
        seek = false;
    }
    
    gettimeofday(&start2, NULL);
    
    glClear(GL_COLOR_BUFFER_BIT);
    
    if (gotFrame)
    {
        frameNr++;
        if (hbd)
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D,texC1);
            glTexImage2D(GL_TEXTURE_2D,0,GL_R16,stride[0]/2,height[0],0,GL_RED,GL_UNSIGNED_SHORT,(unsigned short*)(decoder.getBuffer()[0]));
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D,texC2);
            glTexImage2D(GL_TEXTURE_2D,0,GL_R16,stride[1]/2,height[1],0,GL_RED,GL_UNSIGNED_SHORT,(unsigned short*)(decoder.getBuffer()[1]));
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D,texC3);
            glTexImage2D(GL_TEXTURE_2D,0,GL_R16,stride[2]/2,height[2],0,GL_RED,GL_UNSIGNED_SHORT,(unsigned short*)(decoder.getBuffer()[2]));
        }
        else
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D,texC1);
            glTexImage2D(GL_TEXTURE_2D,0,GL_R8,stride[0],height[0],0,GL_RED,GL_UNSIGNED_BYTE,decoder.getBuffer()[0]);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D,texC2);
            glTexImage2D(GL_TEXTURE_2D,0,GL_R8,stride[1],height[1],0,GL_RED,GL_UNSIGNED_BYTE,decoder.getBuffer()[1]);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D,texC3);
            glTexImage2D(GL_TEXTURE_2D,0,GL_R8,stride[2],height[2],0,GL_RED,GL_UNSIGNED_BYTE,decoder.getBuffer()[2]);
        }
        
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_1D,texM);
    }
    
    glBegin(GL_QUADS);
        glTexCoord2f(0,0);
        glVertex2f(0,1);
        glTexCoord2f(1,0);
        glVertex2f(100*strideRatio,1);
        glTexCoord2f(1,1);
        glVertex2f(100*strideRatio,100);
        glTexCoord2f(0,1);
        glVertex2f(0,100);
    glEnd();
    //glFlush();
    glutSwapBuffers();
    
    if (gotFrame && !pausePlayback)
        glutPostRedisplay();
}

void reshape(int w, int h)
{
    int sx = w, sy = h;
    if ((float)w/h > (float)width[0]/height[0])
        sx = (h*width[0])/height[0];
    else
        sy = (w*height[0])/width[0];
    
    int x0 = (w-sx)/2;
    int y0 = (h-sy)/2;
        
    glViewport(x0,y0,sx,sy);
}

void keyboard(unsigned char key, int x, int)
{
    //fprintf(stderr, "%d, (%f, %f)\n", key, float(x)/width[0], float(y)/height[0]);
    
    switch(key)
    {
    case 27:
    case 17:
    case 23:
        glutDestroyWindow(windNr);
        exit(0);
        break;
    case ' ':
        pausePlayback = !pausePlayback;
        glutPostRedisplay();
        break;
    case '+':
        exposure *= 1.5f;
        glUniform1f(glGetUniformLocation(program, "exposure"), exposure);
        glutPostRedisplay();
        break;
    case '-':
        exposure /= 1.5f;
        glUniform1f(glGetUniformLocation(program, "exposure"), exposure);
        glutPostRedisplay();
        break;
    case 't':
        doTmo = !doTmo;
        glUniform1i(glGetUniformLocation(program, "doTmo"), doTmo);
        glutPostRedisplay();
        break;
    case 'l':
        ldrSim = !ldrSim;
        glUniform1i(glGetUniformLocation(program, "ldrSim"), ldrSim);
        glutPostRedisplay();
        break;
    case 's':
        decoder.seekToTime(float(x)/width[0]);
        seek = true;
        glutPostRedisplay();
        break;
    case 'z':
        frameNr = 0;
        showTimings = !showTimings;
        break;
    }
}

int main(int argc, char** argv)
{
    for (int i=0; i<argc; i++)
    {
        if (argc < 2 || !strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")
            || !strcmp(argv[i], "--h") || !strcmp(argv[i], "-help"))
        {
            fprintf(stderr, "lumaplay -- Playback a high dynamic range (HDR) video\n\n");
            fprintf(stderr, "Usage: lumaplay <hdr_video>\n\n");
            fprintf(stderr, "Interaction options:\n");
            fprintf(stderr, "  Ctr+Q/Ctr+W/Esc\tQuit application\n");
            fprintf(stderr, "  Space\t\t\tPause video\n");
            fprintf(stderr, "  s\t\t\tSeek to position indicated by mouse pointers horizontal position.\n");
            fprintf(stderr, "  \t\t\tLeft border of video is start of video, and right border is end of video.\n");
            fprintf(stderr, "  +/-\t\t\tIncrease/decrease exposure\n");
            fprintf(stderr, "  t\t\t\tApply simple sigmoid tone curve\n");
            fprintf(stderr, "  z\t\t\tShow timings\n");
            fprintf(stderr, "\nSee man page for more information.\n");
            
            return 1;
        }
        else if (!(strcmp(argv[i], "-g") && strcmp(argv[i], "--gamma")) && argc > i++)
            gammaVal = atof(argv[i]);
        else if (!(strcmp(argv[i], "-s") && strcmp(argv[i], "--scaling")) && argc > i++)
            userScaling = atof(argv[i]);
        else if (!(strcmp(argv[i], "-fps") && strcmp(argv[i], "--framerate")) && argc > i++)
                fps = atof(argv[i]);
    }
    
    try
    {
        if (!decoder.initialize(argv[1]))
            return 1;
    
        hbd = decoder.getParams()->highBitDepth;
        stride[0] = decoder.getParams()->stride[0];
        stride[1] = decoder.getParams()->stride[1];
        stride[2] = decoder.getParams()->stride[2];
        width[0] = decoder.getParams()->width[0];
        width[1] = decoder.getParams()->width[1];
        width[2] = decoder.getParams()->width[2];
        height[0] = decoder.getParams()->height[0];
        height[1] = decoder.getParams()->height[1];
        height[2] = decoder.getParams()->height[2];
        
        strideRatio = ((float)stride[0])/width[0];
        if (hbd) strideRatio /= 2;
        
        glutInit(&argc, argv);
        glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
        glutInitWindowSize(width[0], height[0]);
        glutInitWindowPosition(100, 100);
        windNr = glutCreateWindow("Luma HDRv player");
        
        init();
        glutDisplayFunc(display);
        glutReshapeFunc(reshape);
        glutKeyboardFunc(keyboard);
        
        fprintf(stderr, "OpenGL version : %s\n",  (char*)glGetString(GL_VERSION));
        
        glewInit();
        
        if (!setupShader())
            return 1;
        
        glutMainLoop();
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

