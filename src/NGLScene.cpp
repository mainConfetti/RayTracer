#include <QMouseEvent>
#include <QGuiApplication>

#include <glew.h>
#include <glfw3.h>

#include "NGLScene.h"
#include <ngl/Camera.h>
#include <ngl/Light.h>
#include <ngl/Material.h>
#include <ngl/NGLInit.h>
#include <ngl/VAOPrimitives.h>
#include <ngl/ShaderLib.h>

#include "shader.h"


//----------------------------------------------------------------------------------------------------------------------
/// @brief the increment for x/y translation with mouse movement
//----------------------------------------------------------------------------------------------------------------------
const static float INCREMENT=0.01f;
//----------------------------------------------------------------------------------------------------------------------
/// @brief the increment for the wheel zoom
//----------------------------------------------------------------------------------------------------------------------
const static float ZOOM=0.1f;

NGLScene::NGLScene(QWindow *_parent) : OpenGLWindow(_parent)
{
  // re-size the widget to that of the parent (in this case the GLFrame passed in on construction)
  m_rotate=false;
  // mouse rotation values set to 0
  m_spinXFace=0.0f;
  m_spinYFace=0.0f;
  setTitle("Qt5 Simple NGL Demo");
 
}


NGLScene::~NGLScene()
{
  ngl::NGLInit *Init = ngl::NGLInit::instance();
  std::cout<<"Shutting down NGL, removing VAO's and Shaders\n";
  delete m_light;
  Init->NGLQuit();
}

void NGLScene::resizeEvent(QResizeEvent *_event )
{
  if(isExposed())
  {
  // set the viewport for openGL we need to take into account retina display
  // etc by using the pixel ratio as a multiplyer
  glViewport(0,0,width()*devicePixelRatio(),height()*devicePixelRatio());
  // now set the camera size values as the screen size has changed
  m_cam->setShape(45.0f,(float)width()/height(),0.05f,350.0f);
  renderLater();
  }
}


