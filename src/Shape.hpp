#pragma once
#ifndef SHAPE_H
#define SHAPE_H

#include "stn.hpp"
#include "Material.hpp"
#include "Ray.hpp"
#include "MeshBuffer.hpp"

#include "util/umath.hpp"

#include <glm/gtc/quaternion.hpp>

#include <vector>

class Geometry {
public:
	Geometry() {}
	virtual ~Geometry() = default;
	virtual void initialize() {}
	virtual void intersect(const Ray&, HitArray& hits) = 0;
};

class Shape : public Geometry {
public:
	Shape() {}
	virtual ~Shape() = default;

	// Called when the shape needs to precompute matrices or other members
	// TODO: bounding box function 
	void initialize() {}

	// Sets the initial transform.
	void setModelMatrix(const glm::mat4& m) { modelMat = m; }
	// Sets the final transform, enabling motion blur 
	void setNextModelTransforms(const glm::vec3& trns,
								const glm::vec3& rot,
								const glm::vec3& scl);

	glm::mat4 getModelMatrix() const { return modelMat; }
	glm::mat4 getModelMatrix(float tm) const;
	void setMaterial(const std::shared_ptr<Material>& mat) { material = mat; }

	void intersect(const Ray& ray, HitArray& hits) {}
	
protected:
	// Initialized after calling setModelMatrix(),
    // used as precomputed matrix. Avoid caching other matrices, 
	// as that leads to slower times (cache misses?)
	glm::mat4 modelMat;

	std::shared_ptr<Material> material;
	
	// For moving objects; contains the transforms for an
	// object at the end of the time interval, which
	// is used by lerp() to easily compose a new matrix
	glm::quat m_rotation;
	glm::vec3 m_translation;
	glm::vec3 m_scale;

	// determines if new matrices need to be constructed
	// for this object
	bool moving = false;

	virtual glm::vec2 computeUV(const glm::vec3& point) const { 
		return glm::vec2(0.0f); 
	}
	virtual glm::vec4 computeNormal(const glm::vec3& x) const { 
		return glm::vec4(0.0f); 
	}
	// Hit toWorldSpaceHit(
	// 	const glm::vec3& x, // hit position
	// 	const glm::vec3& vx, // unnormalized ray dir
	// 	float t) const;

	Hit toWorldSpaceHit(
		const glm::vec3 &x, // hit position
		const glm::vec3 &vx, // unnormalized ray dir
		const glm::mat4 &model,
		const glm::mat4 &invMod,
		float t) const;

	glm::mat4 modelMatLerp(const float time) const;
};

/* Represents a sphere.
 * intersect() returns either 0 or 2 hits representing the
 * front and back hits respectively.
*/
class Sphere final : public Shape {
public:
	Sphere() {}
	virtual ~Sphere() = default;
	void intersect(const Ray& ray, HitArray& hits) override;
protected:
	glm::vec2 computeUV(const glm::vec3&) const override;
	glm::vec4 computeNormal(const glm::vec3& x) const override;
};

/* Represents a plane.
 * intersect() always return a hit, even if the ray never
 * actually hits the plane. 
 * The 2nd and 4th columns of the model matrix are used for
 * the plane's normal and position respectively, which allows
 * for flexibility when using a MatrixStack for transforms. 
 * Normal: modelMat[1] = glm::vec4(r, 0.0f);
 * Position: modelMat[3] = glm::vec4(r, 1.0f);
 */
class Plane final : public Shape {
public:
	Plane() {}
	virtual ~Plane() = default;
	void intersect(const Ray& ray, HitArray& hits) override;
	void initialize() override;
private:
    glm::vec3 uvec, vvec;
};

class Box final : public Shape {
public:
	Box() {}
	virtual ~Box() = default;
	void intersect(const Ray& ray, HitArray& hits) override;
protected:
	glm::vec2 computeUV(const glm::vec3&) const override;
	glm::vec4 computeNormal(const glm::vec3& x) const override;
};

class Cylinder final : public Shape {
public:
	Cylinder() {}
	virtual ~Cylinder() = default;
	void intersect(const Ray& ray, HitArray& hits) override;
protected:
	glm::vec2 computeUV(const glm::vec3&) const override;
	glm::vec4 computeNormal(const glm::vec3& x) const override;
};

class Torus final : public Shape {
public:
	Torus() {}
	virtual ~Torus() = default;
	void intersect(const Ray& ray, HitArray& hits) override;
	void initialize() override;
protected:
	glm::vec2 computeUV(const glm::vec3&) const override;
	glm::vec4 computeNormal(const glm::vec3& x) const override;
private:
	glm::vec2 tor = glm::vec2(.75f, .25f); // major radius, minor radius
};

class Mesh final : public Shape {
public:
	Mesh() noexcept {};
	Mesh(const std::shared_ptr<MeshBuffer>& mesh) noexcept { meshDat = mesh; }
	virtual ~Mesh() = default;
	void intersect(const Ray& ray, HitArray& hits) override;
private:
	std::shared_ptr<MeshBuffer> meshDat;
	glm::mat4 inv_boundMat;
	
	void initialize() override;
	bool intersect_triangle(const glm::vec3& o, const glm::vec3& d, 
							const size_t &off, float &t, float &u, 
							float &v);

	bool sphere_test(const Ray& ray);
};

enum class OperationType {None, Intersection, Union, Difference};

class CSG final : public Shape {
public:
	std::shared_ptr<Shape> left;
	std::shared_ptr<Shape> right;
	OperationType operationType;

	CSG() {};
	CSG(OperationType o, std::shared_ptr<Shape> l, std::shared_ptr<Shape> r) 
	: left{l}, right{r} { operationType = o; }
	virtual ~CSG() = default;
	void intersect(const Ray& ray, HitArray& hits) override;
private:
	void filter_intersections(
		const std::vector<Interval>& l_intervals,
		const std::vector<Interval>& r_intervals,
		HitArray& hits);
};

#endif