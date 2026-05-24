#include "Display.h"

#include <GLFW/glfw3.h>
#include "Utilities.h"

// global callbacks for glfw

static Display *globalDisplay = nullptr;

static void glfw_error_callback(int error, const char *description)
{
  fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

void ReshapeFunc(GLFWwindow *win, int w, int h)
{
  globalDisplay->ReshapeWindow(w, h);
}

void MoveFunc(GLFWwindow *win, double _x, double _y)
{
  globalDisplay->real_mouse_x = (int)_x;
  globalDisplay->real_mouse_y = (int)_y;
  globalDisplay->mouse_x = (int)((float)globalDisplay->render_width * (float)(globalDisplay->real_mouse_x - globalDisplay->left_i) / (float)(globalDisplay->right_i - globalDisplay->left_i));
  globalDisplay->mouse_y = (int)((float)globalDisplay->render_height * (float)(globalDisplay->real_mouse_y - globalDisplay->top_i) / (float)(globalDisplay->bottom_i - globalDisplay->top_i));
  globalDisplay->mouse_x = std::max(0, std::min(globalDisplay->mouse_x, globalDisplay->render_width));
  globalDisplay->mouse_y = std::max(0, std::min(globalDisplay->mouse_y, globalDisplay->render_height));
}

void CloseFunc(GLFWwindow *window)
{
  globalDisplay->closed = true;

  
}

void ActiveFunc(GLFWwindow *window, int focused)
{
  globalDisplay->active = (bool)focused;
}

void ClickFunc(GLFWwindow *window, int button, int action, int mods)
{
  if (button == GLFW_MOUSE_BUTTON_1 && action == GLFW_PRESS)
    globalDisplay->clickRequest = true;
}

// class functions
GLuint textureID;

void Display::CreateTexture()
{
  if (textureID != 0)
    {
        glDeleteTextures(1, &textureID);
        textureID = 0;
    }
    
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Allocate texture memory
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        34837,
        render_width,
        render_height,
        0,
        GL_RGB,
        GL_FLOAT,
        nullptr
    );
}

void Display::DrawArray(ImageData &id)
{

  if (closed)
    return;

    if (textureID == 0)
    {
        CreateTexture();

    }
    if (textureID == 0)
    {
      return;
    }

  glBindTexture(GL_TEXTURE_2D, textureID);

    glTexSubImage2D(
        GL_TEXTURE_2D,
        0,
        0,
        0,
        id.w,
        id.h,
        GL_RGB,
        GL_FLOAT,
        &id.data[0][0]
    );
}

void Display::FinishDrawing()
{
  glFlush();
  glfwSwapBuffers(window);
}

void Display::UpdateEvent()
{
  //glfwPollEvents();
}

void Display::SetRenderSize(ImageData &id)
{
  render_width = id.w;
  render_height = id.h;
  CreateTexture();
  CalcImageViewport();
}

// Called when the window size is changed.
void Display::ReshapeWindow(int w, int h)
{
  // clear everything
  int display_w, display_h;
  //glfwGetFramebufferSize(window, &display_w, &display_h);
  //glViewport(0, 0, display_w, display_h);
  //glClearColor(0,0,0,0);
  //glClear(GL_COLOR_BUFFER_BIT);

  //glfwSwapBuffers(window);

  //reset view
  window_width = w;
  window_height = h;
  //if (w && h)
  //  glViewport(0, 0, w, h);

  CalcImageViewport();
}

void Display::CalcImageViewport()
{
  //re-calc image render area
  float useable_w = (float)(window_width - gui_width);

  float image_ar = (float)render_width / (float)render_height;
  float viewing_ar = useable_w / (float)window_height;

  top = 1;
  bottom = -1;
  left = -1;

  top_i = 0;
  bottom_i = window_height;
  left_i = 0;

  if (image_ar > viewing_ar)
  {
    //image is too wide, chop top/bottom
    right_i = window_width - gui_width;
    right = 2 * ((float)right_i / (float)window_width) - 1;
    float shrunken_y = useable_w / image_ar;
    float y_pad = (window_height - shrunken_y) / 2.f;
    top_i = (int)y_pad;
    bottom_i = (int)(window_height - y_pad);
    top = -(2 * (y_pad / window_height) - 1);
    bottom = -top;
  }
  else
  {
    // image is too tall, chop left/right
    float shrunken_x = window_height * image_ar;
    float x_pad = (window_width - (gui_width + shrunken_x)) / 2.f;
    left_i = (int)(x_pad);
    right_i = (int)(x_pad + shrunken_x);
    left = 2 * (x_pad / window_width) - 1;
    right = 2 * ((x_pad + shrunken_x) / window_width) - 1;
  }
}

void Display::SetupWindow(int _w, int h)
{
  int w = _w + gui_width;
  // Initialize the OpenGL bindings

  globalDisplay = this;

  // Setup window
  if (!glfwInit())
  {
      fprintf(stderr, "Failed to initialize GLFW\n");
    return;
  }

  
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // We don't want the old OpenGL
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);

  // Create window with graphics context
  window = glfwCreateWindow(w, h, "EnvyWebTrace", NULL, NULL);
  if (window == NULL)
  {
      fprintf(stderr, "Failed to open GLFW window.\n");
      glfwTerminate();
      return;
  }

  glfwMakeContextCurrent(window);

  glfwSwapInterval(1); // Enable vsync

  glfwSetWindowFocusCallback(window, ActiveFunc);
  glfwSetWindowSizeCallback(window, ReshapeFunc);
  glfwSetWindowCloseCallback(window, CloseFunc);
  glfwSetCursorPosCallback(window, MoveFunc);
  glfwSetMouseButtonCallback(window, ClickFunc);
  
  

  ReshapeWindow(w, h);

  closed = false;
  active = true;


}

void Display::DestroyWindow()
{
  glfwDestroyWindow(globalDisplay->window);
  glfwTerminate();
}


void Display::ClearWindow()
{
  int display_w, display_h;
  glfwMakeContextCurrent(window);
  glfwGetFramebufferSize(window, &display_w, &display_h);
  glViewport(0, 0, display_w, display_h);
  glClearColor(0.0f, 0.0f, 0.0f, 1.00f);
  glClear(GL_COLOR_BUFFER_BIT);
}

void Display::Finish()
{
  glfwMakeContextCurrent(window);
}

void Display::PollEvents()
{
  glfwPollEvents();
}