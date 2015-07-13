#ifndef SHADER_H
#define SHADER_H

#include "glew.h"
#include <GL/gl.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader
{
public:
  GLuint Program;
  Shader(const GLchar* vertexSourcePath, const GLchar* fragmentSourcePath);
  void Use();

};

#endif // SHADER_H
