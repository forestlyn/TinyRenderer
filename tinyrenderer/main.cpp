#include <vector>
#include <limits>
#include <math.h>
#include <iostream>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "our_gl.h"

Model *model = NULL;
const int width = 800;
const int height = 800;

float *shadowbuffer = NULL;

Vec3f light_dir(1, 1, 0);
Vec3f camera(1, 1, 4);
Vec3f center(0, 0, 0);
Vec3f up(0, 1, 0);

TGAImage total(1024, 1024, TGAImage::GRAYSCALE);
TGAImage occl(1024, 1024, TGAImage::GRAYSCALE);

struct DepthShader : public IShader
{
	mat<3, 3, float> varying_tri;

	DepthShader() : varying_tri() {}

	virtual Vec4f vertex(int iface, int nthvert)
	{
		Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert));   // read the vertex from .obj file
		gl_Vertex = Viewport * Projection * ModelView * gl_Vertex; // transform it to screen coordinates
		varying_tri.set_col(nthvert, proj<3>(gl_Vertex / gl_Vertex[3]));
		return gl_Vertex;
	}

	virtual bool fragment(Vec3f bar, TGAColor &color)
	{
		Vec3f p = varying_tri * bar;
		color = TGAColor(255, 255, 255) * (p.z / depth);
		return false;
	}
};

struct VisibleShader : IShader
{
	mat<2, 3, float> varying_uv;
	mat<4, 3, float> varying_tri;
	TGAImage image;
	VisibleShader()
	{
	}
	virtual Vec4f vertex(int iface, int nthvert)
	{
		Vec3f vert = model->vert(iface, nthvert);
		Vec2f uv = model->uv(iface, nthvert);
		varying_uv.set_col(nthvert, uv);
		Vec4f screen_pts = Projection * ModelView * embed<4>(vert, 1.f);
		varying_tri.set_col(nthvert, screen_pts);
	}
	virtual bool fragment(Vec3f bar, TGAColor &color)
	{
		Vec2f uv = varying_uv * bar;
		Vec4f pts = varying_tri * bar;
		return false;
	}
};

Vec3f randomPointInSphere()
{
	float u = (float)rand() / (float)RAND_MAX;
	float v = (float)rand() / (float)RAND_MAX;
	float alpha = u * 2 * M_PI;
	float beta = v * M_PI;
	return Vec3f(sin(beta) * cos(alpha), sin(beta) * sin(alpha), cos(beta));
}

Vec3f randomPointInSphereCorrect()
{
	float u = (float)rand() / (float)RAND_MAX;
	float v = (float)rand() / (float)RAND_MAX;
	u = u * 2 - 1;
	float theta = v * 2 * M_PI;
	float r = sqrt(1 - u * u);
	return Vec3f(r * cos(theta), r * sin(theta), u);
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
		model = new Model("obj/diablo3_pose/diablo3_pose.obj");
		// model = new Model("obj/boggie/body.obj");
		// model = new Model("obj/floor.obj");
	}
	// printf("depth:%f\n", depth);

	const int randomPointsNum = 1000;

	TGAImage sphereImage(width, height, TGAImage::RGB);
	// draw sphere
	viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
	lookat(camera, center, up);
	projection(-1. / (camera - center).norm());
	Matrix M = Viewport * Projection * ModelView;
	for (int i = 0; i < randomPointsNum; i++)
	{
		Vec3f pts = randomPointInSphereCorrect();
		Vec4f point = M * embed<4>(pts, 1.0f);
		// printf("%d %d %d\n", int(point[0] / point[3]), int(point[1] / point[3]), int(point[2] / point[3]));
		sphereImage.set(int(point[0] / point[3]), int(point[1] / point[3]), white);
	}
	sphereImage.flip_vertically(); // i want to have the origin at the left bottom corner of the image
	sphereImage.write_tga_file("sphereImage.tga");

	M = Viewport * Projection * ModelView;

	{
		// rendering the frame buffer
		TGAImage image(width, height, TGAImage::RGB);
		// Shader shader(ModelView, (Projection * ModelView).invert_transpose(), M * (Viewport * Projection * ModelView).invert());

		viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
		lookat(camera, center, up);
		projection(-1. / (camera - center).norm());
		light_dir = proj<3>((Projection * ModelView * embed<4>(light_dir, 0.f))).normalize();

		float *newzbuffer = new float[width * height];
		for (int i = width * height; i--; newzbuffer[i] = -std::numeric_limits<float>::max())
			;

		// for (int i = 0; i < model->nfaces(); i++)
		// {
		// 	Vec4f screen_coords[3];

		// 	for (int j = 0; j < 3; j++)
		// 	{
		// 		screen_coords[j] = shader.vertex(i, j);
		// 	}
		// 	triangle(screen_coords, shader, image, newzbuffer);
		// }

		printf("end");
		image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
		image.write_tga_file("output.tga");
	}

	delete model;
}
