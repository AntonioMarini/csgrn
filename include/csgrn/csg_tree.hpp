#ifndef CSG_TREE_H
#define CSG_TREE_H

#include "csgrn/material.hpp"
#include "csgrn/primitive.hpp"
#include "glm/fwd.hpp"
#include <glm/glm.hpp>
#include <vector>
#include "csgrn/operations.hpp"
#include "csgrn/op_instruction.hpp"

struct csg_node {
  csg_node *left = nullptr;
  csg_node *right = nullptr;
 
  op_types op = op_types::op_none;
  primitive_types primitive = primitive_types::none;
  glm::mat4x4 transform = glm::mat4(1.0f);
  glm::vec3 color = glm::vec3(1.0f);

  bool is_leave() {
    return this->left==nullptr || this->right == nullptr;
  }

  // Constructor for Operations
    csg_node(op_types type, csg_node* l, csg_node* r) 
        : op(type), left(l), right(r) {}

    // Constructor for primitive
    csg_node(primitive_types type) 
        : primitive(type) {}
};

class csg_tree {
  public:
    csg_node root;
    csg_tree(csg_node root) : root{root}{};
    
    glm::uint flatten_tree(csg_node* node, std::vector<primitive>& primitives, 
                     std::vector<operation>& operations, 
                    std::vector<instruction>& id_ops){
      if (!node) return 255; // Safety check for null nodes

      if(node->is_leave()){ // it's a primitive
        glm::uint id = primitives.size();
        id_ops.push_back({(glm::uint)node_type::PRIMITIVE, id}); // Create a PRIMITIVE instruction
        primitive p;
        material m;
        m.albedo = glm::vec4(node->color, 1.0f);
        m.spec = 0.0;
        p.mat = m; // Assign the material
        
        p.transform = node->transform;
        p.type = node->primitive;
        primitives.push_back(p);
        return id;
      }

      operation op;
      op.type = (glm::uint)node->op;
      op.operand1 = flatten_tree(node->left, primitives, operations, id_ops);
      op.operand2 = flatten_tree(node->right, primitives, operations, id_ops);
      glm::uint id = operations.size();
      operations.push_back(op);
      id_ops.push_back({(glm::uint)node_type::OPERATION, id}); // Create an OPERATION instruction
      
      return id;
    }; 
};

#endif // !CSG_TREE_H
