#include <stdio.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#define GLFW_INCLUDE_ES3
#include <GLES3/gl3.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <iostream>
#include <vector>

#include "Tracer.h"
#include "Display.h"
#include "BMP.h"
#include "ImageData.h"
#include "Shapes/Shape.h"

// Global variables - the window needs to be passed in to imgui
GLFWwindow* g_window;

// Global variables - these can be edited in the demo
ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
bool show_demo_window = true;
bool show_another_window = false;




ImageData image, preview;

//tracer variables
Tracer *tracer;
Display *display;
std::vector<char> baseNameArr(255);
std::string baseName, bmpName, gifName;

std::string singleTrace;

//tracer state settings
bool shouldReset = false;
bool uiResized = false;
bool isEnteringText = false;
bool shouldReload = false;
std::string sceneName;
int tracer_mode;

// gui variables/settings
float traceDuration = 0;
float traceDiff = 0;

// fps vars
float guiFPS = 0;
int maxFPS = 90;
float maxMsToWait;

// preview vars
std::list<float> msToWaitHistory;
float totalFromMsHistory = 0;
float previewRatio = 0.1f;
float min_ratio;

//may return 0 when not able to detect
//const auto processorCount = std::thread::hardware_concurrency();
//int numThreadsToUse = std::max(1, (int)processorCount - 1);

void ResetTrace();
void ResetFileName();
void HandleGifFrame();

void ResizePreview()
{
  preview.Resize((int)(previewRatio * tracer->requested_width), (int)(previewRatio * tracer->requested_height));
}

void ResizeImage()
{
  image.Resize(tracer->requested_width, tracer->requested_height);
  display->SetRenderSize(image);

  // render at least one pixel
  min_ratio = 2.f / (float)image.w;
  if (image.h < image.w)
    min_ratio = 2.f / (float)image.h;

  tracer->camera.UpdateFOV((float)tracer->requested_width, (float)tracer->requested_height);
}

void ResizeImages()
{
  ResizeImage();
  ResizePreview();
}

void ResetFPS()
{
  maxMsToWait = 1000.f / (float)maxFPS;
}

void SetupScene()
{

  tracer->ClearAll();

  SaveCopy();

  // Read the scene, calling scene.Command for each line.
  ReadScene();

  tracer->Finit();

  display->SetupWindow(tracer->requested_width, tracer->requested_height);

  // Allocate and clear an image array
  ResizeImages();

  ResetFPS();

  tracer_mode = tracer->DefaultMode;
}


