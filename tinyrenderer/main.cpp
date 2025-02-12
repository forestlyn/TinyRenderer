#include <vector>
#include <limits>
#include <math.h>
#include <iostream>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "our_gl.h"
#include <time.h>

using namespace std;
Model *model = NULL;
const int width = 800;
const int height = 800;

Vec3f light_dir(1, 1, 0);
Vec3f camera(1, 1, 4);
Vec3f center(0, 0, 0);
Vec3f up(0, 1, 0);

TGAImage output(width, height, TGAImage::RGB);

struct ZShader : public IShader
{
	mat<4, 3, float> varying_tri;
	virtual Vec4f vertex(int iface, int nthvert)
	{
		Vec4f gl_Vertex = Projection * ModelView * embed<4>(model->vert(iface, nthvert));
		varying_tri.set_col(nthvert, gl_Vertex);
		return Viewport * gl_Vertex;
	}
	virtual bool fragment(Vec3f gl_FragCoord, Vec3f bar, TGAColor &color)
	{
		// float z = bar * varying_tri[2];
		// color = TGAColor(255, 255, 255) * ((z + 1.f) / 2.f);
		color = TGAColor(0, 0, 0);
		return false;
	}
};
float max_elevation_angle(float *zbuffer, Vec2f pos, Vec2f dir)
{
	float angle = 0;
	for (float t = 0; t < 1000.; t += 1)
	{
		Vec2f tempPos = pos + Vec2f(dir.x * t, dir.y * t);
		// printf("%f\n", t);
		if (tempPos.x < 0 || tempPos.y < 0 || tempPos.x >= width || tempPos.y >= height)
			return angle;
		double distance = sqrt(pow((tempPos - pos).x, 2) + pow(tempPos.y - pos.y, 2));
		if (distance < 1)
			continue;
		float z_delta = zbuffer[(int)tempPos.x + int(tempPos.y) * width] - zbuffer[(int)(pos.x) + int(pos.y) * width];
		angle = max(angle, atanf(z_delta / distance));
	}
	return angle;
}
int main(int argc, char **argv)
{
	if (2 == argc)
	{
		model = new Model(argv[1]);
	}
	else
	{
		model = new Model("obj/african_head/african_head.obj");
		// model = new Model("obj/diablo3_pose/diablo3_pose.obj");
		//  model = new Model("obj/boggie/body.obj");
		//  model = new Model("obj/floor.obj");
	}
	float *zbuffer = new float[width * height];
	for (int i = width * height; i--; zbuffer[i] = -std::numeric_limits<float>::max())
		;
	camera = Vec3f(1, 1, 4);
	lookat(camera, center, up);
	viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
	projection(-1. / (camera - center).norm());
	ZShader zshader;
	for (int i = 0; i < model->nfaces(); i++)
	{
		Vec4f screen_coords[3];
		for (int j = 0; j < 3; j++)
		{
			screen_coords[j] = zshader.vertex(i, j);
		}
		triangle(screen_coords, zshader, output, zbuffer);
	}
	int dir_count = 8;
	for (int i = 0; i < width; i++)
	{
		for (int j = 0; j < height; j++)
		{
			if (zbuffer[i + j * width] < -1e5)
				continue;
			// printf("%d %d\n", i, j);
			float t = 0;
			for (float angle = 0; angle < M_PI * 2 - 1e-4; angle += M_PI / dir_count * 2)
			{
				t += M_PI / 2 - max_elevation_angle(zbuffer, Vec2f(i, j), Vec2f(cos(angle), sin(angle)));
			}
			t /= (M_PI / 2) * dir_count;
			t = pow(t, 1);
			output.set(i, j, TGAColor(t * 255, t * 255, t * 255));
		}
	}
	output.flip_vertically();
	output.write_tga_file("output.tga");
}