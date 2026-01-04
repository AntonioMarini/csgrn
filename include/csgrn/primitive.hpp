#ifndef PRIMITIVE_H
#define PRIMITIVE_H

#include "csgrn/material.hpp"
#include <glm/glm.hpp>

enum class primitive_types {
    none = 0,
    sphere = 1,
    cube = 2,
    cylinder = 4
};

// Use alignas to be safe
struct alignas(16) primitive {
    primitive_types type;  // 4 byte
    material mat; 
    glm::mat4 transform;
};

#endif // PRIMITIVE_H
