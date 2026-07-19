#include "../Shape.hpp"

using CONSTANTS::EPSILION;
using std::vector;

void pushIntervals(vector<Interval>& intervals, const HitArray& hits) {
	if (hits.empty()) {
		intervals.push_back(Interval::empty());
		return;
	}
	
	uint i = 0;
	if (hits.size() % 2 == 1) {
		// first hit originates inside the csg, which
		// occurs with rays other than primary
		intervals.push_back(Interval(0.0f, hits.at(0).t));
		i = 1;
	}

	// assume the hits list is sorted and is for one shape;
	// each pair represents enter and exit points for primitives
	for (; i < hits.size(); i += 2) {
		intervals.push_back(Interval(hits.at(i).t, hits.at(i+1).t));
	}
}

void CSG::intersect(const Ray& ray, HitArray& hits) {
	mat4 modelMat = getModelMatrix(ray.time);
	mat4 inv_modelMat = inverse(modelMat);

	Ray mray;
	mray.pos = inv_modelMat * ray.pos;
	mray.dir = inv_modelMat * ray.dir;
	mray.time = ray.time;

	HitArray rightHits;
	this->left->intersect(mray, hits);
	this->right->intersect(mray, rightHits);

	vector<Interval> l_intervals;
	vector<Interval> r_intervals; 

	l_intervals.reserve(hits.max_size());
	r_intervals.reserve(rightHits.max_size());
	
	pushIntervals(l_intervals, hits);
	pushIntervals(r_intervals, rightHits);
	
	for (auto& it : rightHits) {
		// ensure that faces in difference csgs are properly lit
		if (operationType == OperationType::Difference) it.n = -it.n;
		// it barely makes a difference whether this copied or not,
		// so just leave this alone
		hits.push_back(std::move(it));
	}
	// Hit::sortHits(hits);
	hits.sort();

	filter_intersections(l_intervals, r_intervals, hits);

	// need to transform the hits back from CSG space to world space
	mat4 invT_modelMat = transpose(inv_modelMat);
	for (auto &hit : hits) {
		hit.x = modelMat*vec4(hit.x, 1.f);
		hit.n = normalize(invT_modelMat*vec4(hit.n, 0.f));
	}
}

bool intersection_allowed(OperationType op, bool inL, bool inR) {
	switch (op) {
	case OperationType::Union:
		return (inL && !inR) || (!inL && inR);
	case OperationType::Intersection:
		return (inL && inR); 
	case OperationType::Difference:
		return (inL && !inR);
	default:
		break;
	}
	return false;
}

bool insideIntervalAfter(
	const vector<Interval>& intervals,
	float t, float s = -1.0f) 
{
	for (auto& interval : intervals) {
		// Don't consider any intervals before this 
		// distance t in the ray; this is why a
		// loop over intervals is done 
		if (t > interval.max) continue;

		return (interval.min - t < EPSILION) && 
			   (interval.max - t > s*EPSILION);
	}
	return false;
}

void CSG::filter_intersections(
	const vector<Interval>& l_intervals,
	const vector<Interval>& r_intervals, 
	HitArray& hits)
{	
	bool inl = false, inr = false; 
	// unoptimal way to make difference show right shape's faces; hacky
	float s = this->operationType == OperationType::Difference ? 1.0f : -1.0f;
	HitArray new_hits;
	for (auto& hit : hits) {
		inl = insideIntervalAfter(l_intervals, hit.t, s);
		inr = insideIntervalAfter(r_intervals, hit.t, s);
		// Assume that the given hits list is sorted in ascending order
		// of t; that is, the first intersection is outside both shapes
		if (intersection_allowed(this->operationType, inl, inr)) {
			new_hits.push_back(hit);
		}
	}
	hits = new_hits;
}