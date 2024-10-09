#include "our_gl.h"
#include <cmath>
#include <vector>
#include <cstdlib>
#include <limits>
Matrix ModelView;
Matrix Viewport;
Matrix Projection;
IShader::~IShader() {}

void lookat(Vec3f eye, Vec3f center, Vec3f up)
{
    Vec3f z = (eye - center).normalize();
    Vec3f x = cross(up, z).normalize();
    Vec3f y = cross(z, x).normalize();
    ModelView = Matrix::identity();
    for (int i = 0; i < 3; i++)
    {
        ModelView[0][i] = x[i];
        ModelView[1][i] = y[i];
        ModelView[2][i] = z[i];
        ModelView[i][3] = -center[i];
    }
    // ModelView = view * subO;
}

void viewport(int x, int y, int w, int h)
{
    Viewport = Matrix::identity();
    Viewport[0][3] = x + w / 2.f;
    Viewport[1][3] = y + h / 2.f;
    Viewport[2][3] = 255.f / 2.f;

    Viewport[0][0] = w / 2.f;
    Viewport[1][1] = h / 2.f;
    Viewport[2][2] = 255.f / 2.f;
}

void projection(float coeff)
{
    Projection = Matrix::identity();
    Projection[3][2] = coeff;
}

Vec3f Vec4To3(Vec4f v)
{
    return Vec3f(v[0] / v[3], v[1] / v[3], v[2] / v[3]);
}
Vec3f barycentric(Vec3f ab, Vec3f ac, Vec3f pa)
{
    Vec3f u = cross(Vec3f(ab.x, ac.x, pa.x), Vec3f(ab.y, ac.y, pa.y));
    if (abs(u.z) < 1e-2)
    {
        return Vec3f(1, 1, -1);
    }
    // printf("%f %f %f\n", 1 - (u.x + u.y) / u.z, u.x / u.z, u.y / u.z);
    return Vec3f(1 - (u.x + u.y) / u.z, u.x / u.z, u.y / u.z);
}

Vec3f barycentric(Vec4f a, Vec4f b, Vec4f c, Vec4f p)
{
    return barycentric(Vec4To3(b) - Vec4To3(a), Vec4To3(c) - Vec4To3(a), Vec4To3(a) - Vec4To3(p));
}
void triangle(Vec4f *pts, IShader &shader, TGAImage &image, TGAImage &zbuffer)
{
    float minx = std::min(pts[0][0] / pts[0][3], pts[1][0] / pts[1][3]);
    minx = std::min(minx, pts[2][0] / pts[2][3]);
    float maxx = std::max(pts[0][0] / pts[0][3], pts[1][0] / pts[1][3]);
    maxx = std::max(maxx, pts[2][0] / pts[2][3]);
    float miny = std::min(pts[0][1] / pts[0][3], pts[1][1] / pts[1][3]);
    miny = std::min(miny, pts[2][1] / pts[2][3]);
    float maxy = std::max(pts[0][1] / pts[0][3], pts[1][1] / pts[1][3]);
    maxy = std::max(maxy, pts[2][1] / pts[2][3]);
    // printf("%f %f %f %f \n", minx, maxx, miny, maxy);
    Vec4f p = Vec4f();
    TGAColor color;

    for (int x = minx; x <= maxx; x += 1)
    {
        for (int y = miny; y <= maxy; y += 1)
        {
            p[0] = x;
            p[1] = y;
            p[3] = 1;
            Vec3f res = barycentric(pts[0], pts[1], pts[2], p);
            // printf("%f %f %f\n", res.x, res.y, res.z);
            if (res.x < 0 || res.y < 0 || res.z < 0)
                continue;
            float z = 0, w = 0;
            z = res.x * pts[0][2] + res.y * pts[1][2] + res.z * pts[2][2];
            w = res.x * pts[0][3] + res.y * pts[1][3] + res.z * pts[2][3];
            int frag_depth = std::max(0, std::min(255, int(z / w + .5)));
            // printf("%f %f %d %d %f %f\n", p[0], p[1], zbuffer.get(p[0], p[1])[0], frag_depth, z, w);

            if (zbuffer.get(p[0], p[1])[0] < frag_depth)
            {
                if (!shader.fragment(res, color))
                {
                    zbuffer.set(p[0], p[1], TGAColor(frag_depth));
                    image.set(int(x), int(y), color);
                    // printf("%f %f %d\n", p[0], p[1], frag_depth);
                }
            }
        }
    }
}
