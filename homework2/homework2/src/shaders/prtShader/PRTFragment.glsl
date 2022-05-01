#ifdef GL_ES
precision mediump float;
#endif

varying vec3 color;

const float scale = 1.0;
float gamma = 2.2;
void main(){
    vec3 res = pow(color, vec3(gamma));
    gl_FragColor = vec4(color, 1.0) * scale;
}