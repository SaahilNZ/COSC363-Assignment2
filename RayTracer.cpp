/*==================================================================================
* COSC 363  Computer Graphics (2020)
* Department of Computer Science and Software Engineering, University of Canterbury.
*
* A basic ray tracer
* See Lab07.pdf, Lab08.pdf for details.
*===================================================================================
*/
#include <iostream>
#include <cmath>
#include <vector>
#include <glm/glm.hpp>
#include <GL/freeglut.h>
#include <thread>

#include "Cylinder.h"
#include "Noise.h"
#include "Plane.h"
#include "Ray.h"
#include "Sphere.h"
#include "SceneObject.h"
#include "TextureBMP.h"
#include "Torus.h"
using namespace std;

#define clamp(val, min, max) val < min ? min : (val > max ? max : val)
#define colFromBytes(r, g, b) glm::vec3(r / 255.0f, g / 255.0f, b / 255.0f)

#define NUM_THREADS 4
#define BOARD_WIDTH 5
#define BOARD_PRIMARY_COLOUR glm::vec3(0.8, 0.8, 0.8)
#define BOARD_SECONDARY_COLOUR glm::vec3(0.25, 0.25, 0.25)
#define PI acos(-1)
#define NOISE_WIDTH 1024
#define NOISE_HEIGHT 1024

const bool ENABLE_AA = false;
const float WIDTH = 40.0;  
const float HEIGHT = 40.0;
const float EDIST = 40.0;
const int NUMDIV = 1024;
const int MAX_STEPS = 5;
const float XMIN = -WIDTH * 0.5;
const float XMAX =  WIDTH * 0.5;
const float YMIN = -HEIGHT * 0.5;
const float YMAX =  HEIGHT * 0.5;

vector<SceneObject*> sceneObjects;
TextureBMP brickAlbedo;
TextureBMP brickNormal;
TextureBMP bronzeAlbedo;
TextureBMP bronzeNormal;
TextureBMP bronzeMetallic;
bool traced = false;
bool exported = false;
glm::vec3 pixels[NUMDIV][NUMDIV];

float **marbleNoise;
glm::vec3 marbleColours[NOISE_WIDTH][NOISE_HEIGHT];

