#ifndef MATERIAL_H
#define MATERIAL_H

#include <glm/glm.hpp>

using namespace glm;

struct Material {
  vec3 albedo;
  float spec;
};

#endif // !MATERIAL_H
