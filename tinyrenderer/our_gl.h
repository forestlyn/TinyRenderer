#include "tgaimage.h"
#include "geometry.h"

extern Matrix ModelView;
extern Matrix Viewport;
extern Matrix Projection;

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor green = TGAColor(0, 255, 0, 255);
const TGAColor blue = TGAColor(0, 0, 255, 255);

void viewport(int x, int y, int w, int h);
void projection(float coeff = 0.f); // coeff = -1/c
void lookat(Vec3f eye, Vec3f center, Vec3f up);

struct IShader
{
    virtual ~IShader();
    virtual Vec4f vertex(int iface, int nthvert) = 0;
    virtual bool fragment(Vec3f bar, TGAColor &color) = 0;
};

void triangle(Vec4f *pts, IShader &shader, TGAImage &image, TGAImage &zbuffer);