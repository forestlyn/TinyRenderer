#include <vector>
#include <cmath>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor green = TGAColor(0, 255, 0, 255);
const TGAColor blue = TGAColor(0, 0, 255, 255);

Model *model = NULL;
const int width = 300;
const int height = 300;

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

Vec3f barycentric(Vec2i ab, Vec2i ac, Vec2i pa)
{
	Vec3f u = Vec3f(ab.x, ac.x, pa.x) ^ Vec3f(ab.y, ac.y, pa.y);
	if (abs(u.z) < 1)
	{
		return Vec3f(1, 1, -1);
	}
	return Vec3f(1 - u.x / u.z - u.y / u.z, u.x / u.z, u.y / u.z);
}

Vec3f barycentric(Vec2i a, Vec2i b, Vec2i c, Vec2i p)
{
	return barycentric(Vec2i(b.x - a.x, b.y - a.y),
					   Vec2i(c.x - a.x, c.y - a.y),
					   Vec2i(a.x - p.x, a.y - p.y));
}

void triangle(Vec2i t0, Vec2i t1, Vec2i t2, TGAImage &image, TGAColor color)
{
	int minx = std::min(t0.x, t1.x);
	minx = std::min(minx, t2.x);
	minx = std::max(minx, 0);
	int maxx = std::max(t0.x, t1.x);
	maxx = std::max(maxx, t2.x);
	maxx = std::min(width - 1, maxx);
	int miny = std::min(t0.y, t1.y);
	miny = std::min(miny, t2.y);
	miny = std::max(miny, 0);
	int maxy = std::max(t0.y, t1.y);
	maxy = std::max(maxy, t2.y);
	maxy = std::min(height - 1, maxy);

	for (int x = minx; x <= maxx; x++)
	{
		for (int y = miny; y <= maxy; y++)
		{
			Vec3f res = barycentric(t0, t1, t2, Vec2i(x, y));
			if (res.x < 0 || res.y < 0 || res.z < 0)
				continue;
			image.set(x, y, color);
		}
	}
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

	TGAImage image(width, height, TGAImage::RGB);
	// for (int i = 0; i < model->nfaces(); i++)
	// {
	// 	std::vector<int> face = model->face(i);
	// 	for (int j = 0; j < 3; j++)
	// 	{
	// 		Vec3f v0 = model->vert(face[j]);
	// 		Vec3f v1 = model->vert(face[(j + 1) % 3]);
	// 		int x0 = (v0.x + 1.) * width / 2.;
	// 		int y0 = (v0.y + 1.) * height / 2.;
	// 		int x1 = (v1.x + 1.) * width / 2.;
	// 		int y1 = (v1.y + 1.) * height / 2.;
	// 		line(x0, y0, x1, y1, image, white);
	// 	}
	// }
	Vec2i t0[3] = {Vec2i(10, 70), Vec2i(50, 160), Vec2i(70, 80)};
	Vec2i t1[3] = {Vec2i(180, 50), Vec2i(150, 1), Vec2i(70, 180)};
	Vec2i t2[3] = {Vec2i(180, 150), Vec2i(120, 160), Vec2i(130, 180)};
	// calculateIntersection(50, t0[0], t0[1], t0[2]);
	triangle(t0[0], t0[1], t0[2], image, red);
	triangle(t1[0], t1[1], t1[2], image, white);
	triangle(t2[0], t2[1], t2[2], image, green);
	image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
	image.write_tga_file("output.tga");
	delete model;
}
