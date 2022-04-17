#ifdef GL_ES
precision mediump float;
#endif

varying vec3 color;

const float scale = 1.0;
void main(){
    
    gl_FragColor = vec4(color, 1.0) * scale;
}