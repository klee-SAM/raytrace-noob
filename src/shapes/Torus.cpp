#include "../Shape.hpp"

void Torus::initialize() {
	// Refit the major and minor radii so that the torus
	// fits inside a unit sphere
	constexpr float targetRa = 1.f;
	const float initTotRad = tor.x+tor.y;
	const float ratio = targetRa / initTotRad;
	tor *= ratio;
}

vec4 Torus::computeNormal(const vec3& x0) const {
	vec3 innerCirclePoint = vec3(normalize(vec2(x0)) * tor.x, 0.f);
	return vec4(glm::normalize(x0 - innerCirclePoint), 0.f);
}

vec2 Torus::computeUV(const vec3& pos) const {
	return vec2(0.8f*glm::atan(pos.x, pos.y),
					 glm::atan(pos.z,glm::length(vec2(pos))-tor.y));
}

void Torus::intersect(const Ray& ray, HitArray& hits) {
	mat4 modelMat = this->getModelMatrix(ray.time);
	mat4 inv_modelMat = inverse(modelMat);

	vec3 pk = vec3(inv_modelMat*ray.pos);
	vec3 vx = vec3(inv_modelMat*ray.dir);
	vec3 vk = normalize(vx);

	// https://iquilezles.org/articles/intersectors/
	vec3 &ro = pk;
	vec3 &rd = vk;

	float po = 1.0;
    float Ra2 = tor.x*tor.x;
    float ra2 = tor.y*tor.y;
    float m = dot(ro,ro);
    float n = dot(ro,rd);

	// bounding sphere
	// https://www.shadertoy.com/view/3XdyRj
	// (i should've taken more from that source)
    {
        float r = tor.x+tor.y;
        float h = n*n - m + r*r;
        if (h < 0.0 || (n > 0.0 && r*r < m) ) return;
    }

    float k = (m + Ra2 - ra2)/2.0;
    float k3 = n;
    float k2 = n*n - Ra2*dot(vec2(rd),vec2(rd)) + k;
    float k1 = n*k - Ra2*dot(vec2(rd),vec2(ro));
    float k0 = k*k - Ra2*dot(vec2(ro),vec2(ro));
    
	// the bias needed here is way too specific, 
	// a more stable solver is needed
    if( fabs(k3*(k3*k3-k2)+k1) < 0.000625f )
    {
        po = -1.0;
        float tmp=k1; k1=k3; k3=tmp;
        k0 = 1.0/k0;
        k1 = k1*k0;
        k2 = k2*k0;
        k3 = k3*k0;
    }
    
    float c2 = k2*2.0 - 3.0*k3*k3;
    float c1 = k3*(k3*k3-k2)+k1;
    float c0 = k3*(k3*(c2+2.0*k2)-8.0*k1)+4.0*k0;
    c2 /= 3.0;
    c1 *= 2.0;
    c0 /= 3.0;
    float Q = c2*c2 + c0;
    float R = c2*c2*c2 - 3.0*c2*c0 + c1*c1;
    float h = R*R - Q*Q*Q;
    
    if( h > 0.0 )  
    {
        h = glm::sqrt(h);
        float v = glm::sign(R+h)*pow(fabs(R+h),1.0/3.0); // cube root
        float u = glm::sign(R-h)*pow(fabs(R-h),1.0/3.0); // cube root
        vec2 s = vec2( (v+u)+4.0*c2, (v-u)*sqrt(3.0));
        float y = glm::sqrt(0.5f*(length(s)+s.x));
        float x = 0.5*s.y/y;
        float r = 2.0*c1/(x*x+y*y);
        float t1 =  x - r - k3; t1 = (po<0.0)?2.0/t1:t1;
        float t2 = -x - r - k3; t2 = (po<0.0)?2.0/t2:t2;

        if( t1>0.0 ) {
			vec3 x1 = pk + t1*vk;
			Hit h1 = toWorldSpaceHit(x1, vx, modelMat, inv_modelMat, t1);
			hits.push_back(h1);
		}

        if( t2>0.0 ) {
			vec3 x2 = pk + t2*vk;
			Hit h2 = toWorldSpaceHit(x2, vx, modelMat, inv_modelMat, t2);
			hits.push_back(h2);
		}

        return;
    }
    
    float sQ = sqrt(Q);
    float w = sQ*cos( acos(-R/(sQ*Q)) / 3.0 );
    float d2 = -(w+c2); if( d2<0.0 ) return;
    float d1 = sqrt(d2);
    float h1 = sqrt(w - 2.0*c2 + c1/d1);
    float h2 = sqrt(w - 2.0*c2 - c1/d1);
	float t[4];
    t[0] = -d1 - h1 - k3; 
    t[1] = -d1 + h1 - k3;
    t[2] =  d1 - h2 - k3;
    t[3] =  d1 + h2 - k3;
	for (int i = 0; i < 4; ++i) {
		t[i] = (po<0.0)?2.0/t[i]:t[i];
		if (t[i] < 0.f) continue;
		vec3 x = pk + t[i]*vk;
		Hit h = toWorldSpaceHit(x, vx, modelMat, inv_modelMat, t[i]);
		hits.push_back(h);
	}
}
