#pragma once

#include <Mesh.h>

namespace Rendering
{
    namespace Primitive
    {   
        Rendering::Mesh GenerateQuad();
        Rendering::Mesh GenerateCube();
        Rendering::Mesh GenerateSlope();
        Rendering::Mesh GenerateCircle(float radius, unsigned int samples = 20);
        Rendering::Mesh GenerateSphere(float radius, unsigned int pSamples = 20, unsigned int ySamples = 20);
        Rendering::Mesh GenerateCylinder(float radius, float height, unsigned int yawSamples = 20, unsigned int hSamples = 1);
    }
}