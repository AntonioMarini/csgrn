#ifndef CSG_PARSER_HPP
#define CSG_PARSER_HPP

#include "csg_tree.hpp"
#include "csgrn/operations.hpp"
#include "csgrn/primitive.hpp"
#include "glm/ext/matrix_transform.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>
#include <iostream>

static const glm::mat4 z_to_y_up = glm::rotate(
      glm::mat4(1.0f), 
  glm::radians(-90.0f), 
      glm::vec3(1.0f, 0.0f, 0.0f)
);

class lexer {
public:
    static std::vector<std::string> tokenize(const std::string& input) {
        std::vector<std::string> tokens;
        std::string current;
        for (char c : input) {
            if (isspace(c)) continue;
            if (c == '{' || c == '}' || c == '(' || c == ')' || 
                c == '[' || c == ']' || c == ',' || c == ';' || c == '=') {
                if (!current.empty()) {
                    tokens.push_back(current);
                    current.clear();
                }
                tokens.push_back(std::string(1, c));
            } else {
                current += c;
            }
        }
        if (!current.empty()) tokens.push_back(current);
        return tokens;
    }
};

class csg_parser {
    std::vector<std::string> tokens;
    size_t pos = 0;

public:
    csg_parser(const std::string& input) {
        tokens = lexer::tokenize(input);
    }

    csg_node* parse() {
        if (tokens.empty()) {
             std::cerr << "[ERROR] No tokens to parse!" << std::endl;
             return nullptr;
        }
        return parse_exp();
    }

private:
    std::string peek() {
        if (pos < tokens.size()) return tokens[pos];
        return ""; 
    }

    std::string consume() {
        if (pos >= tokens.size()) {
            std::cerr << "[ERROR] Attempted to consume past EOF!" << std::endl;
            return ""; 
        }
        std::string t = tokens[pos++];
        return t;
    }

    void expect(const std::string& token) {
        std::string p = peek();
        if (p == token) {
            consume();
        } else {
            std::cerr << "[SYNTAX ERROR] Expected '" << token << "' but got '" << p << "' at pos " << pos << std::endl;
        }
    }

    csg_node* parse_exp() {
        if (pos >= tokens.size()) return nullptr;

        std::string token = peek();

        if (token == "union" || token == "difference" || token == "intersection") {
            consume(); // Eat the token
            return parse_op(token);
        } else if (token == "multmatrix") {
            consume();
            return parse_multimatrix();
        } else if (token == "color") {
            consume();
            return parse_color();
        } else if (token == "sphere") {
            consume();
            return parse_sphere();
        }else if(token == "cube"){
           consume();
           return parse_cube();
        } else if(token == "cylinder"){
           consume();
          return parse_cylinder();
        } else if (token == "}") {
          return nullptr;
        }
        
        std::cerr << "[ERROR] Unknown token in parse_exp: " << token << std::endl;
        consume(); // Skip unknown to avoid infinite loop
        return nullptr;
    }

    csg_node* parse_op(const std::string& typeStr) {
        
        op_types type = op_types::op_none;
        if (typeStr == "union") type = op_types::op_union;
        if (typeStr == "difference") type = op_types::op_difference;
        if (typeStr == "intersection") type = op_types::op_intersection;

        expect("("); expect(")"); expect("{");

        csg_node* left = parse_exp();
        
        if (!left) {
            std::cerr << "[ERROR] Left child of " << typeStr << " is NULL!" << std::endl;
            return nullptr;
        }
        
        if(peek() == ";") consume();
        
        csg_node* right = parse_exp();

        if (!right) {
             std::cerr << "[ERROR] Right child of " << typeStr << " is NULL!" << std::endl;
             return nullptr;
        }
        
        if(peek() == ";") consume();

        expect("}");

        return new csg_node(type, left, right);    
    }

    csg_node* parse_multimatrix() {
        expect("(");
        glm::mat4 mat = parse_matrix_data();
        
        expect(")"); 
        expect("{");

        csg_node* child = parse_exp();
        
        if (!child) {
            std::cerr << "[CRITICAL ERROR] multmatrix child is NULL. Cannot apply transform." << std::endl;
            return nullptr;
        }

        if(peek() == ";") consume();
        expect("}");

        //mat = glm::rotate(mat, glm::half_pi<float>(), glm::vec3(1,0,0));
        child->transform = mat * child->transform;
        
        return child;
    }

