/*----------------------------------------------------------
* COSC363  Ray Tracer
*
*  The torus class
*  This is a subclass of Object, and hence implements the
*  methods intersect() and normal().
-------------------------------------------------------------*/

#ifndef H_TORUS
#define H_TORUS
#include <glm/glm.hpp>
#include "SceneObject.h"

/**
 * Defines a torus located at 'center'
 * with the specified major and minor radii
 */

class Torus : public SceneObject
{
private:
    glm::vec3 center = glm::vec3(0);
    float majorRadius = 1;
    float minorRadius = 0.5;

public:
    Torus() {};

    Torus(glm::vec3 c, float majR, float minR) : center(c), majorRadius(majR), minorRadius(minR) {}

	float intersect(glm::vec3 p0, glm::vec3 dir);

	glm::vec3 normal(glm::vec3 p);

};

#endif //H_TORUS