#version 120

uniform vec4 viewport;
attribute float radius_attr;

varying float radius;
varying vec2 center;

void main() {
    // 获得当前像素在 NDC 坐标系中的位置，方便后续计算
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
    gl_FrontColor = gl_Color;
    gl_PointSize = radius_attr * min(viewport.z, viewport.w);

    center = gl_Position.xy;
    radius = radius_attr;
}