#ifndef DETSCAT_CAMERA_H
#define DETSCAT_CAMERA_H


#include "mymath.h"



typedef struct {
    Vec3 C;     // Camera center position in world coordinates
    Vec3 n;     // Optical axis (unit vector): from camera toward scene
    double u[3];     // Right vector (unit vector)
    double v[3];     // Up vector (unit vector)
    double f;        // Focal length (meters)
    double p_x;      // Pixel size in x (meters)
    double p_y;      // Pixel size in y (meters)
    int width;       // Number of pixels horizontally
    int height;      // Number of pixels vertically
    double c_x;      // Principal point x (usually width / 2)
    double c_y;      // Principal point y (usually height / 2)
} Camera;



#endif  // DETSCAT_CAMERA_H