void DrawGUI()
{

  //ImGui::SetNextWindowSize(ImVec2((float)display->gui_width, (float)display->window_height));
  //ImGui::SetNextWindowPos(ImVec2((float)(display->window_width - display->gui_width), 0.f));

  ImGui::Begin("Fractal Tracer", NULL, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize); // Create a window called "Hello, world!" and append into it.

  ImGui::Text("GUI (%.1f FPS)", guiFPS);
  ImGui::ProgressBar(image.pctComplete, ImVec2(-1, 0), (std::string("Frame ") + std::to_string(image.trace_num)).data());
  ImGui::Text("%.3fs/trace, conv=%.4f", traceDuration, traceDiff);
  ImGui::Text("Preview Ratio: %.4f", previewRatio);
  if (ImGui::CollapsingHeader("under mouse", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::Text("pos: (%d,%d)", display->mouse_x, display->mouse_y);
    ImGui::Text("object: %s", tracer->info_name.data());
    ImGui::Text("distance: %.4f", tracer->info_dist);
    ImGui::Text("position: (%.2f, %.2f, %.2f)", tracer->info_pos[0], tracer->info_pos[1], tracer->info_pos[2]);
    ImGui::Text(singleTrace.data());
  }

  ImGui::BeginChild("settings");

  if (ImGui::CollapsingHeader("scene"))
  {
    ImGui::Indent(16.0f);

    //ImGui::InputText("scene_file", &baseName);
    if (ImGui::InputText("Text", baseNameArr.data(), baseNameArr.size(),
            ImGuiInputTextFlags_CallbackCharFilter))
    {
      baseName = baseNameArr.data();
      ResetFileName();
    }
    if (ImGui::IsItemActive())
      isEnteringText = true;
    else
      isEnteringText = false;

    if (ImGui::Button("load_scene"))
    {
      //reload scene
      shouldReload = true;
    }
    if (ImGui::Button("save_scene"))
    {
      SaveCopy();
      SaveScene(sceneName, tracer);
    }
    if (ImGui::Button("BMP snap"))
    {
      generateBitmapImage(image, bmpName.data());
      SaveCopy();
    }

    ImGui::Unindent(16.0f);
  }
  if (ImGui::CollapsingHeader("tracer settings"))
  {
    ImGui::Indent(16.0f);
    bool size_changed = false;
    size_changed |= ImGui::InputInt("width##image_width", &tracer->requested_width);
    size_changed |= ImGui::InputInt("height##image_height", &tracer->requested_height);
    if (size_changed)
    {
      uiResized = true;
    }
    const char *items[] = {
      "FULL",
      "NORMAL",
      "DEPTH",
      "DIFFUSE",
      "SIMPLE",
      "POSITION",
      "DEPTH_RATIO",
    };
    if (ImGui::Combo("render_type", &tracer_mode, items, 7, 4))
    {
      ResetTrace();
    }
    if (ImGui::Checkbox("can_receive_input", &tracer->camera.controlsEnabled))
    {
      tracer->camera.PurgeKeys();
    }
    if (ImGui::SliderInt("guiFPS", &maxFPS, 1, 120))
      ResetFPS();
    ImGui::SliderInt("threads", &numThreadsToUse, 1, processorCount);
    ImGui::Checkbox("isPaused", &tracer->isPaused);
    ImGui::Checkbox("gif_render", &wannaTraceGif);
    if (wannaTraceGif)
    {
      if (isTracingGif)
      {
        ImGui::Text(("Trace: " + std::to_string(currentGifTrace)).data());
        ImGui::Text(("Frame: " + std::to_string(currentGifFrame)).data());
        if (ImGui::Button("Stop"))
        {
          isTracingGif = false;
        }
      }
      else
      {
        if (ImGui::Button("Frame0"))
        {
          tracer->TakeSnapshot(0);
        }
        ImGui::SameLine();
        if (ImGui::Button("Frame1"))
        {
          tracer->TakeSnapshot(1);
        }
        ImGui::InputInt("num_gif_frames", &numGifFrames);
        ImGui::InputInt("frame_delay", &gifDelay);
        ImGui::InputInt("traces_per_frame", &tracesPerGifFrame);
        if (ImGui::Button("Start"))
        {
          isTracingGif = true;
          tracer->camera.controlsEnabled = false;
          currentGifTrace = 0;
          gifImages.resize(numGifFrames);
          for (size_t i = 0; i < numGifFrames; i++)
          {
            gifImages[i].Clear();
            gifImages[i].Resize(image.w, image.h);
          }
        }
      }
    }

    ImGui::Unindent(16.0f);
  }
  if (ImGui::CollapsingHeader("camera settings"))
  {
    ImGui::Indent(16.0f);

    if (ImGui::DragFloat3("cam_pos", tracer->camera.position.data(), 0.01f, -10000, 10000, "%.3f"))
    {
      tracer->camera.ResetViews();
      ResetTrace();
    }
    if (ImGui::DragFloat3("cam_rot", tracer->camera.displayRotation.data(), 0.01f, -180, 180, "%.3f"))
    {
      tracer->camera.rotation = EulerToQuat(tracer->camera.displayRotation);
      tracer->camera.ResetViews();
      ResetTrace();
    }

    ImGui::DragFloat("move_speed", &tracer->camera.speedMove, 0.01f);
    ImGui::DragFloat("rot_speed", &tracer->camera.speedRot, 0.01f);
    if (ImGui::Checkbox("use_AA", &tracer->use_AA))
    {
      ResetTrace();
    }
    if (ImGui::Checkbox("depth_of_field", &tracer->depth_of_field))
    {
      ResetTrace();
    }
    if (tracer->depth_of_field)
    {
      ImGui::Indent(10.f);
      if (ImGui::DragFloat("amount", &tracer->camera.w, 0.001f, 0.0, 100, "%.3f"))
        ResetTrace();
      if (ImGui::DragFloat("distance", &tracer->camera.f, 0.01f, 0.f, 10000.f, "%2f"))
        ResetTrace();
      ImGui::Unindent(10.f);
    }
    if (ImGui::Checkbox("halfDome", &tracer->halfDome))
    {
      ResetTrace();
    }
    if (ImGui::DragFloat("fov", &tracer->camera.ry, 0.01f, 0.01f, 1.0f, "%.2f"))
    {
      tracer->camera.UpdateFOV((float)image.w, (float)image.h);
      ResetTrace();
    }

    ImGui::Unindent(16.0f);
  }
  if (ImGui::CollapsingHeader("shapes"))
  {
    ImGui::Indent(16.0f);

    for (size_t i = 0; i < tracer->objects_p.size(); i++)
    {
      if (tracer->objects_p[i]->RenderGenericGUI(i))
      {
        ResetTrace();
      }
    }

    ImGui::Unindent(16.0f);
  }

  ImGui::EndChild();

  ImGui::End();


  // Rendering
  ImGui::Render();
  ImGuiIO &io = ImGui::GetIO();

  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ResetTrace()
{
  shouldReset = true;
  preview.Clear();
  traceDiff = 100;
}

void loop()
{
  glfwPollEvents();

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();


  DrawGUI();

  // // 1. Show a simple window.
  // // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets automatically appears in a window called "Debug".
  // {
  //     static float f = 0.0f;
  //     static int counter = 0;
  //     ImGui::Text("Hello, world!");                           // Display some text (you can use a format string too)
  //     ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
  //     ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color
// 
  //     ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our windows open/close state
  //     ImGui::Checkbox("Another Window", &show_another_window);
// 
  //     if (ImGui::Button("Button"))                            // Buttons return true when clicked (NB: most widgets return true when edited/activated)
  //         counter++;
  //     ImGui::SameLine();
  //     ImGui::Text("counter = %d", counter);
// 
  //     ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
  // }
// 
  // // 2. Show another simple window. In most cases you will use an explicit Begin/End pair to name your windows.
  // if (show_another_window)
  // {
  //     ImGui::Begin("Another Window", &show_another_window);
  //     ImGui::Text("Hello from another window!");
  //     if (ImGui::Button("Close Me"))
  //         show_another_window = false;
  //     ImGui::End();
  // }
// 
  // // 3. Show the ImGui demo window. Most of the sample code is in ImGui::ShowDemoWindow(). Read its code to learn more about Dear ImGui!
  // if (show_demo_window)
  // {
  //     ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver); // Normally user code doesn't need/want to call this because positions are saved in .ini file anyway. Here we just want to make the demo initial state a bit more friendly!
  //     ImGui::ShowDemoWindow(&show_demo_window);
  // }

  ImGui::Render();

  int display_w, display_h;
  glfwMakeContextCurrent(g_window);
  glfwGetFramebufferSize(g_window, &display_w, &display_h);
  glViewport(0, 0, display_w, display_h);
  glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
  glClear(GL_COLOR_BUFFER_BIT);

  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  glfwMakeContextCurrent(g_window);
}


int init_gl()
{
  if(!glfwInit())
  {
      fprintf(stderr, "Failed to initialize GLFW\n");
      return 1;
  }

  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // We don't want the old OpenGL
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);

  // Open a window and create its OpenGL context.
  // The window is created with minimal size,
  // which will be updated with an automatic resize. 
  // You could get the primary viewport size here to create.
  g_window = glfwCreateWindow(1, 1, "WebGui Demo", NULL, NULL);
  if (g_window == NULL)
  {
      fprintf(stderr, "Failed to open GLFW window.\n");
      glfwTerminate();
      return -1;
  }
  glfwMakeContextCurrent(g_window); // Initialize GLEW

  return 0;
}


int init_imgui()
{
  // Setup Dear ImGui binding
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui_ImplGlfw_InitForOpenGL(g_window, true);
  ImGui_ImplGlfw_InstallEmscriptenCallbacks(g_window, "#canvas");
  ImGui_ImplOpenGL3_Init("#version 300 es");

  // Setup style
  ImGui::StyleColorsDark();

  ImGuiIO& io = ImGui::GetIO();

  // Disable loading of imgui.ini file
  io.IniFilename = nullptr;

  // Load Fonts
  io.Fonts->AddFontFromFileTTF("data/xkcd-script.ttf", 23.0f);
  io.Fonts->AddFontDefault();

  return 0;
}


int init()
{
  init_gl();
  init_imgui();
  return 0;
}


void quit()
{
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(g_window);
  glfwTerminate();
}


int main(int argc, char** argv)
{
  if (init() != 0) return 1;

  tracer = new Tracer();
  display = new Display();
  gif_writer = new GifWriter();
  
  SetupScene();

  tracer->DefaultMode = Tracer::TRACE_MODE::DIFFUSE;

  #ifdef __EMSCRIPTEN__
  emscripten_set_main_loop(loop, 0, 1);
  #endif

  quit();

  return 0;
}
