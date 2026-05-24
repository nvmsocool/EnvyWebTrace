#include <stdio.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#define GLFW_INCLUDE_ES3
#include "libs/Eigen/StdVector"
#include "libs/Eigen_unsupported/Eigen/BVH" // For KdBVH

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <iostream>
#include <vector>
#include <list>

#include "Tracer.h"
#include "Display.h"
#include "BMP.h"
#include "ImageData.h"
#include "Shape.h"

#include <thread>
std::thread worker;


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
std::string sceneName;
int tracer_mode;

// gui variables/settings
float traceDuration = 0;

// fps vars
float guiFPS = 0;
int maxFPS = 90;
int previewFPS = 10;
float maxMsToWait;

// preview vars
std::list<float> msToWaitHistory;
std::list<float> fpsHistory;
float totalFPSFromMsHistory = 0;
float totalFromMsHistory = 0;
float previewRatio = 0.1f;
float min_ratio;

bool previewed_this_frame = false;

//may return 0 when not able to detect
//const auto processorCount = std::thread::hardware_concurrency();
//int numThreadsToUse = std::max(1, (int)processorCount - 1);

void ResetTrace();
void ResetFileName(){}

void ResizePreview()
{
  preview.Resize(std::max(1,(int)(previewRatio * tracer->requested_width)), std::max(1,(int)(previewRatio * tracer->requested_height)));
  previewRatio = (float)(preview.w + 2) / (float)image.w;
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
  previewRatio = (float)preview.nPathsPerTrace * (maxFPS / (float)previewFPS) / (float)(tracer->requested_height * tracer->requested_width) ;
  ResizePreview();
}

void ResetFPS()
{
  maxMsToWait = 1000.f / (float)maxFPS;
}

// Read a scene file by parsing each line as a command and calling
// scene->Command(...) with the results.
void ReadScene()
{
  std::vector<std::string> sceneLines = 
  {
    "screen 800 600",
    "camera -0.991188 0.446009 -0.111544 0.650000 48.023769 -169.326431 150.757751 0.002000 0.212000",
    "light 5.000000 5.000000 5.000000 ",
    "sphere 3.500000 -2.500000 0.500000 1.500000 ",
    "brdf 0.600000 0.600000 0.600000 0.600000 0.600000 0.600000 0.000000 ",
    "fractal 0.000000 0.000000 0.000000 2.000000 -0.000000 0.000000 -0.000000 100 11 0.000100 0 1.000000 -0.500000 0.000000 0 0.000000 1.000000 -0.500000 0 -0.500000 0.000000 1.000000 2 2.000000 2.000000 2.000000 3 -1.000000 -1.000000 -1.000000 ",
    "brdf 0.600000 0.600000 0.600000 0.030000 0.030000 0.030000 0.000000 ",
    "box -10.000000 4.000000 -10.000000 20.000000 0.100000 20.000000 ",
    "box -10.000000 -8.000000 -10.000000 20.000000 0.100000 20.000000 ",
    "brdf 0.600000 0.600000 0.600000 0.030000 0.030000 0.030000 0.000000 ",
    "box 8.000000 -10.000000 -10.000000 0.100000 20.000000 20.000000 ",
    "box -3.000000 -10.000000 -10.000000 0.100000 20.000000 20.000000 ",
    "brdf 0.600000 0.600000 0.600000 0.030000 0.030000 0.030000 0.000000 ",
    "box -10.000000 -10.000000 -1.500000 20.000000 20.000000 0.100000 ",
    "box -10.000000 -10.000000 1.500000 20.000000 20.000000 0.100000 "
  };

  // For each line in file
  for (const std::string& line : sceneLines)
  {
    std::vector<std::string> strings;
    std::vector<float> floats;

    // Parse as parallel lists of strings and floats
    std::stringstream lineStream(line);
    for (std::string s; lineStream >> s;)
    { // Parses space-separated strings until EOL
      float f;
      //std::stringstream(s) >> f; // Parses an initial float into f, or zero if illegal
      if (!(std::stringstream(s) >> f))
        f = (float)nan(""); // An alternate that produced NANs
      floats.push_back(f);
      strings.push_back(s);
    }

    if (strings.size() == 0)
      continue; // Skip blanks lines
    if (strings[0][0] == '#')
      continue; // Skip comment lines

    // Pass the line's data to Command(...)
    tracer->Command(strings, floats);
  }
}

int pW = 1, pH = 1;

