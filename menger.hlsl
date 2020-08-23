#version 440 core

out vec4 fragColor;

// OpenGL params
uniform vec2 resolution;
uniform vec3 direction;
uniform vec3 position = vec3(0, 0, 5);
uniform vec3 up;
uniform vec3 right;
uniform float time;
uniform float currentFOV = 45.f;


// Raymarching params
const int MAX_STEPS = 300;
const float MIN_STEP = 0.0001;
const float MAX_DIST = 300.0;
const float MIN_DIST = 0.0;
int step = 0;

// SDFs
const int iters = 20;
const float max_r = 100.0;



float sphereSDF(vec3 p) {
	return length(p) - 1.0;
}


float torusSDF(vec3 p, vec2 t)
{
	vec2 q = vec2(length(p.xz) - t.x, p.y);
	return length(q) - t.y;
}

float floorSDF(vec3 p) {
	return p.y + 6.0;
}

float unionSDF(float a, float b) {
	return min(a, b);
}


float boxSDF(vec3 p, vec3 b) {
	vec3 q = abs(p) - b;
	return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}


float crossSDF(in vec3 p) {
	float da = boxSDF(p.xyz, vec3(10e15, 1.0, 1.0));
	float db = boxSDF(p.yzx, vec3(1.0, 10e15, 1.0));
	float dc = boxSDF(p.zxy, vec3(1.0, 1.0, 10e15));
	return min(da, min(db, dc));
}


float menguerSDF(in vec3 p) {
	float d = boxSDF(p, vec3(1));

	float s = 1;
	

	for (int m = 0; m < 4; m++)
	{
		vec3 a = mod(p * s, 2.0) - 1.0;
		s *= 3.0;
		vec3 r = abs(1.0 - 3.0 * abs(a));

		float da = max(r.x, r.y);
		float db = max(r.y, r.z);
		float dc = max(r.z, r.x);
		float c = (min(da, min(db, dc)) - 1.0) / s;

		d = max(d, c);

		//p += vec3(cos(time * 0.03) * 1, cos(time * 0.03) * 1, 1);
	}
	return d;

}


// Scene to render
float sceneSDF(vec3 p) {

	return unionSDF(menguerSDF(p), floorSDF(p));
}


// Raymarching core
float marchRay(vec3 eye, vec3 marchDir, float start, float end) {
	float dist = start;
	for (int i = 0; i < MAX_STEPS; i++) {
		// Get the step size 
		float step = sceneSDF(eye + dist * marchDir);

		// We're *inside*, stop
		if (abs(step) < MIN_STEP) {
			return dist;
		}

		dist += step;

		// Stop
		if (dist >= end) {
			return end;
		}

		step += 1;
	}
	return end;
}


// Determine ray direction
vec3 rayDir(float FOV, vec2 res, vec2 fragCoord) {
	// Vector on the view plane
	vec2 xy = fragCoord - res / 2.0;

	// Axial distance from plane to scene
	float z = 0.5 * res.y / tan(radians(FOV) / 2);

	// Return normalized vector from fragCoord to scene
	return normalize(vec3(xy, -z));
}

// Estimate normal by symmetric gradient approx
vec3 estimateNormal(vec3 p) {
	return normalize(vec3(
		sceneSDF(vec3(p.x + MIN_STEP, p.y, p.z)) - sceneSDF(vec3(p.x - MIN_STEP, p.y, p.z)),
		sceneSDF(vec3(p.x, p.y + MIN_STEP, p.z)) - sceneSDF(vec3(p.x, p.y - MIN_STEP, p.z)),
		sceneSDF(vec3(p.x, p.y, p.z + MIN_STEP)) - sceneSDF(vec3(p.x, p.y, p.z - MIN_STEP))
	));
}

// Phong model for lightning
// https://en.wikipedia.org/wiki/Phong_reflection_model
vec3 phongLight(vec3 kD, vec3 kS, float alpha, vec3 p, vec3 eye, vec3 lightPos, vec3 lightIntensity) {
	vec3 N = estimateNormal(p);
	vec3 L = normalize(lightPos - p);
	vec3 V = normalize(eye - p);
	vec3 R = reflect(-L, N);

	float dotLN = clamp(dot(L, N), 0, 1);
	float dotRV = dot(R, V);

	if (dotLN < 0.0) {
		return vec3(0, 0, 0);
	}

	if (dotRV < 0.0) {
		return lightIntensity * (kD * dotLN);
	}

	return lightIntensity * (kD * dotLN + kS * pow(dotRV, alpha));
}

vec3 sceneLight(vec3 p, vec3 eye) {
	// Scene colors
	
	vec3 kA = vec3(0.2, 0.2, 0.9 * cos(time * 0.7));
	vec3 kS = vec3(0.3);
	vec3 kD = vec3(0.1, 0.4, 0.7);
	float alpha = 100;
	

	// For Meguer

	// For the floor
	if (floorSDF(p) < MIN_STEP) {
		kA = vec3(1.0, 1.0, 1.0);
		kD = vec3(1, 1, 1);
		alpha = 50;
	}

	// White ambient light

	vec3 ambLight = 0.5 * vec3(1.0, 1.0, 1.0);
	vec3 light1Pos = vec3(10 * sin(time * 0.3), 10, 10);
	//vec3 light1Pos = position;
	vec3 light1Intensity = vec3(1.0);

	vec3 color = ambLight * kA;
	color += phongLight(kD, kS, alpha, p, eye, light1Pos, light1Intensity);

	// Shadow
	vec3 shadowDir = normalize(p - light1Pos);
	float shadowDist = marchRay(light1Pos, shadowDir, MIN_DIST, MAX_DIST);
	vec3 shadowPos = light1Pos + shadowDist * shadowDir;

	if (length(shadowPos - p) > sqrt(MIN_STEP)) {
		// We hitted something
		color *= 0.3;
	}

	return color;
}


// viewMatrix
mat4 viewMatrix(vec3 eye, vec3 center, vec3 up) {
	vec3 f = normalize(center - eye);
	vec3 s = normalize(cross(f, up));
	vec3 u = cross(s, f);

	return mat4(
		vec4(s, 0.0),
		vec4(u, 0.0),
		vec4(-f, 0.0),
		vec4(0.0, 0.0, 0.0, 1));
}



void main() {
	vec3 dir = rayDir(currentFOV, resolution.xy, gl_FragCoord.xy);
	mat4 lookAt = viewMatrix(position, direction + position, up);
	vec3 worldDir = (lookAt * vec4(dir, 0.0)).xyz;

	float dist = marchRay(position, worldDir, MIN_DIST, MAX_DIST);

	// Didn't hit anything, background
	if (dist > MAX_DIST - MIN_STEP) {
		fragColor = vec4(0, 0, 0, 0);
		return;
	}

	// Light
	vec3 p = position + dist * worldDir;
	vec3 color = sceneLight(p, position);

	fragColor = vec4(color, 1.0);
}