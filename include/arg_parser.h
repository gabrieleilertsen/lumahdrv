/**
 * \class ArgParser
 *
 * \brief Interface for command line argument parsing.
 *
 * ArgParser provides functionality to parse input from command line, and to 
 * display help texts on the different possible input arguments.
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
 * \author Gabriel Eilertsen, gabriel.eilertsen@liu.se
 *
 * \date Oct 7 2015
 */


#ifndef ARG_PARSER_H
#define ARG_PARSER_H

#include <string>
#include <vector>
#include <exception>

#define ARG_EPS 1e-8

enum argType_t {ARG_NONE, ARG_FLAG, ARG_UINT, ARG_FLOAT, ARG_STRING};

struct ArgHolder
{
    ArgHolder(std::string name_,
              std::string shortName_,
              std::string description_,
              bool optional_ = true)
        : name(name_), shortName(shortName_), description(description_), 
          optional(optional_), checkRange(false), checkValid(false), argType(ARG_NONE),
          varFLAG(NULL), varUINT(NULL), varFLOAT(NULL), varSTRING(NULL) {}
          
    std::string name, shortName, description;
    bool optional, checkRange, checkValid;
    argType_t argType;
    
    bool *varFLAG;
    
    unsigned int *varUINT, minUINT, maxUINT;
    std::vector<unsigned int> validUINT;
    
    float *varFLOAT, minFLOAT, maxFLOAT;
    std::vector<float> validFLOAT;
    
    std::string *varSTRING;
    std::vector<std::string> validSTRING;
};

class ParserException: public std::exception
{
  std::string msg;  
public:
    ParserException( std::string &message ) : msg(message) {}
    ParserException( const char *message ) { msg = std::string( message ); }

    ~ParserException() throw ()
    {
    }


    virtual const char* what() const throw()
    {
        return msg.c_str();
    }
};

class ArgParser
{
public:
    ArgParser(std::string preInfo = "", std::string postInfo = "")
        : m_preInfo(preInfo), m_postInfo(postInfo) {}
    
    void displayInfo();
    bool read(int argc, char* argv[]);
    
    // flag ---
    void add(bool *var,
             std::string name, std::string shortName, std::string description);
    
    // unsigned int ---
    void add(unsigned int *var,
             std::string name, std::string shortName, std::string description,
             bool optional = true);
    
    void add(unsigned int *var,
             std::string name, std::string shortName, std::string description,
             unsigned int mn, unsigned int mx,
             bool optional = true);

    void add(unsigned int *var,
             std::string name, std::string shortName, std::string description,
             unsigned int validArgs[], int nrValid,
             bool optional = true);
    
    // float ---
    void add(float *var,
             std::string name, std::string shortName, std::string description,
             bool optional = true);
    
    void add(float *var,
             std::string name, std::string shortName, std::string description,
             float mn, float mx,
             bool optional = true);

    void add(float *var,
             std::string name, std::string shortName, std::string description,
             float validArgs[], int nrValid,
             bool optional = true);
    
    // string ---
    void add(std::string *var,
             std::string name, std::string shortName, std::string description,
             bool optional = true);
    
    void add(std::string *var,
             std::string name, std::string shortName, std::string description,
             std::string validArgs[], int nrValid,
             bool optional = true);
    
private:
    std::vector<ArgHolder> m_args;
    std::string m_preInfo, m_postInfo;
};

#endif //ARG_PARSER_H
