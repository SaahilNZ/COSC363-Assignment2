#include "Cylinder.h"
#include <math.h>
#include <iostream>

using namespace std;

float Cylinder::intersect(glm::vec3 p0, glm::vec3 dir)
{
    glm::vec3 vdif = p0 - center;

    float a; // dx^2 + dz^2
    float b; // 2(dx(x0 - xc) + dz(z0 - zc))
    float c; // (x0 - xc)^2 + (z0 - zc)^2 - R^2

    a = (dir.x * dir.x) + (dir.z * dir.z);

    b = 2 * ((dir.x * vdif.x) + (dir.z * vdif.z));

    c = (vdif.x * vdif.x)
        + (vdif.z * vdif.z)
        - (radius * radius);
    
    float delta = (b * b) - (4 * a * c);
    if (fabs(delta) < 0.001) return -1.0;
    if (delta < 0.0) return -1.0;

    float t1 = (-b - sqrt(delta)) / (2 * a);
    float t2 = (-b + sqrt(delta)) / (2 * a);


    if(fabs(t1) < 0.001 )
    {
        if (t2 > 0) return t2;
        else t1 = -1.0;
    }
    if(fabs(t2) < 0.001 ) t2 = -1.0;

    if (p0.y + t1 * dir.y > center.y + height
        && p0.y + t2 * dir.y < center.y + height)
    {
        return ((center.y + height) - p0.y) / dir.y;
    }

    if (p0.y + t1 * dir.y > center.y + height
        || p0.y + t1 * dir.y < center.y) t1 = -1.0f;

    if (p0.y + t2 * dir.y > center.y + height
        || p0.y + t2 * dir.y < center.y) t2 = -1.0f;

    return (t1 < t2) ? (t1 >= 0 ? t1 : t2) : (t2 >= 0 ? t2 : t1);
}

glm::vec3 Cylinder::normal(glm::vec3 p)
{
    glm::vec3 shifted = p - center;
    glm::vec3 ignoreY = glm::vec3(shifted.x / radius, 0, shifted.z / radius);

    if (shifted.y >= height)
    {
        return glm::vec3(0, 1, 0);
    }

    return ignoreY;
}