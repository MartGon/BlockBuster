
#include <string>

namespace App
{
    struct Configuration
    {
        struct  WindowConfig
        {
            std::string name;
            int width;
            int height;
            int xPos;
            int yPos;
        };

        struct OpenGLConfig
        {
            int majorVersion;
            int minorVersion;
            int profileMask;
        };

        WindowConfig window;
        OpenGLConfig openGL;
    };
}