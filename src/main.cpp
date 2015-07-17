
#include <iostream>
#define GLEW_STATIC
#include <glew.h>
#include <glfw3.h>


int main()
{
    std::cout << "starting GLFW context, OpenGL 4.0" << std::endl;

    //Initialise GLFW
    glfwInit();

    //set the required options for GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    //create a window object
    GLFWwindow* window = glfwCreateWindow(800,600, "baseOGL", NULL, NULL);
    glfwMakeContextCurrent(window);
}

