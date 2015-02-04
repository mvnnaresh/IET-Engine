#version 330
 

vec3 calculate_light_direction(vec3 vertex_world_space);
vec3 calculate_diffuse_component_material(vec3 normal, vec3 light_direction);
vec3 calculate_specular_component_material(vec3 normalized_normal, vec3 eye_direction, vec3 reflection_direction);
vec3 get_light_ambient_material();


in  vec3 N; 
in  vec3 vertex_world_space;

uniform vec3 eye_position;
 

out vec4 color;

void main()
{
	// set important material values
    float roughnessValue		= 0.3; // 0 : smooth, 1: rough
    float F0 					= 0.8; // fresnel reflectance at normal incidence
    float k 					= 0.2; // fraction of diffuse reflection (specular reflection = 1 - k)
	
	//Properties
	vec3 light_direction 		= 	calculate_light_direction(vertex_world_space);
	vec3 eye_direction 			=   normalize(eye_position - vertex_world_space);
	vec3 norm 					= 	normalize(N); 
 	vec3 H						= 	normalize(eye_direction + light_direction);

	float NdotL 				= 	max(dot(norm, light_direction), 0.0);
	float NdotH 				= 	max(dot(norm, H), 0.0); 
    float NdotV 				= 	max(dot(norm, eye_direction), 0.0); // note: this could also be NdotL, which is the same value
    float VdotH 				= 	max(dot(eye_direction, H), 0.0);
 	float mSquared 				= 	roughnessValue * roughnessValue;
	 

 	float Gc 					=	2 * ( NdotH * NdotV ) / VdotH;
	float Gb 					=	2 * ( NdotH * NdotL ) / VdotH;
	float geo_attenuation 		= 	min(1.0,min(Gc,Gb));

	// roughness (or: microfacet distribution function)
    // beckmann distribution function
    float r1 					= 	1.0 / ( 4.0 * mSquared * pow(NdotH, 4.0));
    float r2 					= 	(NdotH * NdotH - 1.0) / (mSquared * NdotH * NdotH);
    float roughness 			= 	r1 * exp(r2);

	// fresnel
    // Schlick approximation
    float fresnel 				= 	F0 + (1.0 - F0) * pow(1.0 - VdotH, 5.0);
  
    float specular 				=   (fresnel * geo_attenuation * roughness) / (NdotV * NdotL * 3.14);

    vec3 finalValue 			= 	get_light_ambient_material() * NdotL * (k + specular * (1.0 - k));

	color 						=  	vec4(finalValue, 1.0);
}