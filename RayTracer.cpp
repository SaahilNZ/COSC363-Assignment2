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
#include "Sphere.h"
#include "SceneObject.h"
#include "Ray.h"
#include "Plane.h"
#include <GL/freeglut.h>
#include "TextureBMP.h"
#include "Torus.h"
#include <thread>
using namespace std;

#define colFromBytes(r, g, b) glm::vec3(r / 255.0f, g / 255.0f, b / 255.0f)

#define NUM_THREADS 16
#define BOARD_WIDTH 5
#define BOARD_PRIMARY_COLOUR glm::vec3(0.8, 0.8, 0.8)
#define BOARD_SECONDARY_COLOUR glm::vec3(0.25, 0.25, 0.25)
#define PI acos(-1)

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
TextureBMP texture;
bool traced = false;
glm::vec3 pixels[NUMDIV][NUMDIV];

float angleBetweenVectors(glm::vec3 a, glm::vec3 b, glm::vec3 origin)
{
	glm::vec3 an = glm::normalize(a - origin);
	glm::vec3 bn = glm::normalize(b - origin);
	return glm::acos(glm::dot(an, bn));
}

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

    ray.closestPt(sceneObjects);					 		//Compare the ray with all objects in the scene
    if(ray.index == -1) return backgroundCol;		 		//no intersection
	obj = sceneObjects[ray.index];					 		//object on which the closest point of intersection is found
	glm::vec3 baseColor = obj->getColor();

	bool differentColour = false;

	if (ray.index == 0)
	{
		int iz = (ray.hit.z < 0 ? -ray.hit.z + BOARD_WIDTH : ray.hit.z) / BOARD_WIDTH;
		int ix = (ray.hit.x < 0 ? -ray.hit.x + BOARD_WIDTH : ray.hit.x) / BOARD_WIDTH;
		int k = (iz % 2) ^ (ix % 2);
		baseColor = (k == 0) ? BOARD_PRIMARY_COLOUR : BOARD_SECONDARY_COLOUR;
		// obj->setColor(color);

		float x1 = -15.0;
		float x2 = 5.0;
		float z1 = -60.0;
		float z2 = -90.0;

		float texcoords = (ray.hit.x - x1) / (x2 - x1);
		float texcoordt = (ray.hit.z - z1) / (z2 - z1);
		if (texcoords >= 0 && texcoords <= 1 &&
			texcoordt >= 0 && texcoordt <= 1)
		{
			// color = texture.getColorAt(texcoords, texcoordt);
			// obj->setColor(color);
		}

		differentColour = true;
	}
	else if (ray.index == 3)
	{
		glm::vec3 origin = glm::vec3(10, 10, -60);
		glm::vec3 localHit = glm::normalize(ray.hit - origin);

		float u = 0.5 + atan2(localHit.x, localHit.z) / (2 * PI); 
		float v = 0.5 - asin(localHit.y) / PI;
		
		float mult = (sin((u + v) * 100) / 2.0) + 1;
		baseColor = glm::vec3(mult, mult, mult);

		differentColour = true;
	}

	if (differentColour)
	{
		color = obj->lighting(lightPos, -ray.dir, ray.hit, baseColor);
	}
	else
	{
		color = obj->lighting(lightPos, -ray.dir, ray.hit);						//Object's colour
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
		glm::vec3 normalVec = obj->normal(ray.hit);
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
		glm::vec3 m = obj->normal(refrRay.hit);
		glm::vec3 h = glm::refract(g, -m, 1.0f / eta);

		Ray finalRay(refrRay.hit, h);
		glm::vec3 refractedColour = trace(finalRay, step + 1);
		color = (color * (1 - coeff)) + (coeff * refractedColour);
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

void drawCrystal(float scale, glm::vec3 location, glm::vec3 colour)
{
	float reflectionCoeff = 0.6;
	float refractionCoeff = 0.3;
	float refractiveIndex = 1.05;

	// bottom planes
	Plane *b1 = new Plane(
		location,
		location + glm::vec3(0.5 * scale, scale, 0.5 * scale),
		location + glm::vec3(-0.5 * scale, scale, 0.5 * scale)
	);
	b1->setColor(colour);
	b1->setReflectivity(true, reflectionCoeff);
	b1->setRefractivity(true, refractionCoeff, refractiveIndex);
	sceneObjects.push_back(b1);

	Plane *b2 = new Plane(
		location,
		location + glm::vec3(-0.5 * scale, scale, -0.5 * scale),
		location + glm::vec3(0.5 * scale, scale, -0.5 * scale)
	);
	b2->setColor(colour);
	b2->setReflectivity(true, reflectionCoeff);
	b2->setRefractivity(true, refractionCoeff, refractiveIndex);
	sceneObjects.push_back(b2);
	
	Plane *b3 = new Plane(
		location,
		location + glm::vec3(-0.5 * scale, scale, 0.5 * scale),
		location + glm::vec3(-0.5 * scale, scale, -0.5 * scale)
	);
	b3->setColor(colour);
	b3->setReflectivity(true, reflectionCoeff);
	b3->setRefractivity(true, refractionCoeff, refractiveIndex);
	sceneObjects.push_back(b3);

	Plane *b4 = new Plane(
		location,
		location + glm::vec3(0.5 * scale, scale, -0.5 * scale),
		location + glm::vec3(0.5 * scale, scale, 0.5 * scale)
	);
	b4->setColor(colour);
	b4->setReflectivity(true, reflectionCoeff);
	b4->setRefractivity(true, refractionCoeff, refractiveIndex);
	sceneObjects.push_back(b4);

	// lower planes
	Plane *l1 = new Plane(
		location + glm::vec3(-0.5 * scale, scale, 0.5 * scale),
		location + glm::vec3(0.5 * scale, scale, 0.5 * scale),
		location + glm::vec3(0, 7 * scale, scale)
	);
	l1->setColor(colour);
	l1->setReflectivity(true, reflectionCoeff);
	l1->setRefractivity(true, refractionCoeff, refractiveIndex);
	sceneObjects.push_back(l1);

	Plane *l2 = new Plane(
		location + glm::vec3(0.5 * scale, scale, -0.5 * scale),
		location + glm::vec3(-0.5 * scale, scale, -0.5 * scale),
		location + glm::vec3(0, 7 * scale,  -scale)
	);
	l2->setColor(colour);
	l2->setReflectivity(true, reflectionCoeff);
	l2->setRefractivity(true, refractionCoeff, refractiveIndex);
	sceneObjects.push_back(l2);

	Plane *l3 = new Plane(
		location + glm::vec3(-0.5 * scale, scale, -0.5 * scale),
		location + glm::vec3(-0.5 * scale, scale, 0.5 * scale),
		location + glm::vec3(-scale, 7 * scale, 0)
	);
	l3->setColor(colour);
	l3->setReflectivity(true, reflectionCoeff);
	l3->setRefractivity(true, refractionCoeff, refractiveIndex);
	sceneObjects.push_back(l3);

	Plane *l4 = new Plane(
		location + glm::vec3(0.5 * scale, scale, 0.5 * scale),
		location + glm::vec3(0.5 * scale, scale, -0.5 * scale),
		location + glm::vec3(scale, 7 * scale,  0)
	);
	l4->setColor(colour);
	l4->setReflectivity(true, reflectionCoeff);
	l4->setRefractivity(true, refractionCoeff, refractiveIndex);
	sceneObjects.push_back(l4);

	// upper planes
	Plane *u1 = new Plane(
		location + glm::vec3(-0.5 * scale, scale, 0.5 * scale),
		location + glm::vec3(0, 7 * scale, scale),
		location + glm::vec3(-0.5 * scale, 8.5 * scale, 0.5 * scale),
		location + glm::vec3(-scale, 7 * scale, 0)
	);
	u1->setColor(colour);
	u1->setReflectivity(true, reflectionCoeff);
	u1->setRefractivity(true, refractionCoeff, refractiveIndex);
	sceneObjects.push_back(u1);
	
	Plane *u2 = new Plane(
		location + glm::vec3(0.5 * scale, scale, 0.5 * scale),
		location + glm::vec3(scale, 7 * scale, 0),
		location + glm::vec3(0.5 * scale, 8.5 * scale, 0.5 * scale),
		location + glm::vec3(0, 7 * scale, scale)
	);
	u2->setColor(colour);
	u2->setReflectivity(true, reflectionCoeff);
	u2->setRefractivity(true, refractionCoeff, refractiveIndex);
	sceneObjects.push_back(u2);
	
	Plane *u3 = new Plane(
		location + glm::vec3(-0.5 * scale, scale, -0.5 * scale),
		location + glm::vec3(-scale, 7 * scale, 0),
		location + glm::vec3(-0.5 * scale, 8.5 * scale, -0.5 * scale),
		location + glm::vec3(0, 7 * scale, -scale)
	);
	u3->setColor(colour);
	u3->setReflectivity(true, reflectionCoeff);
	u3->setRefractivity(true, refractionCoeff, refractiveIndex);
	sceneObjects.push_back(u3);
	
	Plane *u4 = new Plane(
		location + glm::vec3(0.5 * scale, scale, 0.5 * scale),
		location + glm::vec3(0, 7 * scale, -scale),
		location + glm::vec3(0.5 * scale, 8.5 * scale, -0.5 * scale),
		location + glm::vec3(scale, 7 * scale, 0)
	);
	u4->setColor(colour);
	u4->setReflectivity(true, reflectionCoeff);
	u4->setRefractivity(true, refractionCoeff, refractiveIndex);
	sceneObjects.push_back(u4);

	// top planes
	Plane *t1 = new Plane(
		location + glm::vec3(0, 7 * scale, scale),
		location + glm::vec3(0.5 * scale, 8.5 * scale, 0.5 * scale),
		location + glm::vec3(0, 9.5 * scale, 0),
		location + glm::vec3(-0.5 * scale, 8.5 * scale, 0.5 * scale)
	);
	t1->setColor(colour);
	t1->setReflectivity(true, reflectionCoeff);
	t1->setRefractivity(true, refractionCoeff, refractiveIndex);
	sceneObjects.push_back(t1);
	
	Plane *t2 = new Plane(
		location + glm::vec3(0, 7 * scale, -scale),
		location + glm::vec3(-0.5 * scale, 8.5 * scale, -0.5 * scale),
		location + glm::vec3(0, 9.5 * scale, 0),
		location + glm::vec3(0.5 * scale, 8.5 * scale, -0.5 * scale)
	);
	t2->setColor(colour);
	t2->setReflectivity(true, reflectionCoeff);
	t2->setRefractivity(true, refractionCoeff, refractiveIndex);
	sceneObjects.push_back(t2);

	Plane *t3 = new Plane(
		location + glm::vec3(-scale, 7 * scale, 0),
		location + glm::vec3(-0.5 * scale, 8.5 * scale, 0.5 * scale),
		location + glm::vec3(0, 9.5 * scale, 0),
		location + glm::vec3(-0.5 * scale, 8.5 * scale, -0.5 * scale)
	);
	t3->setColor(colour);
	t3->setReflectivity(true, reflectionCoeff);
	t3->setRefractivity(true, refractionCoeff, refractiveIndex);
	sceneObjects.push_back(t3);
	
	Plane *t4 = new Plane(
		location + glm::vec3(scale, 7 * scale, 0),
		location + glm::vec3(0.5 * scale, 8.5 * scale, -0.5 * scale),
		location + glm::vec3(0, 9.5 * scale, 0),
		location + glm::vec3(0.5 * scale, 8.5 * scale, 0.5 * scale)
	);
	t4->setColor(colour);
	t4->setReflectivity(true, reflectionCoeff);
	t4->setRefractivity(true, refractionCoeff, refractiveIndex);
	sceneObjects.push_back(t4);
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

	// texture = TextureBMP("Butterfly.bmp");

	Plane *plane = new Plane(glm::vec3(-200, -15, 0),
							 glm::vec3(200, -15, 0),
							 glm::vec3(200, -15,-400),
							 glm::vec3(-200, -15, -400));
	plane->setSpecularity(false);
	plane->setReflectivity(true, 0.25);
	sceneObjects.push_back(plane);

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
	sphere3->setColor(glm::vec3(1, 1, 1));
	sceneObjects.push_back(sphere3);

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


int main(int argc, char *argv[]) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB );
    glutInitWindowSize(NUMDIV, NUMDIV);
    glutInitWindowPosition(20, 20);
    glutCreateWindow("Raytracing");

    glutDisplayFunc(display);
    initialize();

    glutMainLoop();
    return 0;
}
