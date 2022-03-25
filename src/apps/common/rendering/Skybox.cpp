#include <Skybox.h>

using namespace Rendering;

Skybox::Skybox() : cube{Primitive::GenerateCube()}
{

}

#include <iostream>

void Skybox::Draw(GL::Shader& skyboxShader, const glm::mat4 view, const glm::mat4 proj, bool cullFaceEnabled)
{   
    if(cullFaceEnabled)
        glDisable(GL_CULL_FACE);

    glDepthFunc(GL_LEQUAL); 

    cubemap.Bind();
    skyboxShader.SetUniformInt("skybox", 0);
    auto modView = glm::mat4(glm::mat3(view));
    auto transform = proj * modView;
    skyboxShader.SetUniformMat4("transform", transform);
    cube.Draw(skyboxShader);

    glDepthFunc(GL_LESS);
    if(cullFaceEnabled)
        glEnable(GL_CULL_FACE);

    GLenum err;
    while((err = glGetError()) != GL_NO_ERROR)
    {
        std::cout << "Error: " << std::hex << err << '\n';
    }
}

void Skybox::Load(const GL::Cubemap::TextureMap& textureMap, bool flipVertically)
{
    cubemap.Load(textureMap, flipVertically);
}