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
#include <map>

#include "Tracer.h"
#include "Display.h"
#include "BMP.h"
#include "ImageData.h"
#include "Shape.h"


ImageData image, preview;

//tracer variables
Tracer *tracer;
Display *display;
std::vector<char> baseNameArr(255);
std::string userSceneText = "";
std::string baseName, bmpName, gifName;
std::string selectedPreset = "Tentacle";

std::string singleTrace;

//tracer state settings
bool shouldReset = false;
bool uiResized = false;
bool isEnteringText = false;
std::string sceneName;
int tracer_mode;

// fps vars
float guiFPS = 0;
int maxFPS = 90;
int previewFPS = 5;

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

static std::map<std::string, std::string> presets = 
{
  { "Tentacle",
R"(screen 800 600
camera -0.991188 0.446009 -0.111544 0.650000 48.023769 -169.326431 150.757751 0.002000 0.212000
light 5.000000 5.000000 5.000000
sphere 3.500000 -2.500000 0.500000 1.500000
brdf 0.600000 0.600000 0.600000 0.600000 0.600000 0.600000 0.000000
fractal 0.000000 0.000000 0.000000 2.000000 -0.000000 0.000000 -0.000000 100 11 0.000100 2.0 1000 0 1.000000 -0.500000 0.000000 0 0.000000 1.000000 -0.500000 0 -0.500000 0.000000 1.000000 2 2.000000 2.000000 2.000000 3 -1.000000 -1.000000 -1.000000
brdf 0.600000 0.600000 0.600000 0.030000 0.030000 0.030000 0.000000
box -10.000000 4.000000 -10.000000 20.000000 0.100000 20.000000
box -10.000000 -8.000000 -10.000000 20.000000 0.100000 20.000000
brdf 0.600000 0.600000 0.600000 0.030000 0.030000 0.030000 0.000000
box 8.000000 -10.000000 -10.000000 0.100000 20.000000 20.000000
box -3.000000 -10.000000 -10.000000 0.100000 20.000000 20.000000
brdf 0.600000 0.600000 0.600000 0.030000 0.030000 0.030000 0.000000
box -10.000000 -10.000000 -1.500000 20.000000 20.000000 0.100000
box -10.000000 -10.000000 1.500000 20.000000 20.000000 0.100000)"
  },
  { "Menger",
R"(screen 800 600
camera 3.740533 3.235876 -0.310159 0.410000 79.603683 124.465317 -176.227631 0.002000 0.212000
brdf 0.600000 0.600000 0.600000 0.030000 0.030000 0.030000 0.000000 
box -10.000000 -10.000000 -1.500000 20.000000 20.000000 0.100000 
box -10.000000 -10.000000 1.500000 20.000000 20.000000 0.100000 
brdf 0.600000 0.600000 0.600000 0.600000 0.600000 0.600000 0.000000 
fractal 0.000000 0.000000 0.000000 3.000000 -0.000000 0.000000 -0.000000 100 11 0.000100 0.0 10000 0 1.000000 0.000000 0.000000 0 0.000000 1.000000 0.000000 0 0.000000 0.000000 1.000000 0 1.000000 0.000000 -1.000000 0 0.000000 1.000000 -1.000000 0 1.000000 -1.000000 0.000000 3 0.000000 0.000000 -0.333333 0 0.000000 0.000000 1.000000 2 1.000000 1.000000 -1.000000 3 0.000000 0.000000 0.333333 2 3.000000 3.000000 3.000000 3 -2.000000 -2.000000 0.000000 
brdf 0.600000 0.600000 0.600000 0.030000 0.030000 0.030000 0.000000 
box 8.000000 -10.000000 -10.000000 0.100000 20.000000 20.000000 
box -3.000000 -10.000000 -10.000000 0.100000 20.000000 20.000000 
brdf 0.600000 0.600000 0.600000 0.030000 0.030000 0.030000 0.000000 
box -10.000000 4.000000 -10.000000 20.000000 0.100000 20.000000 
box -10.000000 -8.000000 -10.000000 20.000000 0.100000 20.000000 
light 5.000000 5.000000 5.000000 
sphere 3.500000 -2.500000 0.500000 1.500000)"
  },
  { "Sierpinski",
R"(screen 800 600
camera 4.248725 2.256079 1.383799 0.080000 116.052422 120.473274 139.612091 0.002000 0.212000
brdf 0.600000 0.600000 0.600000 0.030000 0.030000 0.030000 0.000000 
box -10.000000 -10.000000 -1.500000 20.000000 20.000000 0.100000 
box -10.000000 -10.000000 1.500000 20.000000 20.000000 0.100000 
brdf 0.600000 0.600000 0.600000 0.600000 0.600000 0.600000 0.000000 
fractal 0.000000 0.000000 0.000000 2.000000 -0.000000 0.000000 -0.000000 100 11 0.000100 2.000000 1000.000000 0 1.000000 1.000000 0.000000 0 0.000000 1.000000 1.000000 0 1.000000 0.000000 1.000000 2 2.000000 2.000000 2.000000 3 -1.000000 -1.000000 -1.000000 
brdf 0.600000 0.600000 0.600000 0.030000 0.030000 0.030000 0.000000 
box 8.000000 -10.000000 -10.000000 0.100000 20.000000 20.000000 
box -3.000000 -10.000000 -10.000000 0.100000 20.000000 20.000000 
brdf 0.600000 0.600000 0.600000 0.030000 0.030000 0.030000 0.000000 
box -10.000000 4.000000 -10.000000 20.000000 0.100000 20.000000 
box -10.000000 -8.000000 -10.000000 20.000000 0.100000 20.000000 
light 5.000000 5.000000 5.000000 
sphere 3.500000 -2.500000 0.500000 1.500000 )"
  },
};


