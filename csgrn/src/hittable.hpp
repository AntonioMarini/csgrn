#ifndef HITTABLE_H
#define HITTABLE_H

#include "ray.hpp"
#include "hit_record.hpp"

class Hittable {
public:
  virtual bool hit(const Ray &r, HitRecord &hit_record, float t_min,
                   float t_max);
};

#endif
