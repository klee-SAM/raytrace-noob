#pragma once
#include "stn.hpp"
#include <stack>

class MatrixStack {
public:
    MatrixStack();
    virtual ~MatrixStack();

    void push();
    void pop();
    glm::mat4& top();

    void translate(const glm::vec3& t);
    void translate(float x, float y, float z);
    void rotate(float angleDeg, const glm::vec3& axis);
    void scale(float s);
    void scale(const glm::vec3& s);
    void shear(float a, const glm::vec3& d);
    
    void mult(const glm::mat4&);

    // Clears the stack, then pushes the identity matrix
    void reset();

private:
    std::shared_ptr<std::stack<glm::mat4>> stack;
};