void DrawGUI()
{

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  // Set top and left padding to 20 pixels
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

  ImGui::SetNextWindowSize(ImVec2((float)image.w, (float)image.h));
  ImGui::SetNextWindowPos(ImVec2((float)(display->window_width-display->gui_width-image.w)/2.f, (float)(display->window_height-image.h)/2.f));
  ImGui::Begin("Image", NULL, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize); // Create a window called "Hello, world!" and append into it.
  ImVec2 end = ImVec2(1,1);
  if (previewed_this_frame)
  {
    end = ImVec2((float)pW/(float)image.w, (float)pH/(float)image.h);
  }
  ImGui::Image(
      (ImTextureID)(intptr_t)display->textureID,
      ImVec2((float)image.w, (float)image.h),
      ImVec2(0,0),
      end
  );
  ImGui::End();
  
  ImGui::PopStyleVar();
  ImGui::PopStyleVar();

  ImGui::SetNextWindowSize(ImVec2((float)display->gui_width, (float)display->window_height));
  ImGui::SetNextWindowPos(ImVec2((float)(display->window_width - display->gui_width), 0.f));

  ImGui::Begin("Fractal Tracer", NULL, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize); // Create a window called "Hello, world!" and append into it.

  ImGui::Text("GUI (%.1f FPS)", guiFPS);
  ImGui::ProgressBar(image.pctComplete, ImVec2(-1, 0), (std::string("Frame ") + std::to_string(image.trace_num)).data());
  ImGui::Text("%.3fs/trace, conv=%.4f", traceDuration, image.diff);
  ImGui::Text("Trace/Frame: %d", image.nPathsPerTrace);
  //if (ImGui::CollapsingHeader("under mouse", ImGuiTreeNodeFlags_DefaultOpen))
  //{
  //  ImGui::Text("pos: (%d,%d)", display->mouse_x, display->mouse_y);
  //  ImGui::Text("object: %s", tracer->info_name.data());
  //  ImGui::Text("distance: %.4f", tracer->info_dist);
  //  ImGui::Text("position: (%.2f, %.2f, %.2f)", tracer->info_pos[0], tracer->info_pos[1], tracer->info_pos[2]);
  //  ImGui::Text(singleTrace.data());
  //}

  ImGui::BeginChild("settings");

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
      image.nPathsPerTrace = 1;
      preview.nPathsPerTrace = 1;
      ResetTrace();
    }
    if (ImGui::Checkbox("can_receive_input", &tracer->camera.controlsEnabled))
    {
      tracer->camera.PurgeKeys();
    }
    if (ImGui::SliderInt("guiFPS", &maxFPS, 1, 120))
      ResetFPS();
    //ImGui::SliderInt("threads", &numThreadsToUse, 1, processorCount);
    ImGui::Checkbox("isPaused", &tracer->isPaused);

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
  previewRatio = (float)preview.nPathsPerTrace * (maxFPS / (float)previewFPS) / (float)(tracer->requested_height * tracer->requested_width) ;
  ResizePreview();
}

void MainTrace()
{
  if (shouldReset)
  {
    image.Clear();

    //this has to happen here due to the potentially different trace times that modes can result in
    tracer->DefaultMode = static_cast<Tracer::TRACE_MODE>(tracer_mode);

    // starting render, disable inputs
    if (tracer->DefaultMode == Tracer::TRACE_MODE::FULL)
    {
      tracer->camera.controlsEnabled = false;
      srand(427857);
    }
    shouldReset = false;
  }

  if (uiResized)
  {
    ResizeImages();
    ResetTrace();
    uiResized = false;
  }

  bool update_pass = (image.trace_num - 1) % 10 == 0;
  float diff = tracer->TraceImage(image, update_pass, 1);
}

void loop()
{
  display->PollEvents();

  auto start_time = std::chrono::high_resolution_clock::now();

      previewed_this_frame = false;

  if (!display->closed)
  {
    tracer->SinglePixelInfoTrace(image, display->mouse_x, display->mouse_y);
    MainTrace();
    if ((image.trace_num < 2 && tracer->DefaultMode != Tracer::TRACE_MODE::FULL) || shouldReset)
    {
      previewed_this_frame = true;
      float diff = tracer->TraceImage(preview, false, 1);
      if (preview.trace_num > 1)
      {
        display->DrawArray(preview);
        pW = preview.w;
        pH = preview.h;
        ResizePreview();
      }
    }
    else
    {
      display->DrawArray(image);
    }
    DrawGUI();
    display->FinishDrawing();
  }

  display->UpdateEvent();

  if (display->clickRequest)
  {
    if (display->real_mouse_x < display->window_width - display->gui_width)
      singleTrace = tracer->SinglePixelDebugTrace(image, display->mouse_x, display->mouse_y);
    display->clickRequest = false;
  }

  if (display->active && !isEnteringText && tracer->camera.controlsEnabled)
  {
    if (tracer->camera.Update())
      ResetTrace();
  }

  auto end_time = std::chrono::high_resolution_clock::now();

  float ms = static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count());

  guiFPS = 1000 / ms;

  // do this smarter
  if (guiFPS > maxFPS)
  {
    image.nPathsPerTrace++;
    preview.nPathsPerTrace++;
  }
  else if (image.nPathsPerTrace > 1)
  {
    image.nPathsPerTrace--;
    preview.nPathsPerTrace--;
  }

  display->ClearWindow();

  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  display->Finish();

}


int init_imgui()
{
  // Setup Dear ImGui binding
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui_ImplGlfw_InitForOpenGL(display->window, true);
  ImGui_ImplGlfw_InstallEmscriptenCallbacks(display->window, "#canvas");
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
  tracer = new Tracer();
  display = new Display();
  
  tracer->ClearAll();

  // Read the scene, calling scene.Command for each line.
  ReadScene();

  tracer->Finit();

  display->SetupWindow(tracer->requested_width, tracer->requested_height);
  
  init_imgui();

  // Allocate and clear an image array
  ResizeImages();

  ResetFPS();

  tracer->DefaultMode = Tracer::TRACE_MODE::DIFFUSE;
  tracer_mode = tracer->DefaultMode;
  
  return 0;
}


void quit()
{
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  display->DestroyWindow();

  delete tracer;
  delete display;
}


int main(int argc, char** argv)
{
  if (init() != 0) return 1;

  #ifdef __EMSCRIPTEN__
  emscripten_set_main_loop(loop, 0, 1);
  #endif

  worker.join();

  quit();

  return 0;
}
