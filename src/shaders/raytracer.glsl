#version 460 core

// ENUMS AS CONSTS
const uint PRIMITIVE_TYPE_PRIMITIVENONE = 0;
const uint PRIMITIVE_TYPE_SPHERE = 1;
const uint PRIMITIVE_TYPE_CUBE = 2;
const uint PRIMITIVE_TYPE_CYLINDER = 4;

const uint OP_TYPE_OPNONE = 0;
const uint OP_TYPE_OPUNION = 1;
const uint OP_TYPE_OPINTERSECTION = 2;
const uint OP_TYPE_OPDIFFERENCE = 4;

const uint ID_OP_TYPE_PRIMITIVE = 0;
const uint ID_OP_TYPE_OPERATION = 1;

// WORKGROUP LOCAL SIZES
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
layout(rgba32f, binding = 0) uniform image2D img_output;

// --- Camera Uniforms ---
uniform vec3 u_camera_pos;
uniform mat4 u_inv_view;

struct material {
  vec4 albedo;
  float spec;
  float _padding1;
  float _padding2;
  float _padding3;
};

// STRUCTS
struct primitive {
    uint type;       
    material material; 
    mat4 transform;  
};

struct operation {
  uint type;
  uint operand1;
  uint operand2;
  uint padding;
};

struct instruction {
    uint type; 
    uint id;
    uint padding1;
    uint padding2;
};

struct ray {
  vec3 origin;
  vec3 dir;
};

// --- ssbos ---
layout(std140, binding = 1) readonly buffer primitive_buffer {
  primitive primitives[];
};
layout(std140, binding = 2) readonly buffer operation_buffer {
  operation operations[];
};
layout(std140, binding = 3) readonly buffer instructions_buffer {
  instruction instructions[];
};

//If t_min > t_max, there is no intersection.
const vec2 NO_HIT_SPAN = vec2(1.0/0.0, -1.0/0.0); // (inf, -inf)

#define MAX_SPANS 8

struct span {
    vec2 interval;
    uint primitive_id;
    bool invert_normal;
};

struct interval_list {
    span spans[MAX_SPANS];
    int count;
};

// --- CSG & Intersection functions ---
vec2 intersect_unit_sphere(ray r) {
    vec3 ro = r.origin;
    vec3 rd = r.dir;

    float a = dot(rd, rd);
    float b = 2.0 * dot(ro, rd);
    float c = dot(ro, ro) - 1.0;
    float delta = b*b - 4.0*a*c;

    if (delta < 0.0) {
        return NO_HIT_SPAN;
    }
    float sqrt_delta = sqrt(delta);
    return vec2(-b - sqrt_delta, -b + sqrt_delta) / (2.0 * a);
}

vec2 intersect_box_AABB(ray r) {
  vec2 span = NO_HIT_SPAN; // initiaal value is no hit.

  vec3 box_min = vec3(-.5);
  vec3 box_max = vec3(.5);

  vec3 ro = r.origin;
  vec3 rd = r.dir;
  
  vec3 t0 = (box_min - ro) / rd; 
  vec3 t1 = (box_max - ro) / rd;

  vec3 tmin = min(t0,t1);
  vec3 tmax = max(t0,t1);

  // find the largest near point:
  float t_enter = max(max(tmin.x, tmin.y), tmin.z);
  float t_exit = min(min(tmax.x, tmax.y), tmax.z);

  if(t_exit < t_enter){
    return span; // NO HIT
  }

  return vec2(t_enter, t_exit);
}

vec2 intersect_cylinder(ray r) {
    vec2 ro = r.origin.xz;
    vec2 rd = r.dir.xz;

    float a = dot(rd, rd);
    float b = 2.0 * dot(ro, rd);
    float c = dot(ro, ro) - 1.0; // Radius is 1.0

    float disc = b * b - 4.0 * a * c;

    // If discriminant is negative, ray misses the infinite tube entirely
    if (disc < 0.0) return vec2(1.0, -1.0); 

    float sqrtDisc = sqrt(disc);
    float t_tube_enter = (-b - sqrtDisc) / (2.0 * a);
    float t_tube_exit  = (-b + sqrtDisc) / (2.0 * a);

    float t_cap_bottom = (-0.5- r.origin.y) / r.dir.y;
    float t_cap_top    = ( 0.5 - r.origin.y) / r.dir.y;

    float t_cap_enter = min(t_cap_bottom, t_cap_top);
    float t_cap_exit  = max(t_cap_bottom, t_cap_top);

    float t_enter = max(t_tube_enter, t_cap_enter);
    float t_exit  = min(t_tube_exit,  t_cap_exit);

    return vec2(t_enter, t_exit);
}

