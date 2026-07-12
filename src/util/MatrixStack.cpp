#include "MatrixStack.hpp"
#include <glm/gtc/matrix_transform.hpp>

MatrixStack::MatrixStack() {
    stack = std::make_shared< std::stack<glm::mat4> >();
    stack->push(glm::mat4(1.0f));
}

MatrixStack::~MatrixStack() {
}

void MatrixStack::push() {
    stack->push(stack->top());
}

void MatrixStack::pop() {
    assert(!stack->empty());
    stack->pop();
    // Stack cannot be empty after popping.
    assert(!stack->empty());
}

glm::mat4& MatrixStack::top() {
    return stack->top(); 
} 

void MatrixStack::translate(const glm::vec3& t) {
    stack->top() = glm::translate(stack->top(), t);
}

void MatrixStack::translate(float x, float y, float z) {
    stack->top() *= glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
}

void MatrixStack::rotate(float angleDeg, const glm::vec3& axis) {
    stack->top() *= glm::rotate(glm::mat4(1.0f), glm::radians(angleDeg), axis);
}

void MatrixStack::scale(float s) {
    stack->top() *= glm::scale(glm::mat4(1.0f), glm::vec3(s));
}

void MatrixStack::scale(const glm::vec3& s) {
    stack->top() *= glm::scale(glm::mat4(1.0f), s);
}

/* Shears the top matrix given a shear factor k, direction vector d.
 * I + k(d)(n^T), where n is (vec(1) - d)
 * https://math.stackexchange.com/questions/2229844/
 */
void MatrixStack::shear(float k, const glm::vec3& d) {
    // Matrices in GLM are in column-major order.
    // M[3][0] -> 4rd column, 1st row.
    glm::mat4 m(1.0f);
    glm::vec3 n = glm::vec3(1.0f) - d;
    // For each coordinate vector
    for (int col = 0; col < 3; ++col) {
        // For each component
        for (int row = 0; row < 3; ++col) {
            // implicit inner product
            m[col][row] += k*d[row]*n[col];
        }
    }
    this->mult(m);
}

void MatrixStack::mult(const glm::mat4& oth) {
    stack->top() *= oth;
}

void MatrixStack::reset() {
    while (!stack->empty()) stack->pop();
    stack->push(glm::mat4(1.0f));
}