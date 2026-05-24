#include "Camera.h"
#include "Utilities.h"
//#include <windows.h>

Camera::Camera()
{
  moveKeyAmts[ImGuiKey_A] = Eigen::Vector3f(-1, 0, 0);
  moveKeyAmts[ImGuiKey_D] = Eigen::Vector3f(1, 0, 0);
  moveKeyAmts[ImGuiKey_E] = Eigen::Vector3f(0, 0, -1);
  moveKeyAmts[ImGuiKey_Q] = Eigen::Vector3f(0, 0, 1);
  moveKeyAmts[ImGuiKey_W] = Eigen::Vector3f(0, -1, 0);
  moveKeyAmts[ImGuiKey_S] = Eigen::Vector3f(0, 1, 0);

  rotKeyAmts[ImGuiKey_J] = Eigen::Vector3f(0, 1, 0);
  rotKeyAmts[ImGuiKey_L] = Eigen::Vector3f(0, -1, 0);
  rotKeyAmts[ImGuiKey_K] = Eigen::Vector3f(-1, 0, 0);
  rotKeyAmts[ImGuiKey_I] = Eigen::Vector3f(1, 0, 0);
  rotKeyAmts[ImGuiKey_O] = Eigen::Vector3f(0, 0, -1);
  rotKeyAmts[ImGuiKey_U] = Eigen::Vector3f(0, 0, 1);
}

void Camera::SetProperties(Eigen::Quaternionf r, Eigen::Vector3f p, float _ry, float w, float h, float fov_amt, float fov_dist)
{
  ry = _ry;
  rx = ry * w / h;
  position = p;
  rotation = r;
  f = fov_dist;
  w = fov_amt;
  ResetViews();
}

void Camera::Move(Eigen::Vector3f p)
{
  position += rotation._transformVector(p * speedMove);
}

void Camera::Rotate(Eigen::Vector3f r)
{
  rotation *= EulerToQuat(r * speedRot);
  displayRotation = QuatToEuler(rotation);
  ResetViews();
}

void Camera::ResetViews()
{
  ViewX = rx * rotation._transformVector(Eigen::Vector3f::UnitX());
  ViewY = ry * rotation._transformVector(Eigen::Vector3f::UnitY());
  ViewZ = -1.f * rotation._transformVector(Eigen::Vector3f::UnitZ());
}

std::string Camera::GetCameraString()
{
  auto euler = QuatToEuler(rotation);
  return std::string("camera ") + std::to_string(position.x()) + " " + std::to_string(position.y()) + " " + std::to_string(position.z()) + " " + std::to_string(ry) + " " + std::to_string(euler.x()) + " " + std::to_string(euler.y()) + " " + std::to_string(euler.z()) + " " + std::to_string(w) + " " + std::to_string(f);
}

void Camera::UpdateFOV(float w, float h)
{
  rx = ry * w / h;
  ResetViews();
}

bool IsKeyDown(ImGuiKey key)
{
  return ImGui::IsKeyDown(key);
}

bool Camera::Update()
{
  bool camera_moved = false;

  for (auto m : moveKeyAmts)
  {
    if (IsKeyDown(m.first))
    {
      Move(m.second);
      camera_moved = true;
    }
  }

  for (auto r : rotKeyAmts)
  {
    if (IsKeyDown(r.first))
    {
      Rotate(r.second);
      camera_moved = true;
    }
  }

  return camera_moved;
}

void Camera::PurgeKeys()
{
  for (auto m : moveKeyAmts)
  {
    IsKeyDown(m.first);
  }
  for (auto r : rotKeyAmts)
  {
    IsKeyDown(r.first);
  }
}
