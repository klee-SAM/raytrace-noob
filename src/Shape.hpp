#pragma once
#include "stn.hpp"
#include "Material.hpp"
#include "Ray.hpp"

class Shape {
public:
	Shape() {}
	virtual ~Shape() = default;

	void setModelMatrix(const glm::mat4& m);
	glm::mat4 getModelMatrix() const { return modelMat; }
	void setMaterial(const std::shared_ptr<Material>& mat) { material = mat; }

	virtual void intersect(const Ray& ray, std::vector<Hit>& hits) = 0;
	
protected:
	// Initialized after calling setModelMatrix(),
    // used as precomputed matrices
	glm::mat4 modelMat;
	glm::mat4 inv_modelMat;
	glm::mat4 invT_modelMat;

	std::shared_ptr<Material> material;

	virtual glm::vec2 computeUV(const glm::vec3& point) const = 0;
	virtual glm::vec4 computeNormal(const glm::vec3& x) const = 0;
	Hit toWorldSpaceHit(
		const glm::vec3& x, // hit position
		const glm::vec3& vx, // unnormalized ray dir
		float t) const;
};

/* Represents a sphere.
 * intersect() returns either 0 or 2 hits representing the
 * front and back hits respectively.
*/
class Sphere : public Shape {
public:
	Sphere() {}
	virtual ~Sphere() = default;
	void intersect(const Ray& ray, std::vector<Hit>& hits) override;
protected:
	virtual glm::vec2 computeUV(const glm::vec3&) const override;
	virtual glm::vec4 computeNormal(const glm::vec3& x) const override;
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
	Plane() {}
	virtual ~Plane() = default;
	void intersect(const Ray& ray, std::vector<Hit>& hits) override;
	void setNormal(const glm::vec3& r) { 
        modelMat[1] = glm::vec4(r, 0.0f);
        computeUVvectors(r); 
    }
    void setPosition(const glm::vec3& r) { modelMat[3] = glm::vec4(r, 1.0f); }
protected:
	virtual glm::vec2 computeUV(const glm::vec3&) const override;
	virtual glm::vec4 computeNormal(const glm::vec3& x) const override;
private:
    glm::vec3 uvec, vvec;
    bool computedUVvectors = false;
    void computeUVvectors(const glm::vec3& n);
};

class Box : public Shape {
public:
	Box() {}
	virtual ~Box() = default;
	void intersect(const Ray& ray, std::vector<Hit>& hits) override;
protected:
	virtual glm::vec2 computeUV(const glm::vec3&) const override;
	virtual glm::vec4 computeNormal(const glm::vec3& x) const override;
};

class Cylinder : public Shape {
public:
	Cylinder() {}
	virtual ~Cylinder() = default;
	void intersect(const Ray& ray, std::vector<Hit>& hits) override;
protected:
	virtual glm::vec2 computeUV(const glm::vec3&) const override;
	virtual glm::vec4 computeNormal(const glm::vec3& x) const override;
};

class Mesh : public Shape {
public:
	Mesh() {};
	Mesh(const std::string& objName, const std::string& directory) {
		loadMesh(objName, directory, true);
	}
	virtual ~Mesh() = default;
	// Assume that the .obj file and .mtl files are in the same directory.
	void loadMesh(const std::string &meshName, 
				  const std::string &directoryPath, 
				  bool = false);
	void fitToUnitBox();
	void intersect(const Ray& ray, std::vector<Hit>& hits) override;
protected:
	virtual glm::vec2 computeUV(const glm::vec3&) const override;
	virtual glm::vec4 computeNormal(const glm::vec3& x) const override;
private:
	// Set to true when intersect() is called for the first time on the mesh.
	bool sphere_matrix_initialized = false;

	// These buffers are only populated when a mesh is loaded.
	std::vector<float> posBuf;
	std::vector<float> norBuf;
	std::vector<float> texBuf;

	double boundingRadius;
	glm::vec3 boundingScales;
	glm::vec3 meshCenter; 	  // Defined in model/local space.
	float meshOffset;
	glm::mat4 inv_sphereMat;  // matrix used for bounding sphere tests 
	glm::mat4 sphereMat; 	  // only useful for debugging
	glm::mat4 invT_sphereMat; // likewise

	void setBoundingRadius();
	bool intersect_triangle(const glm::vec3& o, const glm::vec3& d, 
							const size_t &off, float &t, float &u, 
							float &v);

	void initSphereMatrices();
};


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
