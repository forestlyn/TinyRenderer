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
	Vec3f u = cross(Vec3f(ab.x, ac.x, pa.x), Vec3f(ab.y, ac.y, pa.y));
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

void triangle(Vec3f t0, Vec3f t1, Vec3f t2, float *zbuffer, TGAImage &image, TGAColor color)
{
	float minx = std::min(t0.x, t1.x);
	minx = std::min(minx, t2.x);
	minx = std::max(minx, .0f);
	float maxx = std::max(t0.x, t1.x);
	maxx = std::max(maxx, t2.x);
	maxx = std::min(float(width - 1), maxx);
	float miny = std::min(t0.y, t1.y);
	miny = std::min(miny, t2.y);
	miny = std::max(miny, .0f);
	float maxy = std::max(t0.y, t1.y);
	maxy = std::max(maxy, t2.y);
	maxy = std::min(float(height - 1), maxy);
	// printf("%f %f %f %f \n", minx, maxx, miny, maxy);
	for (float x = minx; x <= maxx; x += 1)
	{
		for (float y = miny; y <= maxy; y += 1)
		{
			Vec3f res = barycentric(t0, t1, t2, Vec3f(x, y, 0));
			if (res.x < 0 || res.y < 0 || res.z < 0)
				continue;
			float z = 0;
			z = res.x * t0.z + res.y * t1.z + res.z * t2.z;
			// printf("%f %f\n", zbuffer[int(x + y * width)], z);
			if (zbuffer[int(x + y * width)] < z)
			{
				zbuffer[int(x + y * width)] = z;
				image.set(int(x), int(y), color);
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
	Vec3f light_dir(0, 0, -1); // define light_dir

	for (int i = 0; i < model->nfaces(); i++)
	{
		std::vector<int> face = model->face(i);
		Vec3f pts[3];
		for (int i = 0; i < 3; i++)
			pts[i] = world2screen(model->vert(face[i]));
		Vec3f n = cross(model->vert(face[2]) - model->vert(face[0]), model->vert(face[1]) - model->vert(face[0]));
		n.normalize();
		float intensity = n * light_dir;
		triangle(pts[0], pts[1], pts[2], zbuffer, image, TGAColor(intensity * 255, intensity * 255, intensity * 255, 255));
	}

	image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
	image.write_tga_file("output.tga");
	delete model;
}
