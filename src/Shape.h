#pragma once
#include "stn.h"
#include "Material.h"
#include "Ray.h"

class Shape {
public:
	Shape();
	virtual ~Shape();

	void setModelMatrix(const glm::mat4& m);
	glm::mat4 getModelMatrix() { return modelMat; }
	void setMaterial(const std::shared_ptr<Material>& mat) { material = mat; }

	virtual void intersect(const Ray& ray, std::vector<Hit>& hits) = 0;
	
protected:
	// Initialized after calling setModelMatrix(),
    // used as precomputed matrices
	glm::mat4 modelMat;
	glm::mat4 inv_modelMat;
	glm::mat4 invT_modelMat;

	std::shared_ptr<Material> material;

    void transform();
};

/* Represents a sphere.
 * intersect() returns either 0 or 2 hits representing the
 * front and back hits respectively.
*/
class Sphere : public Shape {
public:
	Sphere();
	virtual ~Sphere();
	void intersect(const Ray& ray, std::vector<Hit>& hits) override;
};

/* Represents a plane.
 * intersect() always return a hit, even if the ray never
 * actually hits the plane. 
 * The 2nd and 4th columns of the model matrix are used for
 * the plane's normal and position respectively, which allows
 * for flexibility when using a MatrixStack for transforms. 
 */
class Plane : public Shape {
public:
	Plane();
	virtual ~Plane();
	void intersect(const Ray& ray, std::vector<Hit>& hits) override;
	void setNormal(const glm::vec3& r) { 
        modelMat[1] = vec4(r, 0.0f);
        computeUVvectors(r); 
    }
    void setPosition(const glm::vec3& r) { modelMat[3] = vec4(r, 1.0f); }
private:
    glm::vec3 uvec, vvec;
    bool computedUVvectors = false;
    void Plane::computeUVvectors(const glm::vec3& n);
};

// class Mesh : public Shape {
// public:
// 	Mesh();
// 	virtual ~Mesh();
// 	void loadMesh(const std::string &meshName, bool = false);
// 	void fitToUnitBox();
// 	void intersect(const Ray& ray, std::vector<Hit>& hits) override;

// private:
// 	// These buffers are only populated when a mesh is loaded.
// 	std::vector<float> posBuf;
// 	std::vector<float> norBuf;
// 	std::vector<float> texBuf;

// 	double boundingRadius;
// 	glm::vec3 meshCenter; // Defined in model/local space.
// 	glm::mat4 inv_sphereMat; // matrix used for bounding sphere tests 
// 	glm::mat4 sphereMat; // only useful for debugging
// 	glm::mat4 invT_sphereMat; // likewise

// 	std::vector<float> getBoundingBox();
// 	void setBoundingRadius();
// };


// enum class OperationType{Intersection, Union, Difference};

// class CSG : public Shape {
// public:
// 	/*
// 	intersection = 0
// 	difference = 1
// 	union = 2
// 	*/
// 	OperationType operationType;
// 	std::shared_ptr<Shape> left;
// 	std::shared_ptr<Shape> right;

// 	CSG();
// 	CSG(OperationType o, std::shared_ptr<Shape> l, std::shared_ptr<Shape> r) 
// 	: left{l}, right{r} { operationType = o; }
// 	virtual ~CSG();
// 	void intersect(const Ray& ray, std::vector<Hit>& hits) override;

// private:
// 	void filter_intersections(
// 		const float &lt_min, 
// 		const float &lt_max,
// 		const float &rt_min,
// 		const float &rt_max,   
// 		std::vector<Hit>& hits);
// 	void local_intersect(const Ray& ray, std::vector<Hit>& hits);
// };
