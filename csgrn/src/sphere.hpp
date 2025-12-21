#ifndef SPHERE_H
#define SPHERE_H

#include <cmath>
#include <glm/geometric.hpp>
#include <glm/glm.hpp>

#include "hittable.hpp"

using namespace glm;

class Sphere : Hittable {
public:
  vec3 center;
  float radius; // deafaults to 1
  Material material;

  vec3 getNormal(vec3 point) { // todo abstract to shape class/interface
    return normalize(center - point);
  }

  bool hit(const Ray &r, HitRecord &hit_record, float t_min,
           float t_max) override {
    vec3 oc = r.origin - center;

    auto a = dot(r.direction, r.direction);
    auto half_b = dot(oc, r.direction);
    auto c = dot(oc, oc);

    auto d = half_b * half_b - a * c;
    if (d < 0) { // 0 sol -> no hit
      return false;
    }
    auto sqrt_d = sqrt(d);

    auto root1 = (-half_b - sqrt_d) / a;
    auto root2 = (-half_b + sqrt_d) / a;

    return true;
  }
};

#endif // !SPHERE_H
