#include <vector>
#include <iostream>

#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "our_gl.h"

Model *model = NULL;
const int width = 800;
const int height = 800;

Vec3f light_dir = Vec3f(1, 1, 1).normalize();
Vec3f camera(1, 1, 3);
Vec3f center(0, 0, 0);
Vec3f up(0, 1, 0);

struct GouraudShader : public IShader
{
	Vec3f varying_intensity;	 // written by vertex shader, read by fragment shader
	mat<2, 3, float> varying_uv; // same as above

	virtual Vec4f vertex(int iface, int nthvert)
	{
		varying_uv.set_col(nthvert, model->uv(iface, nthvert));
		Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert));
		// printf("%f %f %f %f ", gl_Vertex[0], gl_Vertex[1], gl_Vertex[2], gl_Vertex[3]);
		//  read the vertex from .obj file
		gl_Vertex = Viewport * Projection * ModelView * gl_Vertex; // transform it to screen coordinates
		// gl_Vertex = Viewport * gl_Vertex;													   // transform it to screen coordinates
		varying_intensity[nthvert] = std::max(0.f, model->normal(iface, nthvert) * light_dir); // get diffuse lighting intensity
		// printf("%f %f %f %f\n", gl_Vertex[0], gl_Vertex[1], gl_Vertex[2], gl_Vertex[3]);
		return gl_Vertex;
	}

	virtual bool fragment(Vec3f bar, TGAColor &color)
	{
		float intensity = varying_intensity * bar; // interpolate intensity for the current pixel
		// printf("%f\n", intensity);
		Vec2f uv = varying_uv * bar;
		color = model->diffuse(uv) * intensity; // well duh
		return false;							// no, we do not discard this pixel
	}
};

struct Shader : public IShader
{
	mat<2, 3, float> varying_uv;  // same as above
	mat<4, 4, float> uniform_M;	  //  Projection*ModelView
	mat<4, 4, float> uniform_MIT; // (Projection*ModelView).invert_transpose()

	virtual Vec4f vertex(int iface, int nthvert)
	{
		varying_uv.set_col(nthvert, model->uv(iface, nthvert));
		Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert)); // read the vertex from .obj file
		return Viewport * Projection * ModelView * gl_Vertex;	 // transform it to screen coordinates
	}

	virtual bool fragment(Vec3f bar, TGAColor &color)
	{
		Vec2f uv = varying_uv * bar; // interpolate uv for the current pixel
		Vec3f n = proj<3>(uniform_MIT * embed<4>(model->normal(uv))).normalize();
		Vec3f l = proj<3>(uniform_M * embed<4>(light_dir)).normalize();
		float intensity = std::max(0.f, n * l);
		color = model->diffuse(uv) * intensity; // well duh
		return false;							// no, we do not discard this pixel
	}
};

struct PhongShader : public IShader
{
	mat<2, 3, float> varying_uv;  // same as above
	mat<4, 4, float> uniform_M;	  //  Projection*ModelView
	mat<4, 4, float> uniform_MIT; // (Projection*ModelView).invert_transpose()

	virtual Vec4f vertex(int iface, int nthvert)
	{
		varying_uv.set_col(nthvert, model->uv(iface, nthvert));
		Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert)); // read the vertex from .obj file
		return Viewport * Projection * ModelView * gl_Vertex;	 // transform it to screen coordinates
	}

	virtual bool fragment(Vec3f bar, TGAColor &color)
	{
		Vec2f uv = varying_uv * bar;
		Vec3f n = proj<3>(uniform_MIT * embed<4>(model->normal(uv))).normalize();
		Vec3f l = proj<3>(uniform_M * embed<4>(light_dir)).normalize();
		Vec3f r = (n * (n * l * 2.f) - l).normalize(); // reflected light
		float spec = pow(std::max(r.z, 0.0f), model->specular(uv));
		float diff = std::max(0.f, n * l);
		TGAColor c = model->diffuse(uv);
		color = c;
		for (int i = 0; i < 3; i++)
			color[i] = std::min<float>(1 + c[i] * (diff + .6 * spec), 255);
		return false;
	}
};
int main(int argc, char **argv)
{
	if (2 == argc)
	{
		model = new Model(argv[1]);
	}
	else
	{
		model = new Model("obj/african_head.obj");
		// model = new Model("obj/diablo3_pose/diablo3_pose.obj");
	}

	TGAImage image(width, height, TGAImage::RGB);
	TGAImage zbuffer(width, height, TGAImage::GRAYSCALE);
	// GouraudShader shader = GouraudShader();
	PhongShader shader = PhongShader();

	viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
	lookat(camera, center, up);
	projection(-1. / (camera - center).norm());
	shader.uniform_M = Projection * ModelView;
	shader.uniform_MIT = (Projection * ModelView).invert_transpose();
	for (int i = 0; i < model->nfaces(); i++)
	{
		Vec4f screen_coords[3];
		for (int j = 0; j < 3; j++)
		{
			screen_coords[j] = shader.vertex(i, j);
		}
		triangle(screen_coords, shader, image, zbuffer);
	}
	printf("end");
	image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
	image.write_tga_file("output.tga");
	zbuffer.flip_vertically();
	zbuffer.write_tga_file("zbuffer.tga");
	delete model;
}