EM_JS(void, CopyToClipboard, (const char* str), {
    const text = UTF8ToString(str);

    if (Module.canvas)
        Module.canvas.focus();

    navigator.clipboard.writeText(text)
        .catch(err =>
            console.error("clipboard failed", err));
});

// Read a scene file by parsing each line as a command and calling
// scene->Command(...) with the results.
void ReadScene(std::string s)
{
  std::stringstream ss(s);

  std::string line;

  while (std::getline(ss, line))
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

void PrintScene()
{
    userSceneText.clear();
    userSceneText += "screen " + std::to_string(image.w) + " " + std::to_string(image.h) + "\n";
    userSceneText += tracer->camera.GetCameraString() + "\n";
    for (auto m : tracer->shapes_by_material)
    {
      userSceneText += m.first->Serialize() + "\n";
      for (auto s : m.second)
      {
        userSceneText += s->Serialize() + "\n";
      }
    }
}

void ReloadScene(std::string s)
{
  tracer->ClearAll();
  ReadScene(s);
  tracer->Finit();
  ResizeImages();
  tracer_mode = tracer->DefaultMode;
  ResetTrace();
}

void LoadUserScene()
{
  ReadScene(userSceneText);
}

int pW = 1, pH = 1;
bool fullscreen = true;
bool tooltips = true;

void DrawGUI()
{

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  // Set top and left padding to 20 pixels
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

  ImGui::SetNextWindowSize(ImVec2((float)(display->window_width-display->gui_width), (float)(display->window_height)));
  ImGui::SetNextWindowPos(ImVec2(0,0));

  ImGui::Begin("Image", NULL, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize); // Create a window called "Hello, world!" and append into it.
  
  ImVec2 end = ImVec2(1,1);
  float w = tracer->halfDome ? image.w * 2 : image.w;
  if (previewed_this_frame)
  {
    end = ImVec2((float)(tracer->halfDome ? pW * 2 : pW)/w, (float)pH/(float)image.h);
  }

  ImVec2 displaySize = ImVec2(w, (float)image.h);
  if (fullscreen)
  {
    float imageAspect = w/(float)image.h;
    float containerWidth = (float)(display->window_width - display->gui_width);
    float containerAspect = containerWidth / (float)display->window_height;
    float scaledWidth;
    float scaledHeight;

    // Image is wider than container
    if (imageAspect > containerAspect)
    {
        scaledWidth = containerWidth;
        scaledHeight = containerWidth / imageAspect;
    }
    else
    {
        // Image is taller than container
        scaledHeight = (float)display->window_height;
        scaledWidth = (float)display->window_height * imageAspect;
    }
    
    displaySize = ImVec2(scaledWidth, scaledHeight);
    ImGui::SetCursorPos(ImVec2((containerWidth - scaledWidth) * 0.5f, ((float)display->window_height - scaledHeight) * 0.5f));
  }
  else
  {
    ImGui::SetCursorPos(ImVec2((float)(display->window_width-display->gui_width-w) * 0.5f, (float)(display->window_height-image.h) * 0.5f));
  }


  ImGui::Image(
      (ImTextureID)(intptr_t)display->textureID,
      displaySize,
      ImVec2(0, 0),
      end
  );
  
  ImVec2 imagePos = ImGui::GetItemRectMin();
  ImVec2 imageEnd = ImGui::GetItemRectMax();
  ImVec2 mousePos = ImGui::GetMousePos();

  float localX = std::min((float)image.w, std::max(0.f, (mousePos.x - imagePos.x) * (float)image.w / (imageEnd.x - imagePos.x)));
  float localY = (float)image.h -  std::min((float)image.h, std::max(0.f, (mousePos.y - imagePos.y) * (float)image.h / (imageEnd.y - imagePos.y)));

  if (ImGui::IsItemHovered() && display->active)
  {
      tracer->SinglePixelInfoTrace(image, localX, localY);
      if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
      {
        singleTrace = tracer->SinglePixelDebugTrace(image, localX, localY);
      }
      if (tracer->camera.controlsEnabled)
      {
      if (tracer->camera.UpdateMouse())
        ResetTrace();
      }
  }
  ImGui::End();
  
  ImGui::PopStyleVar();

  ImGui::SetNextWindowSize(ImVec2((float)display->gui_width, (float)display->window_height));
  ImGui::SetNextWindowPos(ImVec2((float)(display->window_width - display->gui_width), 0.f));

  ImGui::Begin("Fractal Tracer", NULL, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize); // Create a window called "Hello, world!" and append into it.

  ImGui::Text("GUI (%.1f FPS)", guiFPS);
  ImGui::ProgressBar(image.pctComplete, ImVec2(-1, 0), (std::string("Frame ") + std::to_string(image.trace_num)).data());
  ImGui::Text("%.3fs/trace, conv=%.4f", image.traceDuration, image.cachedDiff);
  if (tooltips && ImGui::IsItemHovered()) 
      ImGui::SetTooltip("how long each trace takes, and how close it is to converging.");
  ImGui::Text("Trace/Frame: %d", image.nPathsPerTrace);
  if (tooltips && ImGui::IsItemHovered()) 
      ImGui::SetTooltip("how many paths are traced each GUI update.");
  if (ImGui::CollapsingHeader("debug info", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::Text("pos: (%d,%d)", (int)localX, (int)localY);
    ImGui::Text("object: %s", tracer->info_name.data());
    ImGui::Text("distance: %.4f", tracer->info_dist);
    ImGui::Text("position: (%.2f, %.2f, %.2f)", tracer->info_pos[0], tracer->info_pos[1], tracer->info_pos[2]);
    if (ImGui::CollapsingHeader("debug trace"))
    {
      ImGui::Text(singleTrace.data());
    }
  }

  ImGui::BeginChild("settings");

  

  if (ImGui::CollapsingHeader("tracer settings"))
  {
    ImGui::Indent(16.0f);
    bool size_changed = false;
    size_changed |= ImGui::InputInt("width##image_width", &tracer->requested_width);
    size_changed |= ImGui::InputInt("height##image_height", &tracer->requested_height);
    if (size_changed)
    {
      ResizeImages();
      ResetTrace();
    }
    ImGui::Checkbox("fullscreen_preview", &fullscreen);
    if (tooltips && ImGui::IsItemHovered()) 
        ImGui::SetTooltip("View rendered image blown up in the GUI");
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
    if (tooltips && ImGui::IsItemHovered()) 
        ImGui::SetTooltip("Render technique used for image");
    ImGui::SliderInt("guiFPS", &maxFPS, 1, 120);
    if (tooltips && ImGui::IsItemHovered()) 
        ImGui::SetTooltip("Target FPS of the GUI");
    ImGui::SliderInt("previewFPS", &previewFPS, 1, 120);
    if (tooltips && ImGui::IsItemHovered()) 
        ImGui::SetTooltip("ttarget FPS for the preview image. Smaller = more detailed image while dragging sliders");
    ImGui::Checkbox("pause##isPaused", &tracer->isPaused);

    ImGui::Unindent(16.0f);
  }
  if (ImGui::CollapsingHeader("scene"))
  {
    //load preset

    if (selectedPreset.empty() && !presets.empty())
    {
        selectedPreset = presets.begin()->first;
    }

    const char* previewText = selectedPreset.c_str();

    if (ImGui::BeginCombo("Preset", previewText))
    {
        for (const auto& pair : presets)
        {
            const std::string& key = pair.first;
            bool isSelected = (selectedPreset == key);
            if (ImGui::Selectable(key.c_str(), isSelected))
            {
                selectedPreset = key;
                // callback
                userSceneText = presets[key];
                ReloadScene(presets[key]);
                ResetTrace();
            }

            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    if (ImGui::Button("copy_scene"))
    {
      PrintScene();
      CopyToClipboard(userSceneText.c_str());
    }
  }
  if (ImGui::CollapsingHeader("camera settings"))
  {
    ImGui::Indent(16.0f);

    if (ImGui::DragFloat3("position##cam_pos", tracer->camera.position.data(), 0.01f, -10000, 10000, "%.3f"))
    {
      tracer->camera.ResetViews();
      ResetTrace();
    }
    if (ImGui::DragFloat3("rotation##cam_rot", tracer->camera.displayRotation.data(), 0.01f, -180, 180, "%.3f"))
    {
      tracer->camera.rotation = EulerToQuat(tracer->camera.displayRotation);
      tracer->camera.ResetViews();
      ResetTrace();
    }
    if (ImGui::DragFloat("fov", &tracer->camera.ry, 0.01f, 0.01f, 1.0f, "%.2f"))
    {
      tracer->camera.UpdateFOV((float)image.w, (float)image.h);
      ResetTrace();
    }
    if (tooltips && ImGui::IsItemHovered()) 
        ImGui::SetTooltip("Field of view, aka zoom");

    if (ImGui::Checkbox("AA##use_AA", &tracer->use_AA))
    {
      ResetTrace();
    }
    if (tooltips && ImGui::IsItemHovered()) 
        ImGui::SetTooltip("Anti-Aliasing, for smoother edges");
    if (ImGui::Checkbox("DOF##depth_of_field", &tracer->depth_of_field))
    {
      ResetTrace();
    }
    if (tooltips && ImGui::IsItemHovered()) 
        ImGui::SetTooltip("A tilt-shift effect, making near ground and background blurry");
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
    if (tooltips && ImGui::IsItemHovered()) 
        ImGui::SetTooltip("Will render a side-by-side half dome for binocular viewing in VR");

    ImGui::Unindent(16.0f);
  }
  if (ImGui::CollapsingHeader("input"))
  {
    if (ImGui::Checkbox("can_receive_input", &tracer->camera.controlsEnabled))
    {
      tracer->camera.PurgeKeys();
    }
    if (tooltips && ImGui::IsItemHovered()) 
        ImGui::SetTooltip("Disable to prevent accidental trace resets");
    ImGui::Indent(16.0f);
    ImGui::Text("Keyboard Input Speed");
    ImGui::DragFloat("movement##move_speed", &tracer->camera.speedMove, 0.01f);
    ImGui::DragFloat("rotation##rot_speed", &tracer->camera.speedRot, 0.01f);
    ImGui::Text("Mouse Input Speed");
    ImGui::DragFloat("rotation##rot_speed_mouse", &tracer->camera.speedRotMouse, 0.01f);
    ImGui::DragFloat("movement##move_speed_mouse", &tracer->camera.speedMoveMouse, 0.01f);
    ImGui::DragFloat("zoom##zoom_speed_mouse", &tracer->camera.speedZoomMouse, 0.01f);

    ImGui::Unindent(16.0f);
  }
  if (ImGui::CollapsingHeader("fractals"))
  {
    ImGui::Indent(16.0f);

    for (size_t i = 0; i < tracer->objects_p.size(); i++)
    {
      if (dynamic_cast<Fractal*>(tracer->objects_p[i]) && tracer->objects_p[i]->RenderGenericGUI(i))
      {
        ResetTrace();
      }
    }

    ImGui::Unindent(16.0f);
  }
  if (ImGui::CollapsingHeader("lights"))
  {
    ImGui::Indent(16.0f);

    for (size_t i = 0; i < tracer->objects_p.size(); i++)
    {
      if (!dynamic_cast<Fractal*>(tracer->objects_p[i]) && tracer->objects_p[i]->material->isLight() && tracer->objects_p[i]->RenderGenericGUI(i))
      {
        ResetTrace();
      }
    }

    ImGui::Unindent(16.0f);
  }
  if (ImGui::CollapsingHeader("other objects"))
  {
    ImGui::Indent(16.0f);

    for (size_t i = 0; i < tracer->objects_p.size(); i++)
    {
      if (!dynamic_cast<Fractal*>(tracer->objects_p[i]) && !tracer->objects_p[i]->material->isLight() && tracer->objects_p[i]->RenderGenericGUI(i))
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
  userSceneText = "";
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

  bool update_pass = (image.trace_num - 1) % 10 == 0;
  float diff = tracer->TraceImage(image, update_pass, 1);
}

void loop()
{
  display->PollEvents();

  auto start_time = std::chrono::high_resolution_clock::now();

  if (!display->closed)
  {
    if (!tracer->isPaused)
    {
      previewed_this_frame = false;
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
    }
    DrawGUI();
    display->FinishDrawing();
  }

  display->UpdateEvent();

  if (display->active && !isEnteringText && tracer->camera.controlsEnabled)
  {
    if (tracer->camera.Update())
      ResetTrace();
  }

  auto end_time = std::chrono::high_resolution_clock::now();

  float ms = static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count());

  guiFPS = 1000 / ms;

  if (!tracer->isPaused)
  {
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
  ReadScene(presets["Tentacle"]);

  tracer->Finit();

  display->SetupWindow(tracer->requested_width, tracer->requested_height);
  
  init_imgui();

  // Allocate and clear an image array
  ResizeImages();

  tracer_mode = tracer->DefaultMode;
  
  userSceneText.reserve(4096);
  
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

  quit();

  return 0;
}
