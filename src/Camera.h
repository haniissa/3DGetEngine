/**
    @!file Camera.h
    @brief Orbit camera: rotate around the engine, zoom in/out

    Controls:
        - LeftMouse + Drag -> Orbit
        - Scrol Wheel      -> Zoom
        - Right Mouse + Drag -> Pan
*/

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
  Camera(float aspectRatio);
  void Update(float dt);

  // Input events (called from App::ProcessEvents)
  void OnMouseMove(float dx, float dy, bool leftButton, bool rightButton);
  void OnScroll(float delta);
  void OnResize(float width, float height);

  // Auto-rotate for showcase mode
  void SetAutoRotate(bool enabled) { m_autoRotate = enabled; }

  glm::mat4 GetViewMatrix() const;
  glm::mat4 GetProjectionMatrix() const;
  glm::vec3 GetPosition() const { return m_position; }

private:
  void Recalculate();

  glm::vec3 m_target = glm::vec3(2.9F, 0.0F, 0.0F);
  glm::vec3 m_position = glm::vec3(5.0F, 2.0F, 5.0F);
  glm::vec3 m_up = glm::vec3(0.0F, 1.0F, 0.0F);

  float m_distance = 14.0F;
  float m_yaw = 35.0F;  // Horizontal angle (degrees)
  float m_pitch = 22.0F; // Vertical angle (degrees)
  float m_aspectRation = 16.0F / 9.0F;
  float m_fov = 60.0F;

  bool m_autoRotate = false;
  float m_autoRotateSpeed = 5.0F;
};
