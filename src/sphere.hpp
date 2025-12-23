#ifndef SPHERE_H
#define SPHERE_H

#include "material.hpp"

struct Sphere {
  glm::vec3 center;
  float radius;
  Material material;
};

#endif // !SPHERE_H
