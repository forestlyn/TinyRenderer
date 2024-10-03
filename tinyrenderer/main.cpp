#include <vector>
#include <cmath>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
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
	for (int i = 0; i < model->nfaces(); i++)
	{
		std::vector<int> face = model->face(i);
		for (int j = 0; j < 3; j++)
		{
			Vec3f v0 = model->vert(face[j]);
			Vec3f v1 = model->vert(face[(j + 1) % 3]);
			int x0 = (v0.x + 1.) * width / 2.;
			int y0 = (v0.y + 1.) * height / 2.;
			int x1 = (v1.x + 1.) * width / 2.;
			int y1 = (v1.y + 1.) * height / 2.;
			line(x0, y0, x1, y1, image, white);
		}
	}

	image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
	image.write_tga_file("output.tga");
	delete model;
}
