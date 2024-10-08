#include <vector>
#include <cmath>
#include <cstdlib>
#include <limits>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor green = TGAColor(0, 255, 0, 255);
const TGAColor blue = TGAColor(0, 0, 255, 255);

Model *model = NULL;

const int width = 800;
const int height = 800;
const int depth = 255;

Vec3f camera(0, 0, 3);
Vec3f lightdir(0, 0, -1);

Vec3f m2v(Matrix m)
{
	return Vec3f(m[0][0] / m[3][0], m[1][0] / m[3][0], m[2][0] / m[3][0]);
}

Matrix v2m(Vec3f v)
{
	Matrix m(4, 1);
	m[0][0] = v.x;
	m[1][0] = v.y;
	m[2][0] = v.z;
	m[3][0] = 1.f;
	return m;
}

Matrix viewport(int x, int y, int w, int h)
{
	Matrix m = Matrix::identity(4);
	m[0][3] = x + w / 2.f;
	m[1][3] = y + h / 2.f;
	m[2][3] = depth / 2.f;

	m[0][0] = w / 2.f;
	m[1][1] = h / 2.f;
	m[2][2] = depth / 2.f;
	return m;
}
void line(int x0, int y0, int x1, int y1, TGAImage &image, TGAColor color)
{
	int dx = abs(x0 - x1);
	int dy = abs(y0 - y1);
	int maxDelta = dx > dy ? dx : dy;
	int x_delta = x0 < x1 ? 1 : -1;
	int y_delta = y0 < y1 ? 1 : -1;
	int x = x0;
	int y = y0;
	int x_error = 0;
	int y_error = 0;
	for (int t = 0; t <= maxDelta; t++)
	{
		image.set(x, y, color);
		x_error += dx * 2;
		y_error += dy * 2;
		if (x_error > maxDelta)
		{
			x += x_delta;
			x_error -= maxDelta * 2;
		}
		if (y_error > maxDelta)
		{
			y += y_delta;
			y_error -= maxDelta * 2;
		}
	}
}

void line(Vec2i t0, Vec2i t1, TGAImage &image, TGAColor color)
{
	line(t0.x, t0.y, t1.x, t1.y, image, color);
}

Vec3f barycentric(Vec3f ab, Vec3f ac, Vec3f pa)
{
	Vec3f u = (Vec3f(ab.x, ac.x, pa.x) ^ Vec3f(ab.y, ac.y, pa.y));
	if (abs(u.z) < 1e-2)
	{
		return Vec3f(1, 1, -1);
	}
	// printf("%f %f %f\n", 1 - (u.x + u.y) / u.z, u.x / u.z, u.y / u.z);
	return Vec3f(1 - (u.x + u.y) / u.z, u.x / u.z, u.y / u.z);
}

Vec3f barycentric(Vec3f a, Vec3f b, Vec3f c, Vec3f p)
{
	return barycentric(b - a, c - a, a - p);
}

void triangle(Vec3f *pts, Vec3f *uvs, float *zbuffer, TGAImage &image, TGAImage &uvImage, float intensity)
{
	float minx = std::min(pts[0].x, pts[1].x);
	minx = std::min(minx, pts[2].x);
	minx = std::max(minx, .0f);
	float maxx = std::max(pts[0].x, pts[1].x);
	maxx = std::max(maxx, pts[2].x);
	maxx = std::min(float(width - 1), maxx);
	float miny = std::min(pts[0].y, pts[1].y);
	miny = std::min(miny, pts[2].y);
	miny = std::max(miny, .0f);
	float maxy = std::max(pts[0].y, pts[1].y);
	maxy = std::max(maxy, pts[2].y);
	maxy = std::min(float(height - 1), maxy);
	// printf("%f %f %f %f \n", minx, maxx, miny, maxy);
	for (float x = minx; x <= maxx; x += 1)
	{
		for (float y = miny; y <= maxy; y += 1)
		{
			Vec3f res = barycentric(pts[0], pts[1], pts[2], Vec3f(x, y, 0));
			if (res.x < 0 || res.y < 0 || res.z < 0)
				continue;
			float z = 0;
			z = res.x * pts[0].z + res.y * pts[1].z + res.z * pts[2].z;
			// printf("%f %f\n", zbuffer[int(x + y * width)], z);
			if (zbuffer[int(x + y * width)] < z)
			{
				// printf("%f %f\n", zbuffer[int(x + y * width)], z);
				zbuffer[int(x + y * width)] = z;
				TGAColor color;
				Vec3f uv = uvs[0] * res.x + uvs[1] * res.y + uvs[2] * res.z;
				color = uvImage.get(uv[0] * uvImage.get_width(), uv[1] * uvImage.get_height());
				// printf("%f %f %f\n", uv.x, uv.y, uv.z);
				// printf("%f %f %f\n", uvs[0].x, uvs[0].y, uvs[0].z);
				image.set(int(x), int(y), color * intensity);
			}
		}
	}
}
Vec3f world2screen(Vec3f v)
{
	return Vec3f(int((v.x + 1.) * width / 2. + .5), int((v.y + 1.) * height / 2. + .5), v.z);
}
int main(int argc, char **argv)
{
	if (2 == argc)
	{
		model = new Model(argv[1]);
	}
	else
	{
		model = new Model("obj/african_head.obj");
	}

	float *zbuffer = new float[width * height];
	for (int i = width * height; i--; zbuffer[i] = -std::numeric_limits<float>::max())
		;

	TGAImage image(width, height, TGAImage::RGB);
	TGAImage uvimage = TGAImage();
	bool readSuccess = uvimage.read_tga_file("obj/african_head_diffuse.tga");
	// have the origin at the left top corner of the image
	uvimage.flip_vertically();
	Matrix Perspective = Matrix::identity(4);
	Perspective[3][2] = -1. / camera.z;
	Matrix view = viewport(0, 0, width, height);
	if (readSuccess)
	{
		for (int i = 0; i < model->nfaces(); i++)
		{
			std::vector<int> face = model->face(i);
			Vec3f pts[3];
			Vec3f uvs[3];
			Vec3f world_verts[3];
			for (int i = 0; i < 3; i++)
			{
				Vec3f v = model->vert(face[i * 2]);
				world_verts[i] = v;
				Vec3f temp = m2v(view * (Perspective * v2m(v)));
				pts[i] = Vec3f(int(temp.x), int(temp.y), temp.z);
				// pts[i] = temp;
				uvs[i] = model->texture(face[i * 2 + 1]);
			}
			Vec3f n = (world_verts[2] - world_verts[0]) ^ (world_verts[1] - world_verts[0]);
			n.normalize();
			float intensity = n * lightdir;
			if (intensity > 0)
				triangle(pts, uvs, zbuffer, image, uvimage, intensity);
		}
	}

	image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
	image.write_tga_file("output.tga");
	delete model;
}