//---The most important function in a ray tracer! ---------------------------------- 
//   Computes the colour value obtained by tracing a ray and finding its 
//     closest point of intersection with objects in the scene.
//----------------------------------------------------------------------------------
glm::vec3 trace(Ray ray, int step)
{
	// glm::vec3 backgroundCol(0);						   	//Background colour = (0,0,0)
	glm::vec3 backgroundCol = colFromBytes(135, 206, 235);	//Background colour = (135,206,235)
	glm::vec3 lightPos(10, 40, -3);					       	//Light's position
	glm::vec3 color(0);
	SceneObject* obj;
	float ambientLevel = 0.2;
	float texcoords;
	float texcoordt;

    ray.closestPt(sceneObjects);					 		//Compare the ray with all objects in the scene
    if(ray.index == -1) return backgroundCol;		 		//no intersection
	obj = sceneObjects[ray.index];					 		//object on which the closest point of intersection is found
	glm::vec3 baseColor = obj->getColor();

	bool differentColour = false;
	bool differentNormal = false;
	bool differentMetallic = false;
	TextureBMP normalBmp;
	TextureBMP metallicBmp;

	if (ray.index == 0)
	{
		int iz = (ray.hit.z < 0 ? -ray.hit.z + BOARD_WIDTH : ray.hit.z) / BOARD_WIDTH;
		int ix = (ray.hit.x < 0 ? -ray.hit.x + BOARD_WIDTH : ray.hit.x) / BOARD_WIDTH;
		int k = (iz % 2) ^ (ix % 2);
		baseColor = (k == 0) ? BOARD_PRIMARY_COLOUR : BOARD_SECONDARY_COLOUR;

		differentColour = true;
	}
	else if (ray.index == 1)
	{
		float x1 = -200.0;
		float x2 = -100.0;
		float y1 = -15.0;
		float y2 = 85.0;

		texcoords = fmod((ray.hit.x - x1) / (x2 - x1), 1.0);
		texcoordt = fmod((ray.hit.y - y1) / (y2 - y1), 1.0);
		
		baseColor = brickAlbedo.getColorAt(texcoords, texcoordt);
		// baseColor = glm::vec3(0.5);

		differentColour = true;
		differentNormal = true;
		normalBmp = brickNormal;
	}
	else if (ray.index == 4)
	{
		glm::vec3 origin = glm::vec3(10, 10, -60);
		glm::vec3 localHit = glm::normalize(ray.hit - origin);

		texcoords = 0.5 + atan2(localHit.x, localHit.z) / (2 * PI); 
		texcoordt = 0.5 - asin(localHit.y) / PI;
		
		int xPixel = (int)glm::round(texcoords * NOISE_WIDTH);
		int yPixel = NOISE_HEIGHT - (int)glm::round(texcoordt * NOISE_HEIGHT);
		// glm::vec3 col1 = colFromBytes(255, 108, 89);
		// glm::vec3 col2 = colFromBytes(104, 39, 0);
		glm::vec3 col1 = baseColor;
		glm::vec3 col2(0, 1, 1);
		float frac = marbleColours[yPixel][xPixel].r;
		baseColor = (col1 * frac) + (col2 * (1 - frac));
		// baseColor = marbleColours[yPixel][xPixel] * baseColor;
		
		differentColour = true;
	}
	else if (ray.index == 5)
	{
		glm::vec3 origin = glm::vec3(-8, -15, -20);
		float height = 12;
		float radius = 2;
		glm::vec3 localHit = ray.hit - origin;
		glm::vec3 ignoreY = glm::vec3(localHit.x / radius, 0, localHit.z / radius);

		if (localHit.y < height)
		{
			float sScale = 2.0f;
			float tScale = 2.0f;
			texcoords = fmod((0.5 + atan2(ignoreY.x, ignoreY.z) / (2 * PI)) * sScale, 1.0);
			texcoordt = fmod(localHit.y / height * tScale, 1.0);

			baseColor = bronzeAlbedo.getColorAt(texcoords, texcoordt);
			// baseColor = glm::vec3(0.5);
			differentColour = true;
			differentNormal = true;
			differentMetallic = true;
			normalBmp = bronzeNormal;
			metallicBmp = bronzeMetallic;
		}
	}

	if (differentColour)
	{
		if (differentNormal)
		{
			color = obj->lighting(lightPos, -ray.dir, ray.hit, baseColor,
				normalBmp.getColorAt(texcoords, texcoordt));
		}
		else
		{
			color = obj->lighting(lightPos, -ray.dir, ray.hit, baseColor);
		}
	}
	else
	{
		color = obj->lighting(lightPos, -ray.dir, ray.hit);
	}
	
	glm::vec3 lightVec = lightPos - ray.hit;
	Ray shadowRay(ray.hit, lightVec);
	shadowRay.closestPt(sceneObjects);
	if (shadowRay.index > -1 && shadowRay.dist < glm::length(lightVec))
	{
		SceneObject* hitObject = sceneObjects[shadowRay.index];
		if (hitObject->isTransparent() || hitObject->isRefractive())
		{
			glm::vec3 hitCol = hitObject->getColor();
			color = glm::vec3(
				hitObject->getTransparencyCoeff() * ((hitObject->getTransparencyCoeff()) *
					baseColor.r + ((1 - hitObject->getTransparencyCoeff()) * 0.5 * hitCol.r)),
				hitObject->getTransparencyCoeff() * ((hitObject->getTransparencyCoeff()) *
					baseColor.g + ((1 - hitObject->getTransparencyCoeff()) * 0.5 * hitCol.g)),
				hitObject->getTransparencyCoeff() * ((hitObject->getTransparencyCoeff()) *
					baseColor.b + ((1 - hitObject->getReflectionCoeff()) * 0.5 * hitCol.b)));
		}
		else
		{
			color = glm::vec3(
				0.2 * baseColor.r,
				0.2 * baseColor.g,
				0.2 * baseColor.b);
		}
	}

	if (obj->isReflective() && step < MAX_STEPS)
	{
		float rho = obj->getReflectionCoeff();
		if (differentMetallic)
		{
			rho = rho * (metallicBmp.getColorAt(texcoords, texcoordt)).r;
		}
		glm::vec3 normalVec(0);
		if (differentNormal)
		{
			normalVec = obj->normal(ray.hit,
				normalBmp.getColorAt(texcoords, texcoordt));
		}
		else
		{
			normalVec = obj->normal(ray.hit);
		}
		
		glm::vec3 reflectedDir = glm::reflect(ray.dir, normalVec);
		Ray reflectedRay(ray.hit, reflectedDir);
		glm::vec3 reflectedColor = trace(reflectedRay, step + 1);
		color = color + (rho * reflectedColor);
	}

	if (obj->isTransparent() && step < MAX_STEPS)
	{
		float coeff = obj->getTransparencyCoeff();
		Ray transparentRay(ray.hit, ray.dir);
		glm::vec3 transparentColour = trace(transparentRay, step + 1);
		color = (color * (1 - coeff)) + (coeff * transparentColour);
	}

	if (obj->isRefractive() && step < MAX_STEPS)
	{
		float coeff = obj->getRefractionCoeff();
		float eta = 1.0f / obj->getRefractiveIndex();
		glm::vec3 n = obj->normal(ray.hit);
		glm::vec3 g = glm::refract(ray.dir, n, eta);
		Ray refrRay(ray.hit, g);
		refrRay.closestPt(sceneObjects);

		if (obj->getType() == PlaneObject)
		{
			if (refrRay.index > -1 && sceneObjects[refrRay.index]->getType() == PlaneObject)
			{
				if (sceneObjects[refrRay.index]->isRefractive())
				{
					glm::vec3 m = obj->normal(refrRay.hit);
					glm::vec3 h = glm::refract(g, -m, 1.0f / eta);

					Ray finalRay(refrRay.hit, h);
					glm::vec3 refractedColour = trace(finalRay, step + 1);
					color = (color * (1 - coeff)) + (coeff * refractedColour);
				}
				else
				{
					glm::vec3 refractedColour = trace(refrRay, step + 1);
					color = (color * (1 - coeff)) + (coeff * refractedColour);
				}
			}
			else
			{
				glm::vec3 refractedColour = trace(refrRay, step + 1);
				color = (color * (1 - coeff)) + (coeff * refractedColour);
			}
		}
		else
		{
			glm::vec3 m = obj->normal(refrRay.hit);
			glm::vec3 h = glm::refract(g, -m, 1.0f / eta);

			Ray finalRay(refrRay.hit, h);
			glm::vec3 refractedColour = trace(finalRay, step + 1);
			color = (color * (1 - coeff)) + (coeff * refractedColour);
		}
	}

	return color;
}

