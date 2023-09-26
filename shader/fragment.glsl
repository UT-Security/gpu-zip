#version 330 core
out vec4 FragColor;
in vec3 ourColor;
in vec3 position;

uniform int SCR_WIDTH;

// Inspired by: https://github.com/jagracar/webgl-shader-examples/blob/master/shaders/requires/random2d.glsl
float random(vec3 co) {
    float a = 12.9898;
	float b = 78.233;
    float c = 43758.5453;
    float dt = dot(co, vec3(a, b, c));
    float sn = mod(dt, 3.14);
    return fract(sn*1234567.7654321);
}

// A constant time implementation of pattern creation
// Output: Black, Random, Gradient, Skew depending on ourColor.x
void main()
{
    int mod_distance = int(ourColor.x);

    int is_zero = int(ourColor.x == 0.0);
    mod_distance = mod_distance + is_zero;

    int position_int = int( (position.x + 1.0) * SCR_WIDTH/2 + abs(position.y-1.0)*SCR_WIDTH*SCR_WIDTH/2 );
    position_int = position_int*(1-is_zero);

	// Give a random color to each pos
    vec3 rand_color = vec3(random(position * ourColor), random(1.234 * position * position * ourColor), random(9.876 * position * position * position * ourColor));

    float myColor = float(position_int%mod_distance)/float(256.0);
    vec3 final_color = vec3(myColor,myColor,myColor);

    int random = int(ourColor.x == 1.0);
    vec3 color = rand_color*random + final_color*(1-random);

    FragColor = vec4(color, 1.0);

}
