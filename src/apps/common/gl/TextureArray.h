
// Texture Array Implementation
// Notes:
// - ImGui has to be modified in order to use TextureArrays. Check Bookmarks, but basically a struct has to be passed to Image(). That
//   struct should hold the texture id (GLuint) and the layer. Maybe a GLuint id is generated each time  glTexSubImage3D is called. Needs testing. It may be global
//   or equal to layer level.
// - TextureArrays need to load each texture individually from data. So a (Load/Add)Texture has to be created in it
// 