void traceScene()
{
	int width = NUMDIV / NUM_THREADS;

	std::thread threads[NUM_THREADS];
	auto threadFunc = [](int i, int width)
	{
		float xp, yp;  //grid point
		float cellX = (XMAX-XMIN)/NUMDIV;  //cell width
		float cellY = (YMAX-YMIN)/NUMDIV;  //cell height
		glm::vec3 eye(0., 0., 0.);

		// if (i > 1) return;
		for (int x = (i * width); x < ((i + 1) * width); x++)
		{
			xp = XMIN + x*cellX;
			for (int y = 0; y < NUMDIV; y++)
			{
				yp = YMIN + y*cellY;

				if (ENABLE_AA)
				{
					glm::vec3 dir1(xp+0.25*cellX, yp+0.25*cellY, -EDIST);
					glm::vec3 dir2(xp+0.25*cellX, yp+0.75*cellY, -EDIST);
					glm::vec3 dir3(xp+0.75*cellX, yp+0.25*cellY, -EDIST);
					glm::vec3 dir4(xp+0.75*cellX, yp+0.75*cellY, -EDIST);

					Ray ray1 = Ray(eye, dir1);
					Ray ray2 = Ray(eye, dir2);
					Ray ray3 = Ray(eye, dir3);
					Ray ray4 = Ray(eye, dir4);

					glm::vec3 col1 = trace (ray1, 1);
					glm::vec3 col2 = trace (ray2, 1);
					glm::vec3 col3 = trace (ray3, 1);
					glm::vec3 col4 = trace (ray4, 1);

					pixels[x][y] = glm::vec3((col1.r + col2.r + col3.r + col4.r) / 4.0,
											(col1.g + col2.g + col3.g + col4.g) / 4.0,
											(col1.b + col2.b + col3.b + col4.b) / 4.0);
				}
				else
				{
					glm::vec3 dir(xp+0.5*cellX, yp+0.5*cellY, -EDIST);
					Ray ray = Ray(eye, dir);
					glm::vec3 col = trace(ray, 1);
					pixels[x][y] = col;
				}
			}
		}
	};

	for (int i = 0; i < NUM_THREADS; i++)
	{
		threads[i] = std::thread(threadFunc, i, width);
	}

	for (int i = 0; i < NUM_THREADS; i++)
	{
		threads[i].join();
	}
}

