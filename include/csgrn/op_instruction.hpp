#ifndef OP_INSTRUCTION_H
#define OP_INSTRUCTION_H

#include <glm/glm.hpp>

enum class node_type {
    PRIMITIVE,
    OPERATION
};

// Represents a single instruction for the GPU to execute when evaluating the CSG tree
struct alignas(16) instruction {
    glm::uint type;
    glm::uint id;
};

#endif // OP_INSTRUCTION_H
