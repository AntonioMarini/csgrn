#ifndef OPERATIONS_H
#define OPERATIONS_H

#include <glm/glm.hpp>
#include <string>

enum class op_types { op_none=0, op_union = 1, op_intersection = 2, op_difference = 4 };

// Operation always defined between 2 operands ids.
struct alignas(16) operation {
  glm::uint type;
  glm::uint operand1;
  glm::uint operand2;

  static std::string get_op_name(op_types type) {
    switch (type) {
      case op_types::op_union: return "Union";
      case op_types::op_intersection: return "Intersection";
      case op_types::op_difference: return "Difference";
      default: return "Unknown ()";
    }
  }
};


#endif
