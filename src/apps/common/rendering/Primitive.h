#pragma once

#include <Mesh.h>

namespace Rendering
{
    namespace Primitive
    {
        Rendering::Mesh GenerateCube();
        Rendering::Mesh GenerateSlope();
        Rendering::Mesh GenerateCircle(float radius, unsigned int samples = 20);
        Rendering::Mesh GenerateSphere(float radius, unsigned int samples = 20);
    }
}