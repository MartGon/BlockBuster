#include <Framebuffer.h>

#include <utility>

#include <iostream>

using namespace GL;

Framebuffer::~Framebuffer()
{
    //glDeleteTextures(1, &texture);
    glDeleteFramebuffers(1, &fbo);
    glDeleteRenderbuffers(1, &rbo);
}

Framebuffer::Framebuffer(Framebuffer&& other)
{
    *this = std::move(other);
}

Framebuffer& Framebuffer::operator=(Framebuffer&& other)
{
    fbo = other.fbo;
    other.fbo = 0;

    rbo = other.rbo;
    other.rbo = 0;

    texture = other.texture;
    other.texture = 0;

    size = other.size;
    loaded = other.loaded;


    return *this;
}

void Framebuffer::Init(glm::ivec2 winSize)
{
    if(loaded)
        return;

    // Set size
    this->size = winSize;

    // Framebuffer
    glGenFramebuffers(1, &fbo);
    Bind();

    // Color texture
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, winSize.x, winSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Attach color buffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0); 

    // Renderbuffer
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    
    // Depth buffer
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F, winSize.x, winSize.y);

    // Attach renderbuffer
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	    std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    Unbind();


    loaded = true;
}

void Framebuffer::Bind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
}

void Framebuffer::Unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::BindTexture(unsigned int activeTexture)
{
    glActiveTexture(activeTexture);
    glBindTexture(GL_TEXTURE_2D, texture);
}

