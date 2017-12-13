#ifndef STRUCTS_HPP
#define STRUCTS_HPP

#include <map>
#include <glbinding/gl/gl.h>
// use gl definitions from glbinding 
using namespace gl;

// gpu representation of model
struct model_object {
  // vertex array object
  GLuint vertex_AO = 0;
  // vertex buffer object
  GLuint vertex_BO = 0;
  // index buffer object
  GLuint element_BO = 0;
  // primitive type to draw
  GLenum draw_mode = GL_NONE;
  // indices number, if EBO exists
  GLsizei num_elements = 0;
};

struct renderbuffer_object {
  // handle of renderbuffer obj
  GLuint handle = 0;
  // binding point
  GLenum target = GL_NONE;
};

struct framebuffer_object {
  // handle of framebuffer obj
  GLuint handle = 0;
  // binding point
  GLenum target = GL_NONE;
};

// gpu representation of texture
struct texture_object {
  // handle of texture object
  GLuint handle = 0;
  // binding point
  GLenum target = GL_NONE;
};

struct colorRGB {
  float red;
  float green;
  float blue;
};

/* planet struct
    - name identifies planets
    - orbited planet has to match existing planet
    
.-.,="``"=.    
'=/_       \ 
 |  '=._    |
  \     `=./`,  
   '=.__.=' `=' 
*/
struct planet {
  std::string name;
  float size;
  float rotation_speed;
  float distance_to_origin;
  colorRGB color;
  int texture;
  bool mapped;
  texture_object tex_obj;
  texture_object nor_obj;
  renderbuffer_object rb_obj;
  framebuffer_object fb_obj;
};
// moon struct (should inherit from planet)
struct moon {
  std::string name;
  float size;
  float rotation_speed;
  float distance_to_origin;
  std::string orbiting;
  colorRGB color;
  int texture;
  bool mapped;
  texture_object tex_obj;
  texture_object nor_obj;
  renderbuffer_object rb_obj;
  framebuffer_object fb_obj;
};

struct star{
  float x;
  float y;
  float z;
};

// shader handle and uniform storage
struct shader_program {
  shader_program(std::string const& vertex, std::string const& fragment)
   :vertex_path{vertex}
   ,fragment_path{fragment}
   ,handle{0}
   {}

  // path to shader source
  std::string vertex_path; 
  std::string fragment_path; 
  // object handle
  GLuint handle;
  // uniform locations mapped to name
  std::map<std::string, GLint> u_locs{};
};
#endif