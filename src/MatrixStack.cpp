#include "MatrixStack.h"

MatrixStack::MatrixStack() {
    stack.push(glm::mat4(1.0f));
}

MatrixStack::~MatrixStack() {
}

void MatrixStack::push() {
    stack.push(stack.top());
}

void MatrixStack::pop() {
    stack.pop();
}

glm::mat4& MatrixStack::top() {
    return stack.top(); 
} 

// TODO: unit test matrix stack operations

void MatrixStack::translate(const glm::vec3& t) {
    stack.top() = glm::translate(stack.top(), t);
}

void MatrixStack::translate(float x, float y, float z) {
    stack.top() = glm::translate(stack.top(), glm::vec3(x, y, z));
}

void MatrixStack::rotate(float angleDeg, const glm::vec3& axis) {
    stack.top() = glm::rotate(stack.top(), glm::radians(angleDeg), axis);
}

void MatrixStack::scale(float s) {
    stack.top() = glm::scale(stack.top(), glm::vec3(s));
}

void MatrixStack::scale(const glm::vec3& s) {
    stack.top() = glm::scale(stack.top(), s);
}

/* Shears the top matrix given a shear factor k, direction vector d,
 * and normal vector n (that represents the plane).
 * https://math.stackexchange.com/questions/2229844/
 */
void MatrixStack::shear(float k, const glm::vec3& d, const glm::vec3& n) {
    // Matrices in GLM are in column-major order.
    // M[3][0] -> 4rd column, 1st row.
    glm::mat4 m(1.0f);
    // For each coordinate vector
    for (int col = 0; col < 3; ++col) {
        
        // For each component
        for (int row = 0; row < 3; ++col) {
            if (row == col) continue;
            // The idea is that the dir vector is
            // perpendicular to the plane to shear,
            // for example, if shearing in the x direction,
            // dir = (1, 0, 0), and only m[1][0] and m[2][0]
            // are equal to sh.
            // Of course, this calculation is extremely naive
            // and incorrect if dir has more than 1 non-zero 
            // component.
            m[col][row] = (1.0f - d[col]);
        }
    }
    this->mult(m);
}

void MatrixStack::mult(const glm::mat4& oth) {
    stack.top() *= oth;
}

void MatrixStack::reset() {
    while (!stack.empty()) stack.pop();
    stack.push(glm::mat4(1.0f));
}