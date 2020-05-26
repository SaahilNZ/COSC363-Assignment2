/*----------------------------------------------------------
* COSC363  Ray Tracer
*
*  The torus class
*  This is a subclass of Object, and hence implements the
*  methods intersect() and normal().
-------------------------------------------------------------*/

#include "Torus.h"
#include <math.h>
#include "Roots3And4.h"

/**
* Torus' intersection method.  The input is a ray. 
*/
float Torus::intersect(glm::vec3 p0, glm::vec3 dir)
{
    // Based off of findIntersection(ray) in the following link
    // https://github.com/marcin-chwedczuk/ray_tracing_torus_js/blob/master/app/scripts/Torus.js
    glm::vec3 vdif = p0 - center;
    double ox = vdif.x;
    double oy = vdif.y;
    double oz = vdif.z;

    double dx = dir.x;
    double dy = dir.y;
    double dz = dir.z;

    double sumDirSqrd = dx*dx + dy*dy + dz*dz;
    double e = ox*ox + oy*oy + oz*oz - majorRadius*majorRadius - minorRadius*minorRadius;
    double f = ox*dx + oy*dy + oz*dz;
    double fourASqrd = 4.0 * majorRadius * majorRadius;

    double coeffs[5] =
    {
        e * e - fourASqrd * (minorRadius*minorRadius - oy*oy),
        4.0 * f * e + 2.0 * fourASqrd * oy * dy,
        2.0 * sumDirSqrd * e + 4.0 * f * f + fourASqrd * dy * dy,
        4.0 * sumDirSqrd * f,
        sumDirSqrd * sumDirSqrd
    };

    double solutions[4];
    int numSolutions = SolveQuartic(coeffs, solutions);
    float min = INFINITY;
    if (numSolutions == 0) return -1.0;
    for (int i = 0; i < 4; i++)
    {
        double t = solutions[i];
        if (t > 0.001 && t < min)
        {
            min = t;
        }
    }

    return min < INFINITY ? min : -1;
}

/**
* Returns the unit normal vector at a given point.
* Assumption: The input point p lies on the torus.
*/
glm::vec3 Torus::normal(glm::vec3 p)
{
    // Based off of computeNormalAtPoint(point) in the following link
    // https://github.com/marcin-chwedczuk/ray_tracing_torus_js/blob/master/app/scripts/Torus.js
    float x = p.x - center.x;
    float y = p.y - center.y;
    float z = p.z - center.z;

    float paramSquared = majorRadius * majorRadius + minorRadius * minorRadius;
    float sumSquared = x*x + y*y + z*z;

    glm::vec3 n(4.0 * x * (sumSquared - paramSquared),
                4.0 * y * (sumSquared - paramSquared + 2.0 * majorRadius * majorRadius),
                4.0 * z * (sumSquared - paramSquared));
    n = glm::normalize(n);
    return n;
}