    csg_node* parse_color() {
        expect("(");
        expect("[");
        float r = std::stof(consume()); expect(",");
        float g = std::stof(consume()); expect(",");
        float b = std::stof(consume()); 
        if (peek() == ",") { consume(); consume(); } 
        expect("]");
        expect(")"); 
        expect("{");

        csg_node* child = parse_exp();
        
        if (!child) {
            std::cerr << "[CRITICAL ERROR] color child is NULL. Cannot apply color." << std::endl;
            return nullptr;
        }

        if(peek() == ";") consume();
        expect("}");

        child->color = glm::vec3(r, g, b);
        return child;
    }

    csg_node* parse_sphere() {
        csg_node* node = new csg_node(primitive_types::sphere);
        float radius = 1.0f; // Default radius

        expect("(");
        
        while(peek() != ")" && peek() != "") {
            std::string param = consume();
            if (param == "r") {
                if (peek() == "=") {
                    consume(); 
                    try {
                        radius = std::stof(consume());
                    } catch (...) {
                        std::cerr << "[ERROR] Could not parse sphere radius value." << std::endl;
                    }
                }
            } else {
                if (peek() == "=") {
                    consume();
                    consume();
                }
            }

            if (peek() == ",") {
                consume();
            }
        }

        expect(")");
        if(peek() == ";") consume();

        node->transform = glm::scale(node->transform, glm::vec3(radius));

        return node;
    }

csg_node* parse_cube() {
        std::cout << "[DEBUG] Parsing Cube" << std::endl;
        csg_node* node = new csg_node(primitive_types::cube); 
        
        glm::vec3 size(1.0f);
        bool center = true;  

        expect("(");
        
        while(peek() != ")" && peek() != "") {
            std::string param = consume();
            
            if (param == "size") {
                if (peek() == "=") consume(); 

                if (peek() == "[") {
                    consume();
                    float x = std::stof(consume()); expect(",");
                    float y = std::stof(consume()); expect(",");
                    float z = std::stof(consume()); 
                    expect("]");
                    size = glm::vec3(x, y, z);
                } else {
                    float s = std::stof(consume());
                    size = glm::vec3(s);
                }
            } 
            else if (param == "center") {
                if (peek() == "=") consume(); 
                std::string val = consume();
                if (val == "false") center = false;
            }

            if (peek() == ",") consume();
        }

        expect(")");
        if(peek() == ";") consume();

        node->transform = glm::scale(node->transform, size);

        if (!center) {
             node->transform = glm::translate(node->transform, glm::vec3(0.5f));
        }

        node->transform = z_to_y_up * node->transform;

        return node;
    }

csg_node* parse_cylinder() {
        csg_node* node = new csg_node(primitive_types::cylinder); 
        
        float h = 1.0f;       // Default height
        float r1 = 1.0f;
        float r2 = 1.0f;
        bool center = true;   // Default: centered at origin

        expect("(");
        
        while(peek() != ")" && peek() != "") {
            std::string param = consume();
            
            if (param == "h" || param == "height") {
                if (peek() == "=") consume(); 
                h = std::stof(consume());
            } 
            else if (param == "r1" || param == "radius") {
                if (peek() == "=") consume(); 
                r1 = std::stof(consume());
            }
            if (param == "r2") {
                if (peek() == "=") consume(); 
                r2 = std::stof(consume());
            }
            else if (param == "center") {
                if (peek() == "=") consume(); 
                std::string val = consume();
                if (val == "false") center = false;
            }

            if (peek() == ",") consume();
        }

        expect(")");
        if(peek() == ";") consume();

        node->transform = glm::scale(node->transform, glm::vec3(r1, h, r1));
        if (!center) {
             node->transform = glm::translate(node->transform, glm::vec3(0.0f, 0.5f, 0.0f));
        }

        return node;
    }

    glm::mat4 parse_matrix_data() {
        glm::mat4 mat(1.0f);
        expect("[");
        
        float temp[16];
        int idx = 0;

        for(int row = 0; row < 4; ++row) {
            expect("[");
            for(int col = 0; col < 4; ++col) {
                std::string valStr = consume();
                try {
                    temp[idx++] = std::stof(valStr);
                } catch (...) {
                    std::cerr << "[ERROR] Matrix parse failed on value: " << valStr << std::endl;
                    temp[idx-1] = 0.0f;
                }
                if(col < 3) expect(",");
            }
            expect("]");
            if(row < 3) expect(",");
        }
        expect("]");

        glm::mat4 rawZUpMat = glm::transpose(glm::make_mat4(temp));
                static const glm::mat4 z_to_y_up_inv = glm::inverse(z_to_y_up);

        return z_to_y_up * rawZUpMat * z_to_y_up_inv;
    } 
};

#endif
