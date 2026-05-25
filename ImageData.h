#pragma once
#include "Utilities.h"
#include <chrono>

struct ImageData
{
  int w;
  int h;
  size_t trace_num;
  std::vector<Color> data;
  int currX;
  int currY;
  int nPathsPerTrace = 1;

  // for progress bar
  float pctComplete;
  float pixel_num;

  float diff, cachedDiff;

  float traceDuration = 0;
  std::chrono::time_point<std::chrono::high_resolution_clock> start_time;

  void Resize(int _w, int _h)
  {
    w = _w;
    h = _h;
    data.resize(w * h);
    Clear();
  }

  void Clear()
  {
    for (int y = 0; y < h; y++)
      for (int x = 0; x < w; x++)
        data[y * w + x] = Color(0);
    trace_num = 1;
    currX = 0;
    currY = 0;
    diff = 0;
    cachedDiff = 0;
    start_time = std::chrono::high_resolution_clock::now();
  }
};