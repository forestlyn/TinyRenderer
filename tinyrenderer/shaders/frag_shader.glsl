#version 120

uniform vec4 viewport;
varying float radius;
varying vec2 center;

void main(void) {
    // 获得当前待处理像素在 NDC 坐标系中的位置，方便后续计算
    // 由于 gl_FragCoord 是在窗口坐标系中的，所以需要转换
    // 使用二维的NCD坐标，因为球在无论哪个角度投影都是圆
    vec2 ndc_current_pixel = ((2.0 * gl_FragCoord.xy) - (2.0 * viewport.xy)) / (viewport.zw) - 1;

    // 计算当前像素与圆心的距离 也是在二维NDC坐标系中
    vec2 diff = ndc_current_pixel - center;
    float d2 = dot(diff, diff);
    float r2 = radius * radius;

    // 大于丢弃，小于则计算光照强度
    if(d2 > r2) {
        discard;
    } else {
        // 计算光照强度
        // 获得光线方向
        vec3 l = normalize(gl_LightSource[0].position.xyz);
        // 直角三角形计算出z方向的长度
        float dr = sqrt(r2 - d2);
        // 法向量
        vec3 n = vec3(ndc_current_pixel - center, dr);
        // 计算光照强度
        float intensity = .2 + max(dot(l, normalize(n)), 0.0);
        gl_FragColor = gl_Color * intensity;
        // 计算当前像素实际深度
        // gl_FragCoord.z是待处理像素的深度，范围[0,1]，由于渲染的是点，深度就是点的深度
        // dr是ndc坐标中的z方向长度，后半段计算像素相对点的深度，由于ndc范围【-1，1】
        // 所以需要乘以gl_DepthRange.diff / 2.0 得到对应【0，1】深度范围的值
        // 再* gl_ProjectionMatrix[2].z获得窗口范围的值
        gl_FragDepth = gl_FragCoord.z + dr * gl_DepthRange.diff / 2.0 * gl_ProjectionMatrix[2].z;
    }
}