bool is_inside(int op, bool in_a, bool in_b) {
    if (op == OP_TYPE_OPUNION)        return in_a || in_b;
    if (op == OP_TYPE_OPINTERSECTION) return in_a && in_b;
    if (op == OP_TYPE_OPDIFFERENCE)   return in_a && !in_b;
    return false;
}

interval_list merge_spans(interval_list l_a, interval_list l_b, int op){
  interval_list result;
  result.count = 0;

  int i = 0;
  int j = 0;
  bool in_a, in_b = false;
  bool last_in_result = false;

  float t_start = 0.0;
  uint  start_prim_id = 0;
  bool  start_inverted = false;

  while((i < l_a.count || j < l_b.count) && result.count < MAX_SPANS) {
    float t_a = 0.0;
    float t_b = 0.0;

    if(i < l_a.count) {
      if(in_a){
        t_a = l_a.spans[i].interval.y; // if inside take the end of the interval.
      }else {
        t_a = l_a.spans[i].interval.x;
      }
    }else{
      t_a = 1.0/0.0; // INF
    }

    if(j < l_b.count) {
      if(in_b){
        t_b = l_b.spans[j].interval.y; // if inside take the end of the interval.
      }else {
        t_b = l_b.spans[j].interval.x;
      }
    }else{
      t_b = 1.0/0.0; // INF
    }

    float current_t;
    uint current_prim;
    bool current_invert;

    // Pick the closest point
    if (t_a < t_b) {
      current_t = t_a;
      current_prim = l_a.spans[i].primitive_id;
      // Inherit inversion (if A was already inverted)
      current_invert = l_a.spans[i].invert_normal; 
      // Toggle state
      in_a = !in_a;
            
      // exited from A -> move to next A span index
      if (!in_a) i++; 
    } else {
      current_t = t_b;
      current_prim = l_b.spans[j].primitive_id;
            
      bool is_difference_operand = (op == OP_TYPE_OPDIFFERENCE);
      current_invert = is_difference_operand ? !l_b.spans[j].invert_normal : l_b.spans[j].invert_normal;

      // Toggle state
      in_b = !in_b;
      if (!in_b) j++; 
    }

    bool in_result = is_inside(op, in_a, in_b);

    if (in_result != last_in_result) {
        if (in_result) {
                t_start = current_t;
                start_prim_id = current_prim;
                start_inverted = current_invert;
        } else {
          if (current_t > t_start + 0.0001) {
            int idx = result.count;
            result.spans[idx].interval = vec2(t_start, current_t);
            result.spans[idx].primitive_id = start_prim_id;
            result.spans[idx].invert_normal = start_inverted;

            result.count++;
          }
        }
      last_in_result = in_result;
    }
  }

  return result;
}

interval_list make_primitive_interval(vec2 span, uint id) {
    interval_list list;
    if (span.x >= span.y) {
        list.count = 0; // Miss
    } else {
        list.count = 1;
        list.spans[0].interval = span;
        list.spans[0].primitive_id = id;
        list.spans[0].invert_normal = false; // Default
    }
    return list;
}

vec3 get_local_normal(uint type, vec3 p) {
    if (type == PRIMITIVE_TYPE_SPHERE) {
        // For a unit sphere at (0,0,0), normal is just the point itself
        return normalize(p);
    }
    else if (type == PRIMITIVE_TYPE_CUBE) {
        vec3 center = vec3(0);
        vec3 dist = p - center;
        vec3 abs_dist = abs(dist);
        
        // Find the dominant axis (x, y, or z)
        float max_axis = max(max(abs_dist.x, abs_dist.y), abs_dist.z);
        
        // Return normal along that axis (e.g., vec3(1,0,0) or vec3(0,-1,0))
        // using step() avoids branching.
        return normalize(step(vec3(max_axis - 0.0001), abs_dist) * sign(dist));
    }
    else if (type == PRIMITIVE_TYPE_CYLINDER) {
        // Cylinder: Y is [-.5, .5], Radius is 1
        // Check if we are on the top/bottom caps
        if (abs(p.y) > 0.499) {
            return vec3(0.0, sign(p.y), 0.0);
        }
        // Otherwise we are on the side tube
        return normalize(vec3(p.x, 0.0, p.z));
    }
    return vec3(0.0, 1.0, 0.0); // Default fallback
}

