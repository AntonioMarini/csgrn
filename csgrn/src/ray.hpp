#ifndef RAY_H
#define RAY_H

#include <glm/glm.hpp>

using namespace glm;

class Ray {
public:
  vec3 origin;
  vec3 direction;

  vec3 shoot_at(float t) { return origin + t * direction; }
};

#endif // !RAY_H
