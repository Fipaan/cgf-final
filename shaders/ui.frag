#version 330 core

out vec4 FragColor;

uniform vec4 uRect;
uniform vec4 uColor;
uniform vec4 uBorder;
uniform float uRadius;
uniform float uBorderWidth;
uniform int uStyle;
uniform float uProgress;
uniform float uTime;

const float PI = 3.14159265359;

float segment(vec2 p, vec2 a, vec2 b) {
    vec2 edge = b - a;
    return length(p - a - edge * clamp(dot(p - a, edge) / dot(edge, edge), 0.0, 1.0));
}

float stroke(float distance, float width, float aa) {
    return 1.0 - smoothstep(width, width + aa, abs(distance));
}

void main() {
    vec2 uv = (gl_FragCoord.xy - uRect.xy) / uRect.zw;

    if (uStyle == 1) {
        vec2 p = (uv - 0.5) * vec2(uRect.z / uRect.w, 1.0);
        float green = exp(-3.8 * dot(p - vec2(0.38, 0.15), p - vec2(0.38, 0.15)));
        float amber = exp(-8.0 * dot(p - vec2(-0.6, -0.35), p - vec2(-0.6, -0.35)));
        vec3 color = vec3(0.018, 0.038, 0.040) + green * vec3(0.020, 0.047, 0.039) + amber * vec3(0.045, 0.027, 0.010);
        float grain = fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453);
        color += (grain - 0.5) * 0.012;
        vec2 grid = abs(fract(gl_FragCoord.xy / 64.0) - 0.5);
        color += (1.0 - smoothstep(0.005, 0.019, min(grid.x, grid.y))) * 0.004;
        color *= 1.0 - 0.23 * smoothstep(0.3, 1.1, length(p));
        FragColor = vec4(color, 1.0);
        return;
    }

    if (uStyle == 2) {
        vec2 p = (uv - 0.5) * 2.0;
        p.x *= uRect.z / uRect.w;
        float r = length(p);
        float a = atan(p.y, p.x);
        float aa = 3.0 / uRect.w;
        float rings = stroke(r - 0.80, 0.002, aa) * 0.65 + stroke(r - 0.73, 0.001, aa) * 0.35;
        float ticks = pow(max(cos(a * 48.0), 0.0), 28.0);
        rings += ticks * (smoothstep(0.745, 0.755, r) - smoothstep(0.780, 0.79, r)) * 0.75;
        float angle = uTime * 0.035;
        mat2 rotation = mat2(cos(angle), -sin(angle), sin(angle), cos(angle));
        vec2 q = rotation * p;
        vec2 t0 = vec2(0.0, 0.61), t1 = vec2(-0.528, -0.305), t2 = vec2(0.528, -0.305);
        float triangle = min(segment(q, t0, t1), min(segment(q, t1, t2), segment(q, t2, t0)));
        rings += stroke(triangle, 0.002, aa) * 0.70;
        rings += stroke(r - 0.33, 0.002, aa) * 0.60;
        float nodes = 0.0;
        for (int i = 0; i < 6; ++i) {
            float orbit = float(i) * PI / 3.0 - angle * 0.7;
            float d = length(p - vec2(cos(orbit), sin(orbit)) * 0.80);
            nodes += stroke(d - 0.022, 0.003, aa) + exp(-d * d * 1100.0) * 0.32;
        }
        float halo = exp(-r * r * 6.0) * 0.11;
        float pulse = 0.88 + 0.12 * sin(uTime * 0.8);
        float alpha = clamp((rings + nodes) * 0.76 + halo, 0.0, 1.0) * uColor.a;
        FragColor = vec4(uColor.rgb * pulse, alpha);
        return;
    }

    float radius = min(uRadius, min(uRect.z, uRect.w) * 0.5);
    vec2 q = abs(gl_FragCoord.xy - uRect.xy - uRect.zw * 0.5) - uRect.zw * 0.5 + radius;
    float distance = length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - radius;
    float aa = max(fwidth(distance), 0.65);
    float mask = 1.0 - smoothstep(-aa, aa, distance);
    float edge = uBorderWidth > 0.0 ? smoothstep(-uBorderWidth - aa, -uBorderWidth + aa, distance) : 0.0;
    vec4 base = uColor;
    if (uStyle == 3) {
        float fill = (1.0 - smoothstep(uProgress - 0.12, uProgress + 0.05, uv.x)) * uProgress;
        base.rgb += vec3(0.055, 0.095, 0.065) * fill;
        base.rgb += vec3(0.025, 0.022, 0.015) * uv.y;
    }
    vec4 color = mix(base, uBorder, edge);
    FragColor = vec4(color.rgb, color.a * mask);
}
