#version 330 core

in vec2 TexCoords;
out vec4 color;

uniform sampler2D current;
uniform sampler2D prev;
uniform sampler2D bloom;

uniform vec3 screen;
uniform float alpha;
uniform float[9] kernel;
uniform int crt;

vec2 px_step = vec2(1.0 / screen.x, 1.0 / screen.y);

uniform float edge_threshold;
uniform float edge_threshold_min;

//const float edge_threshold = 1.0 / 8.0;
//const float edge_threshold_min = 1.0 / 24.0;
const float subpix_trim = 1.0 / 4.0;
const float subpix_trim_scale = 1.0 / (1.0 - subpix_trim);
const float subpix_trim_cap = 3.0 / 4.0;

const uint search_steps = 16u;

vec4 Blur(vec2 tc, vec4 c)
{
	vec2 offsets[9] = vec2[](
		vec2(-px_step.x, px_step.y),  // top-left
		vec2(0.0f,    px_step.y),  // top-center
		vec2(px_step.x,  px_step.y),  // top-right
		vec2(-px_step.x, 0.0f),    // center-left
		vec2(0.0f,    0.0f),    // center-center
		vec2(px_step.x,  0.0f),    // center-right
		vec2(-px_step.x, -px_step.y), // bottom-left
		vec2(0.0f,    -px_step.y), // bottom-center
		vec2(px_step.x,  -px_step.y)  // bottom-right
	);

	vec3 sampleTex[9];
	for(int i = 0; i < 9; i++)
	{
		sampleTex[i] = vec3(texture(prev, tc + offsets[i]));
	}
	vec3 col = vec3(0.0);
	for(int i = 0; i < 9; i++)
		col += sampleTex[i] * kernel[i];

	return vec4(col, 1.0) * alpha + c;
}

vec4 None()
{
	return vec4(texture(current, TexCoords).rgb, 1.0);
}

float Luminance(vec3 clr)
{
	vec3 lumi = vec3(0.2126, 0.7152, 0.0722);
	return dot(clr, lumi);
}

vec3 Lerp(vec3 a, vec3 b, float t)
{
	return a * vec3(t) + (b - vec3(t) * b);
}

