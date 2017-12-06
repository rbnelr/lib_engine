
vec3 normal_mapping (vec3 pos_cam, vec3 geom_norm_cam, vec4 tang_cam, vec2 uv, sampler2D normal_tex) {
	
	geom_norm_cam = normalize(geom_norm_cam);
	
	mat3 TBN_tang_to_cam;
	{
		vec3	t =			tang_cam.xyz;
		float	b_sign =	tang_cam.w;
		vec3	b;
		vec3	n =			geom_norm_cam;
		
		t = normalize( t -(dot(t, n) * n) );
		b = cross(n, t);
		b *= b_sign;
		
		TBN_tang_to_cam = mat3(t, b, n);
	}
	
	vec3 normal_tang_sample = texture(normal_tex, uv).rgb; // normal in tangent space
	vec3 norm_tang = normalize(normal_tang_sample * 2.0 -1.0);
	vec3 norm_cam = TBN_tang_to_cam * norm_tang;
	
	vec3 view_cam = normalize(-pos_cam);
	
	//const float ARTIFACT_FIX_THRES = 0.128;
	//if (dot(view_cam, norm_cam) < ARTIFACT_FIX_THRES) {
	//	norm_cam = mix(norm_cam, geom_norm_cam, map(dot(view_cam, norm_cam), ARTIFACT_FIX_THRES, 0));
	//	//DBG_COL(map(dot(view_cam, norm_cam), ARTIFACT_FIX_THRES, 0));
	//}
	
	//DBG_COL(norm_cam * v3(-1,-1,1));
	//DBG_COL(pow(normal_tang_sample, v3(2.2)));
	
	return norm_cam;
}
