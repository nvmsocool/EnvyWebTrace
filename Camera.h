#pragma once
#include <unordered_map>
#include "libs/Eigen/Geometry"
#include "imgui.h"

class Camera
{
public:
  Camera();
  ~Camera(){};

  //position, rotation, ry
  float ry, rx;
  Eigen::Vector3f position, displayRotation, ViewX, ViewY, ViewZ;
  Eigen::Quaternionf rotation;

  std::unordered_map<ImGuiKey, Eigen::Vector3f> moveKeyAmts;
  std::unordered_map<ImGuiKey, Eigen::Vector3f> rotKeyAmts;

  //camera movement settings
  float speedMove = 0.01f;
  float speedRot = 2.f;

  float speedMoveMouse = 1.f;
  float speedRotMouse = 1.f;
  float speedZoomMouse = 1.f;


  bool controlsEnabled = true;

  // depth of field settings
  float f = 5.f;
  float w = 0.2f;

  void SetProperties(Eigen::Quaternionf r, Eigen::Vector3f p, float _ry, float w, float h, float fov_amt, float fov_dist);
  void Move(Eigen::Vector3f p);
  void Rotate(Eigen::Vector3f r);
  void ResetViews();
  std::string GetCameraString();
  void UpdateFOV(float w, float h);
  bool Update();
  bool UpdateMouse();
  void PurgeKeys();

private:
};