//---The main display module -----------------------------------------------------------
// In a ray tracing application, it just displays the ray traced image by drawing
// each cell as a quad.
//---------------------------------------------------------------------------------------
void display()
{
	float xp, yp;  //grid point
	float cellX = (XMAX-XMIN)/NUMDIV;  //cell width
	float cellY = (YMAX-YMIN)/NUMDIV;  //cell height

	glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

	glBegin(GL_QUADS);  //Each cell is a tiny quad.

	if (!traced)
	{
		traceScene();
		traced = true;
	}

	for(int i = 0; i < NUMDIV; i++)	//Scan every cell of the image plane
	{
		xp = XMIN + i*cellX;
		for(int j = 0; j < NUMDIV; j++)
		{
			yp = YMIN + j*cellY;

			glColor3f(pixels[i][j].r,
					  pixels[i][j].g,
					  pixels[i][j].b);
			glVertex2f(xp, yp);				//Draw each cell with its color value
			glVertex2f(xp+cellX, yp);
			glVertex2f(xp+cellX, yp+cellY);
			glVertex2f(xp, yp+cellY);
        }
    }

    glEnd();
    glFlush();
}

void drawCube()
{
	// Draw a cube
	Plane *cubeBottom = new Plane(glm::vec3(-15, -14.99, -50),
								  glm::vec3(-5, -14.99, -50),
								  glm::vec3(-5, -14.99, -60),
								  glm::vec3(-15, -14.99, -60));
	cubeBottom->setColor(glm::vec3(1, 0, 0));
	cubeBottom->setRefractivity(true, 0.5, 1.03);
	cubeBottom->setReflectivity(true, 0.8);
	sceneObjects.push_back(cubeBottom);
	Plane *cubeTop = new Plane(glm::vec3(-15, -5, -50),
							   glm::vec3(-5, -5, -50),
							   glm::vec3(-5, -5, -60),
							   glm::vec3(-15, -5, -60));
	cubeTop->setColor(glm::vec3(1, 0, 0));
	cubeTop->setRefractivity(true, 0.5, 1.03);
	cubeTop->setReflectivity(true, 0.8);
	sceneObjects.push_back(cubeTop);
	Plane *cubeBack = new Plane(glm::vec3(-15, -15, -60),
							   glm::vec3(-5, -15, -60),
							   glm::vec3(-5, -5, -60),
							   glm::vec3(-15, -5, -60));
	cubeBack->setColor(glm::vec3(1, 0, 0));
	cubeBack->setRefractivity(true, 0.5, 1.03);
	cubeBack->setReflectivity(true, 0.8);
	sceneObjects.push_back(cubeBack);
	Plane *cubeFront = new Plane(glm::vec3(-15, -15, -50),
							   glm::vec3(-5, -15, -50),
							   glm::vec3(-5, -5, -50),
							   glm::vec3(-15, -5, -50));
	cubeFront->setColor(glm::vec3(1, 0, 0));
	cubeFront->setRefractivity(true, 0.5, 1.03);
	cubeFront->setReflectivity(true, 0.8);
	sceneObjects.push_back(cubeFront);
	Plane *cubeLeft = new Plane(glm::vec3(-15, -15, -50),
							   glm::vec3(-15, -15, -60),
							   glm::vec3(-15, -5, -60),
							   glm::vec3(-15, -5, -50));
	cubeLeft->setColor(glm::vec3(1, 0, 0));
	cubeLeft->setRefractivity(true, 0.5, 1.03);
	cubeLeft->setReflectivity(true, 0.8);
	Plane *cubeRight = new Plane(glm::vec3(-5, -15, -50),
							   glm::vec3(-5, -15, -60),
							   glm::vec3(-5, -5, -60),
							   glm::vec3(-5, -5, -50));
	cubeRight->setColor(glm::vec3(1, 0, 0));
	cubeRight->setRefractivity(true, 0.5, 1.03);
	cubeRight->setReflectivity(true, 0.8);
	sceneObjects.push_back(cubeRight);
}

