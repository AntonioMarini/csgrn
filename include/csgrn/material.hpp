#ifndef MATERIAL_H
#define MATERIAL_H

#include <glm/glm.hpp>

struct alignas(16) material {
  glm::vec4 albedo;
  float spec;
  float _padding1;
  float _padding2;
  float _padding3;
};

#endif // !MATERIAL_H
