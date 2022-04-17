attribute vec3 aVertexPosition;
attribute vec3 aNormalPosition;
attribute mat3 aPrecomputeLT;

uniform mat4 uModelMatrix;
uniform mat4 uViewMatrix;
uniform mat4 uProjectionMatrix;

uniform vec3 uPrecomputeL0;
uniform vec3 uPrecomputeL1;
uniform vec3 uPrecomputeL2;
uniform vec3 uPrecomputeL3;
uniform vec3 uPrecomputeL4;
uniform vec3 uPrecomputeL5;
uniform vec3 uPrecomputeL6;
uniform vec3 uPrecomputeL7;
uniform vec3 uPrecomputeL8;
uniform float uLightness;

varying vec3 color;

void main(){
    color = vec3(0,0,0);
    color += uPrecomputeL0 * aPrecomputeLT[0][0];
    color += uPrecomputeL1 * aPrecomputeLT[0][1];
    color += uPrecomputeL2 * aPrecomputeLT[0][2];
    color += uPrecomputeL3 * aPrecomputeLT[1][0];
    color += uPrecomputeL4 * aPrecomputeLT[1][1];
    color += uPrecomputeL5 * aPrecomputeLT[1][2];
    color += uPrecomputeL6 * aPrecomputeLT[2][0];
    color += uPrecomputeL7 * aPrecomputeLT[2][1];
    color += uPrecomputeL8 * aPrecomputeLT[2][2];
    color *= uLightness;
    // precomputeLT = aPrecomputeLT;
    gl_Position = uProjectionMatrix * uViewMatrix * uModelMatrix * vec4(aVertexPosition, 1.0);
}