void generateMarble()
{
	marbleNoise = new float*[NOISE_HEIGHT];
	
	for (int i = 0; i < NOISE_HEIGHT; i++)
	{
		marbleNoise[i] = new float[NOISE_WIDTH];
	}

	generateNoise(marbleNoise, NOISE_WIDTH, NOISE_HEIGHT);

	float xPeriod = 5.0;
	float yPeriod = 10.0;
	float turbPower = 2.0;
	float turbSize = 128.0;
	
	for (int y = 0; y < NOISE_HEIGHT; y++)
	{
		for (int x = 0; x < NOISE_WIDTH; x++)
		{
			float xyValue = x * xPeriod / NOISE_WIDTH
				+ y * yPeriod / NOISE_HEIGHT
				+ turbPower * turbulence(marbleNoise, NOISE_WIDTH, NOISE_HEIGHT, x, y, turbSize) / 256.0;
    		float sineValue = fabs(sin(xyValue * PI));
			marbleColours[y][x] = glm::vec3(sineValue);
		}
	}
}

//---This function initializes the scene ------------------------------------------- 
//   Specifically, it creates scene objects (spheres, planes, cones, cylinders etc)
//     and add them to the list of scene objects.
//   It also initializes the OpenGL orthographc projection matrix for drawing the
//     the ray traced image.
//----------------------------------------------------------------------------------
void initialize()
{
    glMatrixMode(GL_PROJECTION);
    gluOrtho2D(XMIN, XMAX, YMIN, YMAX);

    glClearColor(0, 0, 0, 1);
	generateMarble();
	
	brickAlbedo = TextureBMP("textures/brick_albedo.bmp");
	brickNormal = TextureBMP("textures/brick_normal.bmp");
	bronzeAlbedo = TextureBMP("textures/bronze_albedo.bmp");
	bronzeNormal = TextureBMP("textures/bronze_normal.bmp");
	bronzeMetallic = TextureBMP("textures/bronze_metallic.bmp");

	Plane *plane = new Plane(glm::vec3(-200, -15, 0),
							 glm::vec3(200, -15, 0),
							 glm::vec3(200, -15,-400),
							 glm::vec3(-200, -15, -400));
	plane->setSpecularity(false);
	plane->setReflectivity(true, 0.25);
	sceneObjects.push_back(plane);
	
	Plane *brickWall = new Plane(glm::vec3(-200, -15, -150),
								 glm::vec3(200, -15, -150),
								 glm::vec3(200, 35, -150),
								 glm::vec3(-200, 35, -150));
	brickWall->setColor(glm::vec3(1, 0.8, 0));
	brickWall->setSpecularity(false);
	sceneObjects.push_back(brickWall);

	Sphere *sphere1 = new Sphere(glm::vec3(-5.0, 0.0, -90.0), 15.0);
	sphere1->setColor(glm::vec3(0, 0, 1));   //Set colour to blue
	sphere1->setReflectivity(true, 0.8);
	sceneObjects.push_back(sphere1);		 //Add sphere to scene objects

	Sphere *sphere2 = new Sphere(glm::vec3(5, -2, -70), 4.0);
	sphere2->setColor(glm::vec3(1, 0, 0));
	sphere2->setRefractivity(true, 0.65, 1.01);
	sphere2->setReflectivity(true, 0.5);
	sceneObjects.push_back(sphere2);

	Sphere *sphere3 = new Sphere(glm::vec3(10, 10, -60), 3.0);
	sphere3->setColor(glm::vec3(0, 0.5, 1));
	sceneObjects.push_back(sphere3);

	Cylinder *cylinder = new Cylinder(glm::vec3(-9, -15, -20), 2, 12);
	cylinder->setColor(colFromBytes(149, 116, 70));
	cylinder->setReflectivity(true, 0.2);
	sceneObjects.push_back(cylinder);

	Sphere *sphere4 = new Sphere(glm::vec3(15, -10, -40), 5.0);
	sphere4->setColor(glm::vec3(0, 1, 0));
	sphere4->setTransparency(true, 0.8);
	sphere4->setReflectivity(true, 0.5);
	sceneObjects.push_back(sphere4);

	drawCube();

	Torus *torus = new Torus(glm::vec3(2, -10, -40), 4, 2);
	torus->setColor(glm::vec3(0, 0.2, 0));
	// torus->setRefractivity(true, 0.5, 1.5);
	torus->setReflectivity(true, 0.4);
	sceneObjects.push_back(torus);
	
	// drawCrystal(1.0f, glm::vec3(-7.5, -15, -35), colFromBytes(255, 0, 255));
}

