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
bool inRange(int x, int x0, int x1)
{
	if (x <= x0 && x >= x1)
		return true;
	if (x >= x0 && x <= x1)
		return true;
	return false;
}
Vec3i calculateIntersection(int x, Vec2i t0, Vec2i t1, Vec2i t2)
{
	float dx1 = t1.x - t0.x;
	float dx2 = t2.x - t1.x;
	float dx3 = t0.x - t2.x;
	float dy1 = t1.y - t0.y;
	float dy2 = t2.y - t1.y;
	float dy3 = t0.y - t2.y;
	float y1 = -1;
	// 注意dx=0
	if (dx1 != 0 && inRange(x, t0.x, t1.x))
	{
		y1 = (x - t0.x) / dx1 * dy1 + t0.y;
	}
	float y2 = -1;
	if (dx2 != 0 && inRange(x, t1.x, t2.x))
	{
		y2 = (x - t1.x) / dx2 * dy2 + t1.y;
	}
	float y3 = -1;
	if (dx3 != 0 && inRange(x, t0.x, t2.x))
	{
		y3 = (x - t2.x) / dx3 * dy3 + t2.y;
	}
	return Vec3i(y1, y2, y3);
}

void triangle(Vec2i t0, Vec2i t1, Vec2i t2, TGAImage &image, TGAColor color)
{
	int minx = t0.x < t1.x ? t0.x : t1.x;
	minx = minx < t2.x ? minx : t2.x;
	int maxx = t0.x < t1.x ? t1.x : t0.x;
	maxx = maxx < t2.x ? t2.x : maxx;

	for (int x = minx; x <= maxx; x++)
	{
		Vec3i res = calculateIntersection(x, t0, t1, t2);
		if (res.x == -1)
		{
			line(x, res.y, x, res.z, image, color);
		}
		if (res.y == -1)
		{
			line(x, res.x, x, res.z, image, color);
		}
		if (res.z == -1)
		{
			line(x, res.x, x, res.y, image, color);
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
	triangle(t0[0], t0[1], t0[2], image, red);
	triangle(t1[0], t1[1], t1[2], image, white);
	triangle(t2[0], t2[1], t2[2], image, green);
	image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
	image.write_tga_file("output.tga");
	delete model;
}
