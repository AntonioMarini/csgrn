#ifndef HITRECORD_H
#define HITRECORD_H

#include <glm/glm.hpp>

#include "material.hpp"

using namespace glm;

struct HitRecord {
  bool hit;
  float t_in;
  float t_out;
  vec3 point;
  vec3 normal;
  Material material;
};

#endif // !HITRECORD_H