vec4 FXAA(vec2 tc)
{
	vec3 n = texture(current, tc + vec2(0, -px_step.y)).rgb;
	vec3 s = texture(current, tc + vec2(0, px_step.y)).rgb;
	vec3 e = texture(current, tc + vec2(px_step.x, 0)).rgb;
	vec3 w = texture(current, tc + vec2(-px_step.x, 0)).rgb;
	vec3 o = texture(current, tc).rgb;

	float ln = Luminance(n);
	float ls = Luminance(s);
	float le = Luminance(e);
	float lw = Luminance(w);
	float lo = Luminance(o);

	float lmax = max(lo, max(max(ln, lw), max(ls, le)));
	float lmin = min(lo, min(min(ln, lw), min(ls, le)));
	float range = lmax - lmin;

	if (range < max(edge_threshold_min, lmax * edge_threshold))
		return vec4(o, 1.0);

	float ll = (ln + ls + le + lw) * 0.25;
	float rl = abs(ll - lo);
	float blend = max(0.0, (rl / range) - subpix_trim) * subpix_trim_scale;
	blend = min(subpix_trim_cap, blend);

	//return vec4(1.0, blend / subpix_trim_cap, 0.0, 1.0);

	vec3 nw = texture(current, tc + vec2(-px_step.x, -px_step.y)).rgb;
	vec3 ne = texture(current, tc + vec2(px_step.x, -px_step.y)).rgb;
	vec3 sw = texture(current, tc + vec2(-px_step.x, px_step.y)).rgb;
	vec3 se = texture(current, tc + vec2(px_step.x, px_step.y)).rgb;

	float lnw = Luminance(nw);
	float lne = Luminance(ne);
	float lsw = Luminance(sw);
	float lse = Luminance(se);

	vec3 all = nw + n + ne + w + o + e + sw + s + se;
	all *= (1.0 / 9.0);
	
	float v_edge =
		abs((0.25 * lnw) + (-0.5 * ln) + (0.25 * lne)) +
		abs((0.50 * lw) + (-1.0 * lo) + (0.50 * le)) +
		abs((0.25 * lsw) + (-0.5 * ls) + (0.25 * lse));

	float h_edge =
		abs((0.25 * lnw) + (-0.5 * lw) + (0.25 * lsw)) +
		abs((0.50 * ln) + (-1.0 * lo) + (0.50 * ls)) +
		abs((0.25 * lne) + (-0.5 * le) + (0.25 * lse));

	bool is_horz = (h_edge >= v_edge);

	/*
	if (is_horz)
		return vec4(1.0, 1.0, 0.0, 1.0);
	else
		return vec4(0.0, 0.0, 1.0, 1.0);
	/**/

	float step = is_horz ? -px_step.y : -px_step.x;
	if (!is_horz)
	{
		ln = lw;
		ls = le;
	}

	float n_grad = abs(ln - lo);
	float s_grad = abs(ls - lo);
	ln = (ln + lo) * 0.5;
	ls = (ls + lo) * 0.5;

	bool pair_n = (n_grad >= s_grad);

	/*
	if (pair_n)
		return vec4(0.0, 0.0, 1.0, 1.0);
	else
		return vec4(0.0, 1.0, 0.0, 1.0);
	/**/

	if (!pair_n)
	{
		ln = ls;
		n_grad = s_grad;
		step = -step;
	}

	vec2 n_pos = tc + vec2(is_horz ? 0.0 : step * 0.5, is_horz ? step * 0.5 : 0.0);
	vec2 p_pos = n_pos;
	vec2 np_off = is_horz ? vec2(px_step.x, 0.0) : vec2(0.0, px_step.y);

	float n_end = ln;
	float p_end = ln;

	bool n_done = false;
	bool p_done = false;

	n_pos += np_off * vec2(-1.0, -1.0);
	p_pos += np_off * vec2(1.0, 1.0);

	for (uint i = 0u; i < search_steps; i++)
	{
		if (!n_done)
			n_end = Luminance(texture(current, n_pos).rgb);
		if (!p_done)
			p_end = Luminance(texture(current, p_pos).rgb);

		n_done = n_done || (abs(n_end - ln) >= n_grad);
		p_done = p_done || (abs(p_end - ln) >= n_grad);
		if (n_done && p_done)
			break;
		if (!n_done)
			n_pos -= np_off;
		if (!p_done)
			p_pos += np_off;
	}

	float n_dst = is_horz ? tc.x - n_pos.x : tc.y - n_pos.y;
	float p_dst = is_horz ? p_pos.x - tc.x : p_pos.y - tc.y;
	bool is_dir_neg = (n_dst < p_dst);

	/*
	if (is_dir_neg)
		return vec4(1.0, 0.0, 0.0, 1.0);
	else
		return vec4(0.0, 0.0, 1.0, 1.0);
	/**/

	if (((lo - ln) < 0.0) == ((n_end - ln) < 0.0))
		step = 0.0;

	float span = (p_dst + n_dst);
	n_dst = is_dir_neg ? n_dst : p_dst;
	float sub_px_off = (0.5 + (n_dst * (-1.0 / span))) * step;

	/*
	float ox = is_horz ? 0.0 : sub_px_off * 2.0 / px_step.x;
	float oy = is_horz ? sub_px_off * 2.0 / px_step.y : 0.0;
	if (ox < 0.0)
		return vec4(Lerp(vec3(lo), vec3(1.0, 0.0, 0.0), -ox), 1.0);
	if (ox > 0.0)
		return vec4(Lerp(vec3(lo), vec3(0.0, 0.0, 1.0), ox), 1.0);
	if (oy < 0.0)
		return vec4(Lerp(vec3(lo), vec3(1.0, 1.0, 0.0), -oy), 1.0);
	if (oy > 0.0)
		return vec4(Lerp(vec3(lo), vec3(0.0, 1.0, 1.0), oy), 1.0);
	/**/

	vec3 f = texture(current, tc + (is_horz ? vec2(0.0, sub_px_off) : vec2(sub_px_off, 0.0))).rgb;
	return vec4(Lerp(all, f, blend), 1.0);
}

