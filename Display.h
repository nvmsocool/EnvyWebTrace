#pragma once
#include "ImageData.h"
#include <GLES3/gl3.h>

struct GLFWwindow;

class Display
{
public:
  Display(){};

  GLFWwindow* window;
  GLuint textureID;

  int window_width, window_height;
  int render_width, render_height;
  int gui_width{ 350 };
  float top, bottom, left, right;
  int top_i, bottom_i, left_i, right_i;
  bool closed{ true};
  bool active{ false };

  void DrawArray(ImageData& id);
  void FinishDrawing();
  void UpdateEvent();
  void ReshapeWindow(int w, int h);
  void SetRenderSize(ImageData& id);
  void CalcImageViewport();

  void SetupWindow(int w, int h);
  void DestroyWindow();
  void ClearWindow();
  void Finish();
  void PollEvents();

private:
  void CreateTexture();
};