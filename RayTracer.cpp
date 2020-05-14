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

    ray.closestPt(sceneObjects);					 		//Compare the ray with all objects in the scene
    if(ray.index == -1) return backgroundCol;		 		//no intersection
	obj = sceneObjects[ray.index];					 		//object on which the closest point of intersection is found

	if (ray.index == 0)
	{
		int iz = (ray.hit.z < 0 ? -ray.hit.z + BOARD_WIDTH : ray.hit.z) / BOARD_WIDTH;
		int ix = (ray.hit.x < 0 ? -ray.hit.x + BOARD_WIDTH : ray.hit.x) / BOARD_WIDTH;
		int k = (iz % 2) ^ (ix % 2);
		color = (k == 0) ? BOARD_PRIMARY_COLOUR : BOARD_SECONDARY_COLOUR;
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
		color = obj->lighting(lightPos, -ray.dir, ray.hit) * color;
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
					(ray.index == 0 ? color.r : obj->getColor().r) + ((1 - hitObject->getTransparencyCoeff()) * 0.5 * hitCol.r)),
				hitObject->getTransparencyCoeff() * ((hitObject->getTransparencyCoeff()) *
					(ray.index == 0 ? color.g : obj->getColor().g) + ((1 - hitObject->getTransparencyCoeff()) * 0.5 * hitCol.g)),
				hitObject->getTransparencyCoeff() * ((hitObject->getTransparencyCoeff()) *
					(ray.index == 0 ? color.b : obj->getColor().b) + ((1 - hitObject->getReflectionCoeff()) * 0.5 * hitCol.b)));
		}
		else
		{
			color = glm::vec3(
				0.2 * (ray.index == 0 ? color.r : obj->getColor().r),
				0.2 * (ray.index == 0 ? color.g : obj->getColor().g),
				0.2 * (ray.index == 0 ? color.b : obj->getColor().b));
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
	// sphere1->setSpecularity(false);
	sphere1->setReflectivity(true, 0.8);
	sceneObjects.push_back(sphere1);		 //Add sphere to scene objects

	Sphere *sphere2 = new Sphere(glm::vec3(5, -2, -70), 4.0);
	sphere2->setColor(glm::vec3(1, 0, 0));
	sphere2->setRefractivity(true, 0.65, 1.01);
	sphere2->setReflectivity(true, 0.5);
	sceneObjects.push_back(sphere2);

	Sphere *sphere3 = new Sphere(glm::vec3(10, 10, -60), 3.0);
	sphere3->setColor(glm::vec3(0, 1, 1));
	sceneObjects.push_back(sphere3);

	Sphere *sphere4 = new Sphere(glm::vec3(15, -10, -40), 5.0);
	sphere4->setColor(glm::vec3(0, 1, 0));
	sphere4->setTransparency(true, 0.8);
	sphere4->setReflectivity(true, 0.5);
	sceneObjects.push_back(sphere4);

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

	Torus *torus = new Torus(glm::vec3(2, -10, -40), 4, 2);
	torus->setColor(glm::vec3(0, 0.2, 0));
	// torus->setRefractivity(true, 0.5, 1.5);
	torus->setReflectivity(true, 0.4);
	sceneObjects.push_back(torus);
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
