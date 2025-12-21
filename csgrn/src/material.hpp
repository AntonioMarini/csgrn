#ifndef MATERIAL_H
#define MATERIAL_H

#include <glm/glm.hpp>

struct Material {
  glm::vec3 albedo;
  float spec;
};

#endif // !MATERIAL_H