void exportTga()
{
	const int width = NUMDIV;
	const int height = NUMDIV;
	const int headerLen = 18;
	const short int bpp = 24;
	const int pixelBytes = width * height * (bpp / 8);
	
	// file format in little endian, assumes host machine is big endian

	static unsigned char bytes[headerLen + pixelBytes];

	bytes[2] = 2; // Image type (uncompressed true color)

	// Image specification
	bytes[8] = width & 0xFF;			// X-Origin
	bytes[9] = (width >> 8) & 0xFF;		// X-Origin
	bytes[10] = height & 0xFF;			// Y-Origin
	bytes[11] = (height >> 8) & 0xFF;	// Y-Origin
	bytes[12] = width & 0xFF;			// Width
	bytes[13] = (width >> 8) & 0xFF;	// Width
	bytes[14] = height & 0xFF;			// Height
	bytes[15] = (height >> 8) & 0xFF;	// Height
	bytes[16] = bpp;					// Pixel depth (bytes per pixel)
	bytes[17] = 0;						// Image descriptor
	int p = 18;
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			glm::vec3 pixel = pixels[x][y];
			bytes[p] = clamp(255 * pixel.b, 0, 255);
			p++;
			bytes[p] = clamp(255 * pixel.g, 0, 255);
			p++;
			bytes[p] = clamp(255 * pixel.r, 0, 255);
			p++;
		}
	}

	cout << "Filled pixel data" << endl;


	FILE *writePtr;

	writePtr = fopen("render_output.tga", "wb");
	cout << "Opened file for write" << endl;
	fwrite(bytes, sizeof(bytes), 1, writePtr);
	cout << "Wrote file" << endl;
	fclose(writePtr);
	cout << "Closed file stream" << endl;
}

void keyboard(unsigned char key, int x, int y)
{
	if (key == ' ' )
	{
		if (!exported)
		{
			exportTga();
			exported = true;
		}
	}
}

int main(int argc, char *argv[]) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB );
    glutInitWindowSize(NUMDIV, NUMDIV);
    glutInitWindowPosition(20, 20);
    glutCreateWindow("Raytracing");

    glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
    initialize();

    glutMainLoop();
    return 0;
}
