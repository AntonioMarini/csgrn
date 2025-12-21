#version 460 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
layout(rgba32f, binding = 0) uniform image2D imgOutput;

struct Ray {
  vec3 origin;
  vec3 dir;
};

struct Material {
  vec3 albedo;
  float spec;
};

struct Sphere {
  vec3 center;
  float radius;
  Material material;
};

struct HitRecord {
  bool hit;
  float t_in;
  float t_out;
  vec3 point;
  vec3 normal;
  Material material;
};

layout(std140, binding = 1) buffer SceneBuffer {
  Sphere sceneObjects[];
};
// Intersection functions

HitRecord hit_sphere(Ray r, Sphere s) {
  HitRecord rec;
  rec.hit = false;

  vec3 oc = r.origin - s.center;

  // find a,b,c for the quadratic equation solution formula
  float a = dot(r.dir, r.dir);
  float half_b = dot(oc, r.dir);
  float c = dot(oc, oc) - s.radius * s.radius;
  float delta = half_b * half_b - a * c;

  if (delta < 0.0) { // no solutions -> ray doesn't hit the sphere
    return rec;
  }

  // 1 solution == 0 -> ray hit tangent
  // 2 solutions -> ray enters and exit -> record t_in and t_out

  float square_delta = sqrt(delta);

  rec.t_in = (-half_b - square_delta) / a;
  rec.t_out = (-half_b + square_delta) / a;

  // if t_out is negative -> sphere is behind the eye
  if (rec.t_out < 0.0) {
    return rec;
  }
  rec.hit = true;

  // t_out positive but t_in negative -> the eye is inside the sphere -> closest t is t_out
  float t_closest = rec.t_in;
  if (t_closest < 0.001) {
    t_closest = rec.t_out;
  }

  rec.point = r.origin + r.dir * t_closest;
  rec.normal = normalize(rec.point - s.center);
  rec.material = s.material;

  return rec;
}

// Intersection funtions//

void main() {
  ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);
  ivec2 dims = imageSize(imgOutput);

  if (pixel_coords.x >= dims.x || pixel_coords.y >= dims.y) {
    return;
  }

  vec2 uv = vec2(pixel_coords) / vec2(dims);
  uv = uv * 2.0 - 1.0;
  uv.x *= float(dims.x) / float(dims.y); // Corrects aspect ratio

  Ray r;
  r.origin = vec3(0.0, 0.0, 1.0);
  r.dir = normalize(vec3(uv.x, uv.y, -1.0));

  Sphere sphere = sceneObjects[0];

  HitRecord rec = hit_sphere(r, sphere);

  vec3 color = vec3(0.5, 0.7, 1.0); // background color
  if (rec.hit) {
    // if hit calculate light using normal
    vec3 lightDir = normalize(vec3(-1.0, 1.0, 1.0));
    float lightIntensity = max(0.0, dot(rec.normal, lightDir));
    color = rec.material.albedo * lightIntensity;
  }

  imageStore(imgOutput, pixel_coords, vec4(color, 1.0));
}
