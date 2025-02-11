#include <vector>
#include <limits>
#include <math.h>
#include <iostream>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "our_gl.h"
#include <time.h>

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
		float z = bar * varying_tri[2];
		color = TGAColor(255, 255, 255) * ((z + 1.f) / 2.f);
		return false;
	}
};
struct Shader : IShader
{
	mat<2, 3, float> varying_uv;
	mat<4, 3, float> varying_tri;
	virtual Vec4f vertex(int iface, int nthvert)
	{
		varying_uv.set_col(nthvert, model->uv(iface, nthvert));
		Vec4f gl_Vertex = Projection * ModelView * embed<4>(model->vert(iface, nthvert));
		varying_tri.set_col(nthvert, gl_Vertex);
		return Viewport * gl_Vertex;
	}

	virtual bool fragment(Vec3f gl_FragCoord, Vec3f bar, TGAColor &color)
	{
		Vec2f uv = varying_uv * bar;
		if (std::abs(shadowbuffer[int(gl_FragCoord.x + gl_FragCoord.y * width)] - gl_FragCoord.z < 1e-2))
		{
			// printf("occl\n");
			occl.set((int)(uv.x * 1024), (int)(uv.y * 1024), TGAColor(255));
		}
		color = TGAColor(255, 0, 0);
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
	u = u * 2.0 - 1.0;
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
		// model = new Model("obj/african_head/african_head.obj");
		model = new Model("obj/diablo3_pose/diablo3_pose.obj");
		// model = new Model("obj/boggie/body.obj");
		// model = new Model("obj/floor.obj");
	}

	float *zbuffer = new float[width * height];
	shadowbuffer = new float[width * height];
	for (int i = width * height; i--; zbuffer[i] = -std::numeric_limits<float>::max())
		;

	// printf("depth:%f\n", depth);
	srand(time(NULL));
	int randomPointsNum = 100;

	TGAImage sphereImage(width, height, TGAImage::RGB);
	// draw sphere
	viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
	lookat(camera, center, up);
	projection(-1. / (camera - center).norm());
	Matrix M = Viewport * Projection * ModelView;
	for (int i = 0; i < randomPointsNum; i++)
	{
		Vec3f pts = randomPointInSphere();
		Vec4f point = M * embed<4>(pts, 1.0f);
		// printf("%d %d %d\n", int(point[0] / point[3]), int(point[1] / point[3]), int(point[2] / point[3]));
		sphereImage.set(int(point[0] / point[3]), int(point[1] / point[3]), white);
	}
	sphereImage.flip_vertically(); // i want to have the origin at the left bottom corner of the image
	sphereImage.write_tga_file("sphereImage.tga");
	sphereImage.clear();
	for (int i = 0; i < randomPointsNum; i++)
	{
		Vec3f pts = randomPointInSphereCorrect();
		Vec4f point = M * embed<4>(pts, 1.0f);
		// printf("%d %d %d\n", int(point[0] / point[3]), int(point[1] / point[3]), int(point[2] / point[3]));
		sphereImage.set(int(point[0] / point[3]), int(point[1] / point[3]), white);
	}
	sphereImage.flip_vertically(); // i want to have the origin at the left bottom corner of the image
	sphereImage.write_tga_file("sphereImageCorrect.tga");

	M = Viewport * Projection * ModelView;
	randomPointsNum = 1000;
	{

		for (int iter = 1; iter <= randomPointsNum; iter++)
		{
			printf("iter:%d\n", iter);
			for (int i = 0; i < 3; i++)
				up[i] = (float)rand() / (float)RAND_MAX;
			camera = randomPointInSphereCorrect();
			camera.y = std::abs(camera.y);
			for (int i = width * height; i--; shadowbuffer[i] = zbuffer[i] = -std::numeric_limits<float>::max())
				;

			TGAImage frame(width, height, TGAImage::RGB);
			lookat(camera, center, up);
			viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
			projection(0);
			M = Viewport * Projection * ModelView;

			ZShader zshader;
			// printf("model->nfaces():%d\n", model->nfaces());
			for (int i = 0; i < model->nfaces(); i++)
			{
				Vec4f screen_coords[3];

				for (int j = 0; j < 3; j++)
				{
					screen_coords[j] = zshader.vertex(i, j);
				}
				triangle(screen_coords, zshader, frame, shadowbuffer);
			}
			frame.flip_vertically();
			frame.write_tga_file("framebuffer.tga");
			Shader shader;
			occl.clear();
			for (int i = 0; i < model->nfaces(); i++)
			{
				Vec4f screen_coords[3];
				for (int j = 0; j < 3; j++)
				{
					screen_coords[j] = shader.vertex(i, j);
				}
				triangle(screen_coords, shader, frame, zbuffer);
			}
			occl.write_tga_file("occl.tga");

			//        occl.gaussian_blur(5);
			for (int i = 0; i < 1024; i++)
			{
				for (int j = 0; j < 1024; j++)
				{
					float tmp = total.get(i, j)[0];
					total.set(i, j, TGAColor((tmp * (iter - 1) + occl.get(i, j)[0]) / (float)iter + .5f));
				}
			}
		}
		total.flip_vertically();
		total.write_tga_file("occlusion.tga");
		occl.flip_vertically();
		occl.write_tga_file("occl.tga");
	}
}