void NGLScene::initialize()
{
  // we must call this first before any other GL commands to load and link the
  // gl commands from the lib, if this is not done program will crash
  ngl::NGLInit::instance();

  glClearColor(0.4f, 0.4f, 0.4f, 1.0f);			   // Grey Background
  // enable depth testing for drawing
  glEnable(GL_DEPTH_TEST);
  // enable multisampling for smoother drawing
  glEnable(GL_MULTISAMPLE);

  const GLchar* computeSourcePath = "./shaders/rayTracer.glslcs";
      std::string computeCode;
  std::ifstream cShaderFile;
  cShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  try
  {
      //Open File
    cShaderFile.open(computeSourcePath);
    std::stringstream cShaderStream;
    // Read file buffer contents into streams
    cShaderStream << cShaderFile.rdbuf();
    // close file handler
    cShaderFile.close();
    // Convert stream into GLchar array
    computeCode = cShaderStream.str();
  }
  catch(std::ifstream::failure e)
  {
    std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
  }
  const GLchar* cShaderCode = computeCode.c_str();

  GLuint compute;
  GLint success;
  GLchar infoLog[512];

    // Vertex Shader
    compute = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(compute, 1, &cShaderCode, NULL);
    glCompileShader(compute);

    // Print Compile Errors if any
    glGetShaderiv(compute, GL_COMPILE_STATUS, &success);
    if(!success)
    {
      glGetShaderInfoLog(compute, 512,NULL,infoLog);
      std::cout<<"ERROR::SHADER::COMPUTE::COMPILATION_FAILED\n"<< infoLog << std::endl;
    }

    traceShader = glCreateProgram();
    glAttachShader(traceShader, compute);
    glLinkProgram(traceShader);
    //Print linking errors if any
    glGetProgramiv(traceShader, GL_LINK_STATUS, &success);
    if(!success)
    {
      glGetProgramInfoLog(traceShader, 512, NULL, infoLog);
      std::cout<<"ERROR::SHADER::pROGRAM::LINKING_FAILED\n"<< infoLog<<std::endl;
    }

    glDeleteShader(compute);

  shader = new Shader("./shaders/PhongVertex.glsl", "./shaders/PhongFragment.glsl");
  shader->Use();
  // Set up vertex data (and buffer(s)) and attribute pointers
  GLfloat vertices[] = {
      // Positions         // Colors
      0.5f, -0.5f, 0.0f,   1.0f, 0.0f, 0.0f,  // Bottom Right
     -0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,  // Bottom Left
      0.0f,  0.5f, 0.0f,   0.0f, 0.0f, 1.0f   // Top
  };
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  // Bind the Vertex Array Object first, then bind and set vertex buffer(s) and attribute pointer(s).
  glBindVertexArray(VAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  // Position attribute
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
  glEnableVertexAttribArray(0);
  // Color attribute
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
  glEnableVertexAttribArray(1);

  glBindVertexArray(0); // Unbind VAO

  glViewport(0,0,width()*devicePixelRatio(),height()*devicePixelRatio());
}


void NGLScene::loadMatricesToShader()
{
  //ngl::ShaderLib *shader=ngl::ShaderLib::instance();

  ngl::Mat4 MV;
  ngl::Mat4 MVP;
  ngl::Mat3 normalMatrix;
  ngl::Mat4 M;
  M=m_mouseGlobalTX;
  MV=  M*m_cam->getViewMatrix();
  MVP= M*m_cam->getVPMatrix();
  normalMatrix=MV;
  normalMatrix.inverse();
  glUniformMatrix4fv(glGetUniformLocation(shader->Program, "MV"),1, false, MV.openGL());
  glUniformMatrix4fv(glGetUniformLocation(shader->Program, "MVP"),1, false, MVP.openGL());
  glUniformMatrix3fv(glGetUniformLocation(shader->Program, "normalMatrix"),1, false, normalMatrix.openGL());
  glUniformMatrix4fv(glGetUniformLocation(shader->Program, "M"),1, false, M.openGL());
}

void NGLScene::render()
{
  // clear the screen and depth buffer
  glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // grab an instance of the shader manager

//  // Rotation based on the mouse position for our global transform
//  ngl::Mat4 rotX;
//  ngl::Mat4 rotY;
//  // create the rotation matrices
//  rotX.rotateX(m_spinXFace);
//  rotY.rotateY(m_spinYFace);
//  // multiply the rotations
//  m_mouseGlobalTX=rotY*rotX;
//  // add the translations
//  m_mouseGlobalTX.m_m[3][0] = m_modelPos.m_x;
//  m_mouseGlobalTX.m_m[3][1] = m_modelPos.m_y;
//  m_mouseGlobalTX.m_m[3][2] = m_modelPos.m_z;

//   // get the VBO instance and draw the built in teapot
//  ngl::VAOPrimitives *prim=ngl::VAOPrimitives::instance();
//  prim->createSphere("sphere", 2, 20);
//  // draw
//  loadMatricesToShader();
//  prim->draw("sphere");


  // Draw the triangle
  shader->Use();
  glBindVertexArray(VAO);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  glBindVertexArray(0);

}

//----------------------------------------------------------------------------------------------------------------------
void NGLScene::mouseMoveEvent (QMouseEvent * _event)
{
  // note the method buttons() is the button state when event was called
  // this is different from button() which is used to check which button was
  // pressed when the mousePress/Release event is generated
  if(m_rotate && _event->buttons() == Qt::LeftButton)
  {
    int diffx=_event->x()-m_origX;
    int diffy=_event->y()-m_origY;
    m_spinXFace += (float) 0.5f * diffy;
    m_spinYFace += (float) 0.5f * diffx;
    m_origX = _event->x();
    m_origY = _event->y();
    renderLater();

  }
        // right mouse translate code
  else if(m_translate && _event->buttons() == Qt::RightButton)
  {
    int diffX = (int)(_event->x() - m_origXPos);
    int diffY = (int)(_event->y() - m_origYPos);
    m_origXPos=_event->x();
    m_origYPos=_event->y();
    m_modelPos.m_x += INCREMENT * diffX;
    m_modelPos.m_y -= INCREMENT * diffY;
    renderLater();

   }
}


//----------------------------------------------------------------------------------------------------------------------
void NGLScene::mousePressEvent ( QMouseEvent * _event)
{
  // this method is called when the mouse button is pressed in this case we
  // store the value where the maouse was clicked (x,y) and set the Rotate flag to true
  if(_event->button() == Qt::LeftButton)
  {
    m_origX = _event->x();
    m_origY = _event->y();
    m_rotate =true;
  }
  // right mouse translate mode
  else if(_event->button() == Qt::RightButton)
  {
    m_origXPos = _event->x();
    m_origYPos = _event->y();
    m_translate=true;
  }

}

//----------------------------------------------------------------------------------------------------------------------
void NGLScene::mouseReleaseEvent ( QMouseEvent * _event )
{
  // this event is called when the mouse button is released
  // we then set Rotate to false
  if (_event->button() == Qt::LeftButton)
  {
    m_rotate=false;
  }
        // right mouse translate mode
  if (_event->button() == Qt::RightButton)
  {
    m_translate=false;
  }
}

//----------------------------------------------------------------------------------------------------------------------
void NGLScene::wheelEvent(QWheelEvent *_event)
{

	// check the diff of the wheel position (0 means no change)
	if(_event->delta() > 0)
	{
		m_modelPos.m_z+=ZOOM;
	}
	else if(_event->delta() <0 )
	{
		m_modelPos.m_z-=ZOOM;
	}
	renderLater();
}
//----------------------------------------------------------------------------------------------------------------------

void NGLScene::keyPressEvent(QKeyEvent *_event)
{
  // this method is called every time the main window recives a key event.
  // we then switch on the key value and set the camera in the GLWindow
  switch (_event->key())
  {
  // escape key to quit
  case Qt::Key_Escape : QGuiApplication::exit(EXIT_SUCCESS); break;
  // turn on wirframe rendering
  case Qt::Key_W : glPolygonMode(GL_FRONT_AND_BACK,GL_LINE); break;
  // turn off wire frame
  case Qt::Key_S : glPolygonMode(GL_FRONT_AND_BACK,GL_FILL); break;
  // show full screen
  case Qt::Key_F : showFullScreen(); break;
  // show windowed
  case Qt::Key_N : showNormal(); break;
  default : break;
  }
  // finally update the GLWindow and re-draw
  //if (isExposed())
    renderLater();
}
ngl::Vec3 NGLScene::viewToWorld(float _z)
{
  ngl::Mat4 MV, tMVP;
  MV = m_mouseGlobalTX*m_cam->getViewMatrix();
  tMVP= MV*m_cam->getProjectionMatrix();
  //transpose MVP matrix to conert from ngl to opengl format
  tMVP.transpose();
  ngl::Mat4 inverse=(tMVP).inverse();
  float x = (2.0f * m_origX) / width() - 1.0f;
  float y = 1.0f - (2.0f * m_origY)/ height();
  ngl::Vec4 temp(x,y,_z,1.0f);
  //un project
  ngl::Vec4 worldCoords=inverse*temp;
  //scale by w
  worldCoords/=worldCoords.m_w;
  return worldCoords.toVec3();
}