void main()
{
	//color = vec4(texture(current, TexCoords).rgb + texture(bloom, TexCoords).rgb + texture(prev, TexCoords).rgb * alpha, 0);
	//color = texture(current, TexCoords);
	//color = texture(bloom, TexCoords);
	
	//if (mod(floor(TexCoords.y * screen.y), 2) == 0)
	//	discard;

	//if (mod(floor(TexCoords.y * screen.y), 4) == 1)
	vec4 a = texture(current, TexCoords);
	vec4 b = texture(bloom, TexCoords);
	vec4 c = texture(prev, TexCoords);

	int x = int(floor(TexCoords.x * screen.x));
	int y = int(floor(TexCoords.y * screen.y));

	if (crt == 1)
	{
		// RGB
		// BRG
		vec2 p = vec2(1 / screen.x, 0);
		if (y % 2 == 0)
		{
			vec2 s = vec2(x - mod(x, 3), y) / screen.xy;
			vec3 o = vec3(0, 0, 0);
			for (int n = 0; n < 3; ++n)
			{
				o += max(texture(current, s + p * n).rgb, texture(bloom, s + p * n).rgb);
			}
			o /= 3;

			if (x % 3 == 0)
				color = vec4(o.r, 0, 0, 1);
			else if (x % 3 == 1)
				color = vec4(0, o.g, 0, 1);
			else if (x % 3 == 2)
				color = vec4(0, 0, o.b, 1);
		}
		else
		{
			vec2 s = vec2(x - mod(x - 1, 3), y) / screen.xy;
			vec3 o = vec3(0, 0, 0);
			for (int n = 0; n < 3; ++n)
			{
				o += max(texture(current, s + p * n).rgb, texture(bloom, s + p * n).rgb);
			}
			o /= 3;
			if (x % 3 == 0)
				color = vec4(0, 0, o.b, 1);
			else if (x % 3 == 1)
				color = vec4(o.r, 0, 0, 1);
			else if (x % 3 == 2)
				color = vec4(0, o.g, 0, 1);
		}
	}
	else if (crt == 2)
	{
		// RGB
		// RGB
		// RGB
		// ___
		if (y % 4 == 0)
			color = vec4(0, 0, 0, 1);
		else if (x % 3 == 0)
			color = vec4(max(a.r, b.r), 0, 0, 1);
		else if (x % 3 == 1)
			color = vec4(0, max(a.g, b.g), 0, 1);
		else if (x % 3 == 2)
			color = vec4(0, 0, max(a.b, b.b), 1);
	}
	else if (crt == 3)
	{
		// RGB_RGB_
		// RGB_____
		// RGB_RGB_
		// ____RGB_
		vec2 px = vec2(1 / screen.x, 0);
		vec2 py = vec2(0, 1 / screen.y);
		int k = int(mod(int(x / 4), 2)) * 2;
		vec2 s = vec2(x - mod(x, 4) + 1, y - mod(y - k, 4) + 1 - k) / screen.xy;
		vec3 o = vec3(0, 0, 0);
		for (int n = 0; n < 16; ++n)
		{
			vec2 ss = s + px * mod(n, 4) + py * (n / 4);
			o += max(texture(current, ss).rgb, texture(bloom, ss).rgb);
		}
		o /= 16;

		int clr = int(sign(mod(y + k, 4))) * int(mod(x, 4));
		if (clr == 0)
			color = vec4(0, 0, 0, 1);
		else if (clr == 1)
			color = vec4(o.r, 0, 0, 1);
		else if (clr == 2)
			color = vec4(0, o.g, 0, 1);
		else if (clr == 3)
			color = vec4(0, 0, o.b, 1);
	}
	else if (crt == 4)
	{
		// R_G_B_
		// _B_R_G
		int i = int(mod(x + mod(y, 2) * 3, 6));
		int clr = int(mod(i, 2)) * int(ceil(i / 2.0));
		if (clr == 0)
			color = vec4(0, 0, 0, 1);
		else if (clr == 1)
			color = vec4(max(a.r, b.r), 0, 0, 1);
		else if (clr == 2)
			color = vec4(0, max(a.g, b.g), 0, 1);
		else if (clr == 3)
			color = vec4(0, 0, max(a.b, b.b), 1);
	}
	else if (crt == 5)
	{
		// RGB
		// GBR
		// BRG
		int clr = int(mod(x - mod(y, 3), 3)) + 1;
		if (clr == 0)
			color = vec4(0, 0, 0, 1);
		else if (clr == 1)
			color = vec4(max(a.r, b.r), 0, 0, 1);
		else if (clr == 2)
			color = vec4(0, max(a.g, b.g), 0, 1);
		else if (clr == 3)
			color = vec4(0, 0, max(a.b, b.b), 1);
	}
	else if (crt == 6)
	{
		// RGB_
		// RGB_
		// RGB_
		if (x % 4 == 0)
			color = vec4(max(a.r, b.r), 0, 0, 1);
		else if (x % 4 == 1)
			color = vec4(0, max(a.g, b.g), 0, 1);
		else if (x % 4 == 2)
			color = vec4(0, 0, max(a.b, b.b), 1);
		else if (x % 4 == 3)
			color = vec4(0, 0, 0, 1);
	}
	else
	{
		color = vec4(max(max(a.rgb, b.rgb), c.rgb * alpha), 1);
	}

	//color = texture(prev, TexCoords);
	//color = vec4(Luminance(texture(current, TexCoords).rgb), Luminance(texture(prev, TexCoords).rgb), 0.0, 1.0);
	//vec4 p = texture(prev, TexCoords);
	//color = vec4((c.r + c.g + c.b) / 3.0, (p.r + p.g + p.b) / 3.0, 0.0, 1.0);

	//vec4 clr = texture(current, TexCoords) + texture(prev, TexCoords) * 0.95;
	//color = vec4(clr.rgb, 1.0);

	//color = Blur(TexCoords, FXAA(TexCoords));
	//color = Blur(TexCoords, texture(current, TexCoords));	
}