void main() {
  ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);
  ivec2 dims = imageSize(img_output);

  if (pixel_coords.x >= dims.x || pixel_coords.y >= dims.y) {
    return;
  }

  // --- Ray Generation ---
  vec2 uv = vec2(pixel_coords) / vec2(dims);
  uv = uv * 2.0 - 1.0;
  uv.x *= float(dims.x) / float(dims.y);

  ray r;
  r.origin = u_camera_pos;

  // --- Final Color ---
  vec3 color = vec3(0.5, 0.7, 1.0); // Sky blue background
  vec3 light_dir = normalize(vec3(0.5, 1.0, 0.8)); // Light direction
  // Calculate world-space ray direction from camera through the view plane
  vec4 view_space_target = vec4(uv.x, uv.y, -1.0, 1.0);
  vec4 world_space_target = u_inv_view * view_space_target;
  r.dir = normalize(world_space_target.xyz / world_space_target.w - r.origin);

  interval_list stack[16];
  int sp = 0;

  uint num_id_ops = instructions.length();
  for (uint i = 0; i < num_id_ops; i++) {
      instruction inst = instructions[i];
      if (inst.type == ID_OP_TYPE_PRIMITIVE) {
          primitive p = primitives[inst.id];
          
          // Transform ray into primitive's object space
          mat4 inv_transform = inverse(p.transform);

          ray transformed_ray;
          transformed_ray.origin = (inv_transform * vec4(r.origin, 1.0)).xyz;
          transformed_ray.dir = (inv_transform * vec4(r.dir, 0.0)).xyz;

          vec2 hit_span = NO_HIT_SPAN;
          if (p.type == PRIMITIVE_TYPE_SPHERE) {
              hit_span = intersect_unit_sphere(transformed_ray);
          }
          else if (p.type == PRIMITIVE_TYPE_CUBE) {  
              hit_span = intersect_box_AABB(transformed_ray);
          }
          else if(p.type == PRIMITIVE_TYPE_CYLINDER){  
              hit_span = intersect_cylinder(transformed_ray);
          }
          
          stack[sp++] = make_primitive_interval(hit_span, inst.id);

      } else if (inst.type == ID_OP_TYPE_OPERATION) {
          // CALCULATE BRANCH SPANS
          interval_list op2 = stack[--sp];
          interval_list op1 = stack[--sp];
          operation op = operations[inst.id];
     
          // MERGE THEM
          stack[sp++] = merge_spans(op1, op2, int(op.type));
      }
  }


if (sp > 0) {
        interval_list final_list = stack[0]; // The result of the whole tree

        float t_closest = 1.0 / 0.0; // Infinity
        int best_idx = -1;

        for (int k = 0; k < final_list.count; k++) {
            float t_enter = final_list.spans[k].interval.x;
            if (t_enter > 0.001 && t_enter < t_closest) {
                t_closest = t_enter;
                best_idx = k;
            }
        }

        if (best_idx != -1) {
            span hit = final_list.spans[best_idx];
            primitive prim = primitives[hit.primitive_id];

            vec3 world_pos = r.origin + r.dir * t_closest;

            mat4 inv_mat = inverse(prim.transform);
            vec3 local_pos = (inv_mat * vec4(world_pos, 1.0)).xyz;

            vec3 local_normal = get_local_normal(prim.type, local_pos);

            mat3 normal_matrix = transpose(inverse(mat3(prim.transform)));
            vec3 world_normal = normalize(normal_matrix * local_normal);

            if (hit.invert_normal) {
                world_normal = -world_normal;
            }

            float ambient = 0.2;
            float diffuse = max(0.0, dot(world_normal, light_dir));
            
            vec3 view_dir = normalize(u_camera_pos - world_pos);
            vec3 reflect_dir = reflect(-light_dir, world_normal);
            float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 32.0) * prim.material.spec;
            vec3 albedo = prim.material.albedo.rgb;

            color = albedo * (ambient + diffuse) + vec3(spec);
        }
    }
    imageStore(img_output, pixel_coords, vec4(color, 1.0));
}
