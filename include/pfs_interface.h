/**
 * \class PfsInterface
 *
 * \brief Output/input of HDR video frames to/from pfstools.
 *
 * PfsInterface provides a minimal interface to read a PFS HDR video stream 
 * piped from standard input, and to pipe HDR to a PFS standard output stream.
 *
 *
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
 * \author Rafal Mantiuk, mantiuk@gmail.com
 *
 * \date Oct 7 2015
 */

#ifndef PFS_INTERFACE_H
#define PFS_INTERFACE_H

/*
 * Read frames using pfstools. The class isolates the code from pfstools inteface.
 */ 

#include <cstddef>
#include <stdio.h>

#include "luma_frame.h"

namespace pfs
{
class DOMIO;
}


class PfsInterface
{
  FILE *fh;
  pfs::DOMIO *pfsio;  
  
public:
  
  PfsInterface() : fh(NULL), pfsio(NULL)
  {
  }

  ~PfsInterface();     

  bool readFrame(const char *inputFile, LumaFrame &frame);
  bool writeFrame(const char *outputFile, LumaFrame &frame);
};

#endif //PFS_INTERFACE_H
