#include "application_solar.hpp"
#include "launcher.hpp"

#include "utils.hpp"
#include "shader_loader.hpp"
#include "model_loader.hpp"
#include "texture_loader.hpp"

#include <glbinding/gl/gl.h>
// use gl definitions from glbinding 
using namespace gl;

// dont load gl bindings from glfw
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <math.h>
#include <random>
#include <vector>

// amount of distributed stats
int static const starAmount = 1000;
planet skysphere {"skysphere", 300.0f, 0.0f, 0.0f, {1.0f,1.0f,0.8f}, 11, false};
// rotation matrix for skysphere
glm::fmat4 rotation {};

// modes for quad shader
bool greyscale = false;
bool mirrorH = false;
bool mirrorV = false;
bool gaussian = false;

/*----------------------------------------------------------------------------*/
///////////////////////////////// Constructor //////////////////////////////////
/*----------------------------------------------------------------------------*/

ApplicationSolar::ApplicationSolar(std::string const& resource_path)
 :Application{resource_path}
 ,planet_object{}
 ,star_object{}
 ,orbit_object{}
 ,quad_object{}
{ 
  initializeBigBang();
  distributeStars(starAmount);
  initializeOrbits();
  initializeQuad();
  initializeFrameBuffer();
  initializeTextures();
  initializeGeometry();
  initializeShaderPrograms();
}


/*----------------------------------------------------------------------------*/
///////////////////////////////// Rendering ////////////////////////////////////
/*----------------------------------------------------------------------------*/

void ApplicationSolar::render() const {
  glBindFramebuffer(GL_FRAMEBUFFER, fb_object.handle);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
  // do the sky first of all so the depth mask won't mess everything up
  // really messy, really
  glDepthMask(GL_FALSE); // Sphere is always in the back
  	// take the rotation of the camera as ModelMatrix, so you are in an actual sphere
  glUseProgram(m_shaders.at("skysphere").handle);
  glUniformMatrix4fv(m_shaders.at("skysphere").u_locs.at("ModelMatrix"),
                     1, GL_FALSE, glm::value_ptr(rotation));
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, skysphere.tex_obj.handle);
  int color_sampler_location = glGetUniformLocation(m_shaders.at("skysphere").handle, "ColorTex");
  glUseProgram(m_shaders.at("skysphere").handle);
  glUniform1i(color_sampler_location, 0);
  glBindVertexArray(planet_object.vertex_AO);

  glDrawElements(planet_object.draw_mode, planet_object.num_elements, model::INDEX.type, NULL);
  glDepthMask(1); 

  glBindVertexArray(star_object.vertex_AO);
  glUseProgram(m_shaders.at("stars").handle);
  glDrawArrays(star_object.draw_mode, 0, star_object.num_elements);

  // upload transforms of every planet in the solar system
  for (auto const& planet : solar_system) {
    // calculate the orbit of every planet
    getOrbit(planet);
    // and upload it
    glBindVertexArray(orbit_object.vertex_AO);
    glUseProgram(m_shaders.at("orbit").handle);
    glDrawArrays(orbit_object.draw_mode, 0, orbit_object.num_elements);
    // upload the planet itself
    uploadPlanetTransforms(planet);
    // bind the VAO to draw
    glBindVertexArray(planet_object.vertex_AO);
    // draw bound vertex array using bound shader
    glDrawElements(planet_object.draw_mode, 
                   planet_object.num_elements, 
                   model::INDEX.type, NULL);
  }

  // iterate over every moon seperately
  for (auto const& moon : moon_system) {
    // calculate the orbit of every moon
    getOrbit(moon);
    // and upload it too
    glBindVertexArray(orbit_object.vertex_AO);
    glUseProgram(m_shaders.at("orbit").handle);
    glDrawArrays(orbit_object.draw_mode, 0, orbit_object.num_elements);
    // upload the moon itself too
    uploadMoonTransforms(moon);
    // bind the VAO to draw
    glBindVertexArray(planet_object.vertex_AO);
    // draw bound vertex array using bound shader
    glDrawElements(planet_object.draw_mode, 
                   planet_object.num_elements, 
                   model::INDEX.type, NULL);
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  glUseProgram(m_shaders.at("quad").handle);
  //bind texture to shader
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, tex_object.handle);

  color_sampler_location = glGetUniformLocation(m_shaders.at("quad").handle, "ColorTex");
  glUniform1i(color_sampler_location, 0);

  glBindVertexArray(quad_object.vertex_AO);

  glDrawArrays(quad_object.draw_mode, NULL, quad_object.num_elements);
  //glBindVertexArray(0);
  
}

/*----------------------------------------------------------------------------*/
////////////////////////////// Transform upload ////////////////////////////////
/*----------------------------------------------------------------------------*/

/**
 * Uploads the transformation matrix to shader to create a planet
 * @param p a planet object
 */
void ApplicationSolar::uploadPlanetTransforms(planet const& p) const {
  if (p.name == "sun"){
    // transform planet (where orbit planet is sun)
    glm::fmat4 model_matrix;
    model_matrix = glm::rotate(model_matrix, 
                 float(glfwGetTime()* p.rotation_speed), 
                 glm::fvec3{0.0f, 1.0f, 0.0f});
    model_matrix = glm::translate(model_matrix, 
                 glm::fvec3 {0.0f, 0.0f, -1.0f*p.distance_to_origin});
    model_matrix = glm::scale(model_matrix, 
                 glm::fvec3 {p.size, p.size, p.size});
    glUseProgram(m_shaders.at("sun").handle);

    glUniform3f(m_shaders.at("sun").u_locs.at("ColorVector"),
       
                 p.color.red, p.color.green, p.color.blue);
    glUniformMatrix4fv(m_shaders.at("sun").u_locs.at("ModelMatrix"),
                        1, GL_FALSE, glm::value_ptr(model_matrix));
  } else if (p.mapped){
    // transform planet (where orbit planet is sun)
    glm::fmat4 model_matrix;
    model_matrix = glm::rotate(model_matrix, 
                 float(glfwGetTime()* p.rotation_speed), 
                 glm::fvec3{0.0f, 1.0f, 0.0f});
    model_matrix = glm::translate(model_matrix, 
                 glm::fvec3 {0.0f, 0.0f, -1.0f*p.distance_to_origin});
    model_matrix = glm::scale(model_matrix, 
                 glm::fvec3 {p.size, p.size, p.size});
    glUseProgram(m_shaders.at(activeShader + "_normal").handle);

    glUniform3f(m_shaders.at(activeShader + "_normal").u_locs.at("ColorVector"),
       
                 p.color.red, p.color.green, p.color.blue);
    glUniformMatrix4fv(m_shaders.at(activeShader + "_normal").u_locs.at("ModelMatrix"),
                        1, GL_FALSE, glm::value_ptr(model_matrix));
  } else {
    glm::fmat4 model_matrix;
    model_matrix = glm::rotate(model_matrix, 
                 float(glfwGetTime()* p.rotation_speed), 
                 glm::fvec3{0.0f, 1.0f, 0.0f});
    model_matrix = glm::translate(model_matrix, 
                 glm::fvec3 {0.0f, 0.0f, -1.0f*p.distance_to_origin});
    model_matrix = glm::scale(model_matrix, 
                 glm::fvec3 {p.size, p.size, p.size});
    // extra matrix for normal transformation to keep them orthogonal to surface
    glUseProgram(m_shaders.at(activeShader).handle);
    glUniform3f(m_shaders.at(activeShader).u_locs.at("ColorVector"), p.color.red, p.color.green, p.color.blue);

    glm::fmat4 normal_matrix = 
          glm::inverseTranspose(glm::inverse(m_view_transform) * model_matrix);
    glUniformMatrix4fv(m_shaders.at(activeShader).u_locs.at("NormalMatrix"),
                     1, GL_FALSE, glm::value_ptr(normal_matrix));
    glUniformMatrix4fv(m_shaders.at(activeShader).u_locs.at("ModelMatrix"),
                     1, GL_FALSE, glm::value_ptr(model_matrix));
  }
  uploadTextures(p);
  
}

/**
 * Uploads the transformation matrix to shader to create a moon
 * @param p a moon object
 */
void ApplicationSolar::uploadMoonTransforms(moon m) const {
  planet origin;
  // iterate over solar system to find orbited planet
  for (auto const& p : solar_system) {
    if (m.orbiting == p.name) {
      origin = p;
      break;
    }
  }
  glm::fmat4 model_matrix;
  // rotate and translate model matrix just like the orbited planet
  model_matrix = glm::rotate(model_matrix, 
                             float(glfwGetTime()* origin.rotation_speed), 
                             {0.0f, 1.0f, 0.0f});
  model_matrix = glm::translate(model_matrix, 
                             {0.0f, 0.0f, -1.0f*origin.distance_to_origin});
  // same procedure with the moon parameters + scaling
  model_matrix = glm::rotate(model_matrix, 
                             float(glfwGetTime()* m.rotation_speed), 
                             {0.0f, 1.0f, 0.0f});
  model_matrix = glm::translate(model_matrix, 
                             {0.0f, 0.0f, -1.0f*m.distance_to_origin});
  model_matrix = glm::scale(model_matrix,
                             {m.size, m.size, m.size});

  glUseProgram(m_shaders.at(activeShader).handle);
  glUniformMatrix4fv(m_shaders.at(activeShader).u_locs.at("ModelMatrix"),
                     1, GL_FALSE, glm::value_ptr(model_matrix));

  // extra matrix for normal transformation to keep them orthogonal to surface
  glUniform3f(m_shaders.at(activeShader).u_locs.at("ColorVector"), m.color.red, m.color.green, m.color.blue);
  glm::fmat4 normal_matrix = glm::inverseTranspose(
                             glm::inverse(m_view_transform) * model_matrix);
  glUniformMatrix4fv(m_shaders.at(activeShader).u_locs.at("NormalMatrix"),
                     1, GL_FALSE, glm::value_ptr(normal_matrix));

  uploadTextures(m);

}

/*----------------------------------------------------------------------------*/
////////////////////////////// Matrix updates //////////////////////////////////
/*----------------------------------------------------------------------------*/

/**
 * update uniform locations
 */ 
void ApplicationSolar::uploadUniforms() {
  updateUniformLocations();
  
  updateView();
  updateProjection();
}

/**
 * update view matrix of every shader

 */ 
void ApplicationSolar::updateView() {
  // vertices are transformed in camera space, so camera transform must be inverted
  glm::fmat4 view_matrix = glm::inverse(m_view_transform);
  // upload matrix to gpu
  glUseProgram(m_shaders.at("planet").handle);
  glUniformMatrix4fv(m_shaders.at("planet").u_locs.at("ViewMatrix"),
                     1, GL_FALSE, glm::value_ptr(view_matrix));

  glUseProgram(m_shaders.at("planet_normal").handle);
  glUniformMatrix4fv(m_shaders.at("planet_normal").u_locs.at("ViewMatrix"),
                     1, GL_FALSE, glm::value_ptr(view_matrix));

  glUseProgram(m_shaders.at("planet_cel_normal").handle);
  glUniformMatrix4fv(m_shaders.at("planet_cel_normal").u_locs.at("ViewMatrix"),
                     1, GL_FALSE, glm::value_ptr(view_matrix));

  glUseProgram(m_shaders.at("planet_cel").handle);
  glUniformMatrix4fv(m_shaders.at("planet_cel").u_locs.at("ViewMatrix"),
                     1, GL_FALSE, glm::value_ptr(view_matrix));

  glUseProgram(m_shaders.at("sun").handle);
  glUniformMatrix4fv(m_shaders.at("sun").u_locs.at("ViewMatrix"),
                      1, GL_FALSE, glm::value_ptr(view_matrix));

  glUseProgram(m_shaders.at("skysphere").handle);
  glUniformMatrix4fv(m_shaders.at("skysphere").u_locs.at("ViewMatrix"),
                      1, GL_FALSE, glm::value_ptr(view_matrix));

  glUseProgram(m_shaders.at("stars").handle);
  glUniformMatrix4fv(m_shaders.at("stars").u_locs.at("ViewMatrix"),
                     1, GL_FALSE, glm::value_ptr(view_matrix));

  glUseProgram(m_shaders.at("orbit").handle);
  glUniformMatrix4fv(m_shaders.at("orbit").u_locs.at("ViewMatrix"),
                     1, GL_FALSE, glm::value_ptr(view_matrix));
}

/**
 * update projection matrix of every shader
 */ 
void ApplicationSolar::updateProjection() {
  glBindRenderbuffer(GL_RENDERBUFFER, rb_object.handle);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, GLsizei(1200u), GLsizei(600u));

  glBindTexture(GL_TEXTURE_2D, tex_object.handle);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, GLsizei(1200u), GLsizei(600u), 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
  // upload matrix to gpu
  glUseProgram(m_shaders.at("planet").handle);
  glUniformMatrix4fv(m_shaders.at("planet").u_locs.at("ProjectionMatrix"),
                     1, GL_FALSE, glm::value_ptr(m_view_projection));

  glUseProgram(m_shaders.at("planet_normal").handle);
  glUniformMatrix4fv(m_shaders.at("planet_normal").u_locs.at("ProjectionMatrix"),
                     1, GL_FALSE, glm::value_ptr(m_view_projection));

  glUseProgram(m_shaders.at("planet_cel_normal").handle);
  glUniformMatrix4fv(m_shaders.at("planet_cel_normal").u_locs.at("ProjectionMatrix"),
                     1, GL_FALSE, glm::value_ptr(m_view_projection));

  glUseProgram(m_shaders.at("planet_cel").handle);
  glUniformMatrix4fv(m_shaders.at("planet_cel").u_locs.at("ProjectionMatrix"),
                     1, GL_FALSE, glm::value_ptr(m_view_projection));

  glUseProgram(m_shaders.at("sun").handle);
  glUniformMatrix4fv(m_shaders.at("sun").u_locs.at("ProjectionMatrix"),
                      1, GL_FALSE, glm::value_ptr(m_view_projection));

  glUseProgram(m_shaders.at("skysphere").handle);
  glUniformMatrix4fv(m_shaders.at("skysphere").u_locs.at("ProjectionMatrix"),
                      1, GL_FALSE, glm::value_ptr(m_view_projection));

  glUseProgram(m_shaders.at("stars").handle);
  glUniformMatrix4fv(m_shaders.at("stars").u_locs.at("ProjectionMatrix"),
                     1, GL_FALSE, glm::value_ptr(m_view_projection));

  glUseProgram(m_shaders.at("orbit").handle);
  glUniformMatrix4fv(m_shaders.at("orbit").u_locs.at("ProjectionMatrix"),
                     1, GL_FALSE, glm::value_ptr(m_view_projection));
}

/*----------------------------------------------------------------------------*/
//////////////////////////////// Navigation ////////////////////////////////////
/*----------------------------------------------------------------------------*/

/**
 * handles key callback
 * WASD-movement
 * @param key an integer as the key value
 * @param action an integer for the corresponding key action
 */ 
void ApplicationSolar::keyCallback(int key, int scancode, int action, int mods){
  if ((key == GLFW_KEY_W) && (action == GLFW_PRESS || GLFW_REPEAT)) {
    m_view_transform = glm::translate(m_view_transform, 
                                      {0.0f, 0.0f, -1.1f});
    updateView();
  }
  else if ((key == GLFW_KEY_S) && (action == GLFW_PRESS ||GLFW_REPEAT)) {
    m_view_transform = glm::translate(m_view_transform, 
                                      {0.0f, 0.0f, 1.1f});
    updateView();
  }
  else if ((key == GLFW_KEY_A && action) == (GLFW_PRESS ||GLFW_REPEAT)) {
    m_view_transform = glm::translate(m_view_transform, 
                                      {-1.1f, 0.0f, 0.0f});
    updateView();
  }
  else if ((key == GLFW_KEY_D && action) == (GLFW_PRESS ||GLFW_REPEAT)) {
    m_view_transform = glm::translate(m_view_transform, 
                                      {1.1f, 0.0f, 0.0f});
    updateView();
  }
  else if ((key == GLFW_KEY_1 && action) == (GLFW_PRESS)) {
    activeShader = "planet";
  }
  else if ((key == GLFW_KEY_2 && action) == (GLFW_PRESS)) {
    activeShader = "planet_cel";
  }
  else if ((key == GLFW_KEY_7 && action) == (GLFW_PRESS)) {
  	greyscale = !greyscale;
    glUseProgram(m_shaders.at("quad").handle);
    glUniform1i(m_shaders.at("quad").u_locs.at("Greyscale"),greyscale);
  }
  else if ((key == GLFW_KEY_8 && action) == (GLFW_PRESS)) {
  	mirrorH = !mirrorH;
    glUseProgram(m_shaders.at("quad").handle);
    glUniform1i(m_shaders.at("quad").u_locs.at("MirrorH"),mirrorH);
  }
  else if ((key == GLFW_KEY_9 && action) == (GLFW_PRESS)) {
  	mirrorV = !mirrorV;
    glUseProgram(m_shaders.at("quad").handle);
    glUniform1i(m_shaders.at("quad").u_locs.at("MirrorV"),mirrorV);
  }
  else if ((key == GLFW_KEY_0 && action) == (GLFW_PRESS)) {
  	gaussian = !gaussian;
    glUseProgram(m_shaders.at("quad").handle);
    glUniform1i(m_shaders.at("quad").u_locs.at("Gaussian"),gaussian);
  }
  
}

/**
 * Handles mouse movements while providing the current x and y delta
 * @param pos_x a double for the x-movement
 * @param pos_y a double for the y-movement
 */ 
void ApplicationSolar::mouseCallback(double pos_x, double pos_y) {
  m_view_transform = glm::rotate(m_view_transform, -0.01f, {pos_y, pos_x, 0.0f});
  rotation = glm::rotate(rotation, 0.01f, glm::fvec3{pos_y, pos_x, 0.0f});  
  updateView();
}

/*----------------------------------------------------------------------------*/
////////////////////////////// Initialization //////////////////////////////////
/*----------------------------------------------------------------------------*/

/**
 * Loads all the shader programs
 */
void ApplicationSolar::initializeShaderPrograms() {
  //fullscreenquadshaderstuff
   m_shaders.emplace("quad", 
                    shader_program{m_resource_path + "shaders/quad.vert",
                    m_resource_path + "shaders/quad.frag"});
  //m_shaders.at("quad").u_locs["ProjectionMatrix"] = -1;
  m_shaders.at("quad").u_locs["ColorTex"] = -1;
  m_shaders.at("quad").u_locs["Greyscale"] = -1;
  m_shaders.at("quad").u_locs["Gaussian"] = -1;
  m_shaders.at("quad").u_locs["MirrorV"] = -1;
  m_shaders.at("quad").u_locs["MirrorH"] = -1;

  // store shader program objects in container
  m_shaders.emplace("planet", 
                    shader_program{m_resource_path + "shaders/simple.vert",
                    m_resource_path + "shaders/simple.frag"});
  // request uniform locations for shader program
  m_shaders.at("planet").u_locs["NormalMatrix"] = -1;
  m_shaders.at("planet").u_locs["ModelMatrix"] = -1;
  m_shaders.at("planet").u_locs["ViewMatrix"] = -1;
  m_shaders.at("planet").u_locs["ProjectionMatrix"] = -1;
  m_shaders.at("planet").u_locs["ColorVector"] = -1;
  m_shaders.at("planet").u_locs["ColorTex"] = -1;

  m_shaders.emplace("planet_normal", 
                    shader_program{m_resource_path + "shaders/normal.vert",
                    m_resource_path + "shaders/normal.frag"});
  // request uniform locations for shader program
  m_shaders.at("planet_normal").u_locs["NormalMatrix"] = -1;
  m_shaders.at("planet_normal").u_locs["ModelMatrix"] = -1;
  m_shaders.at("planet_normal").u_locs["ViewMatrix"] = -1;
  m_shaders.at("planet_normal").u_locs["ProjectionMatrix"] = -1;
  m_shaders.at("planet_normal").u_locs["ColorVector"] = -1;
  m_shaders.at("planet_normal").u_locs["ColorTex"] = -1;
  m_shaders.at("planet_normal").u_locs["NormalTex"] = -1;

    m_shaders.emplace("planet_cel_normal", 
                    shader_program{m_resource_path + "shaders/cel_normal.vert",
                    m_resource_path + "shaders/cel_normal.frag"});
  // request uniform locations for shader program
  m_shaders.at("planet_cel_normal").u_locs["NormalMatrix"] = -1;
  m_shaders.at("planet_cel_normal").u_locs["ModelMatrix"] = -1;
  m_shaders.at("planet_cel_normal").u_locs["ViewMatrix"] = -1;
  m_shaders.at("planet_cel_normal").u_locs["ProjectionMatrix"] = -1;
  m_shaders.at("planet_cel_normal").u_locs["ColorVector"] = -1;
  m_shaders.at("planet_cel_normal").u_locs["ColorTex"] = -1;
  m_shaders.at("planet_cel_normal").u_locs["NormalTex"] = -1;

  m_shaders.emplace("planet_cel", 
                    shader_program{m_resource_path + "shaders/cel.vert",
                    m_resource_path + "shaders/cel.frag"});
  // request uniform locations for shader program
  m_shaders.at("planet_cel").u_locs["NormalMatrix"] = -1;
  m_shaders.at("planet_cel").u_locs["ModelMatrix"] = -1;
  m_shaders.at("planet_cel").u_locs["ViewMatrix"] = -1;
  m_shaders.at("planet_cel").u_locs["ProjectionMatrix"] = -1;
  m_shaders.at("planet_cel").u_locs["ColorVector"] = -1;
  m_shaders.at("planet_cel").u_locs["ColorTex"] = -1;

  m_shaders.emplace("sun", shader_program{m_resource_path + "shaders/sun.vert",
                                        m_resource_path + "shaders/sun.frag"});
  // request uniform locations for shader program
  m_shaders.at("sun").u_locs["ModelMatrix"] = -1;
  m_shaders.at("sun").u_locs["ViewMatrix"] = -1;
  m_shaders.at("sun").u_locs["ProjectionMatrix"] = -1;
  m_shaders.at("sun").u_locs["ColorVector"] = -1;
  m_shaders.at("sun").u_locs["ColorTex"] = -1;

  m_shaders.emplace("skysphere", shader_program{m_resource_path + "shaders/skysphere.vert",
                                        m_resource_path + "shaders/skysphere.frag"});
  // request uniform locations for shader program
  m_shaders.at("skysphere").u_locs["ModelMatrix"] = -1;
  m_shaders.at("skysphere").u_locs["ViewMatrix"] = -1;
  m_shaders.at("skysphere").u_locs["ProjectionMatrix"] = -1;
  m_shaders.at("skysphere").u_locs["ColorVector"] = -1;
  m_shaders.at("skysphere").u_locs["ColorTex"] = -1;


  // storing star shader
  m_shaders.emplace("stars", 
                    shader_program{m_resource_path + "shaders/stars.vert",
                    m_resource_path + "shaders/stars.frag"});

  // stars only need view and projection matrix
  m_shaders.at("stars").u_locs["ViewMatrix"] = -1;
  m_shaders.at("stars").u_locs["ProjectionMatrix"] = -1;

  // storing orbit shader
  m_shaders.emplace("orbit", 
                    shader_program{m_resource_path + "shaders/orbit.vert",
                    m_resource_path + "shaders/orbit.frag"});

  // orbits need model, view and projection matrix
  m_shaders.at("orbit").u_locs["ModelMatrix"] = -1;
  m_shaders.at("orbit").u_locs["ViewMatrix"] = -1;
  m_shaders.at("orbit").u_locs["ProjectionMatrix"] = -1;

}

// load models
void ApplicationSolar::initializeGeometry() {

  model planet_model = model_loader::obj(m_resource_path + "models/sphere.obj", 
                                         model::NORMAL | model::TEXCOORD | model::TANGENT);
  model star_model = model{stars, (model::NORMAL | model::POSITION), {1}};
  model orbit_model = model{orbits, (model::POSITION), {1}};
  model quad_model = model{quad, {model::TEXCOORD | model::POSITION}, {1}};
  
  /**
   * ---| PLANET GEOMETRY
   */

  // generate vertex array object
  glGenVertexArrays(1, &planet_object.vertex_AO);
  // bind the array for attaching buffers
  glBindVertexArray(planet_object.vertex_AO);

  // generate generic buffer
  glGenBuffers(1, &planet_object.vertex_BO);
  // bind this as an vertex array buffer containing all attributes
  glBindBuffer(GL_ARRAY_BUFFER, planet_object.vertex_BO);
  // configure currently bound array buffer
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * planet_model.data.size(), 
               planet_model.data.data(), GL_STATIC_DRAW);

  // activate first attribute on gpu
  glEnableVertexAttribArray(0);
  // first attribute is 3 floats with no offset & stride
  glVertexAttribPointer(0, model::POSITION.components, 
                        model::POSITION.type, 
                        GL_FALSE, planet_model.vertex_bytes, 
                        planet_model.offsets[model::POSITION]);
  // activate second attribute on gpu
  glEnableVertexAttribArray(1);
  // second attribute is 3 floats with no offset & stride
  glVertexAttribPointer(1, model::NORMAL.components, model::NORMAL.type, 
                        GL_FALSE, planet_model.vertex_bytes, 
                        planet_model.offsets[model::NORMAL]);

  glEnableVertexAttribArray(2);
  // third attribute is 3 floats with no offset & stride
  glVertexAttribPointer(2, model::TEXCOORD.components, model::TEXCOORD.type, 
                        GL_FALSE, planet_model.vertex_bytes, 
                        planet_model.offsets[model::TEXCOORD]);

  glEnableVertexAttribArray(3);
  // third attribute is 3 floats with no offset & stride
  glVertexAttribPointer(3, model::TANGENT.components, model::TANGENT.type, 
                        GL_FALSE, planet_model.vertex_bytes, 
                        planet_model.offsets[model::TANGENT]);


   // generate generic buffer
  glGenBuffers(1, &planet_object.element_BO);
  // bind this as an vertex array buffer containing all attributes
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, planet_object.element_BO);
  // configure currently bound array buffer
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, 
               model::INDEX.size * planet_model.indices.size(), 
               planet_model.indices.data(), GL_STATIC_DRAW);

  // store type of primitive to draw
  planet_object.draw_mode = GL_TRIANGLES;
  // transfer number of indices to model object 
  planet_object.num_elements = GLsizei(planet_model.indices.size());


  /**
   * ---| STAR GEOMETRY
   */

  glGenVertexArrays(1, &star_object.vertex_AO);

  glBindVertexArray(star_object.vertex_AO);

  glGenBuffers(1, &star_object.vertex_BO);

  glBindBuffer(GL_ARRAY_BUFFER, star_object.vertex_BO);

  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * star_model.data.size(), 
               star_model.data.data(), GL_STATIC_DRAW);  

  glEnableVertexAttribArray(0);

  glVertexAttribPointer(0, model::POSITION.components, 
                        model::POSITION.type, GL_FALSE, 
                        star_model.vertex_bytes, 
                        star_model.offsets[model::POSITION]);

  glEnableVertexAttribArray(1);

  glVertexAttribPointer(1, model::NORMAL.components, 
                        model::NORMAL.type, GL_FALSE, 
                        star_model.vertex_bytes, 
                        star_model.offsets[model::NORMAL]);

  glGenBuffers(1, &star_object.element_BO);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, star_object.element_BO);

  glBufferData(GL_ELEMENT_ARRAY_BUFFER, 
               model::INDEX.size * star_model.indices.size(), 
               star_model.indices.data(), GL_STATIC_DRAW);
  
  star_object.draw_mode = GL_POINTS;
  // Divide data size by 6 as one element consists out of 6 floats
  star_object.num_elements = GLsizei(star_model.data.size()/6);

  /**
   * ---| ORBIT GEOMETRY
   */

  glGenVertexArrays(1, &orbit_object.vertex_AO);

  glBindVertexArray(orbit_object.vertex_AO);


  glGenBuffers(1, &orbit_object.vertex_BO);

  glBindBuffer(GL_ARRAY_BUFFER, orbit_object.vertex_BO);

  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * orbit_model.data.size(), 
               orbit_model.data.data(), GL_STATIC_DRAW); 

  glEnableVertexAttribArray(0);

  glVertexAttribPointer(0, model::POSITION.components, 
                        model::POSITION.type, GL_FALSE, 
                        orbit_model.vertex_bytes, 
                        orbit_model.offsets[model::POSITION]);

  glGenBuffers(1, &orbit_object.element_BO);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, orbit_object.element_BO);

  glBufferData(GL_ELEMENT_ARRAY_BUFFER, 
               model::INDEX.size * orbit_model.indices.size(), 
               orbit_model.indices.data(), GL_STATIC_DRAW);

  orbit_object.draw_mode = GL_LINE_LOOP;
  // Divide data size by 6 as one element consists out of 3 floats
  orbit_object.num_elements = GLsizei(orbit_model.data.size()/3);

  glBindVertexArray(0); 

  /**
   * ---| QUAD GEOMETRY
   */
    // generate vertex array object
  glGenVertexArrays(1, &quad_object.vertex_AO);
  // bind the array for attaching buffers
  glBindVertexArray(quad_object.vertex_AO);

  // generate generic buffer
  glGenBuffers(1, &quad_object.vertex_BO);
  // bind this as an vertex array buffer containing all attributes
  glBindBuffer(GL_ARRAY_BUFFER, quad_object.vertex_BO);
  // configure currently bound array buffer
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * quad_model.data.size(), 
               quad_model.data.data(), GL_STATIC_DRAW);  

  // activate first attribute on gpu
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(0, model::POSITION.components, model::POSITION.type, 
                        GL_FALSE, quad_model.vertex_bytes, 
                        quad_model.offsets[model::POSITION]);
  // activate second attribute on gpu
  glEnableVertexAttribArray(1);


  glVertexAttribPointer(1, model::TEXCOORD.components, model::TEXCOORD.type, 
                        GL_FALSE, quad_model.vertex_bytes, 
                        quad_model.offsets[model::TEXCOORD]);

  // store type of primitive to draw
  quad_object.draw_mode = GL_TRIANGLE_STRIP;
  // transfer number of indices to model object 
  quad_object.num_elements = GLsizei(quad_model.data.size()/5);

  glBindVertexArray(0); 

}

void ApplicationSolar::initializeFrameBuffer(){
  //Renderbuffer specification
  glGenRenderbuffers(1, &rb_object.handle);
  glBindRenderbuffer(GL_RENDERBUFFER, rb_object.handle);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, GLsizei(1200u), GLsizei(600u));
  glActiveTexture(GL_TEXTURE0);
  glGenTextures(1, &tex_object.handle);
  glBindTexture(GL_TEXTURE_2D, tex_object.handle);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, GLsizei(1200u), GLsizei(600u), 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
  //Framebuffer specification
  //gen fbo & bind for config
  glGenFramebuffers(1, &fb_object.handle);
  glBindFramebuffer(GL_FRAMEBUFFER, fb_object.handle);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, tex_object.handle, 0);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rb_object.handle);

  GLenum draw_buffers[1] = {GL_COLOR_ATTACHMENT1};
  glDrawBuffers(1, draw_buffers);
  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

  if(status != GL_FRAMEBUFFER_COMPLETE){
    std::cout << status <<" creating fb failed" << std::endl;
  }

}
void ApplicationSolar::initializeTextures() {
  // skysphere should not be part of the solar system (right now)
  std::string name = skysphere.name;
  std::cout << name << std::endl;
  int tex_num = skysphere.texture;
  pixel_data texture = texture_loader::file(m_resource_path + "textures/" + name + ".png");
  textures.push_back(texture);
  // assign numbers to textures as stated in struct
  glActiveTexture(GL_TEXTURE0 + tex_num);
  glGenTextures(1, &skysphere.tex_obj.handle);
  glBindTexture(GL_TEXTURE_2D, skysphere.tex_obj.handle);
  //TODO: brauchen wir das?
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, texture.width, texture.height, 0, texture.channels, texture.channel_type, texture.ptr());
  for(auto& p : solar_system){
    std::string name = p.name;
    std::cout << name << std::endl;
    int tex_num = p.texture;
    pixel_data texture = texture_loader::file(m_resource_path + "textures/" + name + ".png");
    textures.push_back(texture);

    glActiveTexture(GL_TEXTURE0 + tex_num);
    glGenTextures(1, &p.tex_obj.handle);
    glBindTexture(GL_TEXTURE_2D, p.tex_obj.handle);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, texture.width, texture.height, 0, texture.channels, texture.channel_type, texture.ptr());
  }
  for(auto& m : moon_system){
    std::string name = m.name;
    int tex_num = m.texture;
    pixel_data texture = texture_loader::file(m_resource_path + "textures/" + name + ".png");
    textures.push_back(texture);

    glActiveTexture(GL_TEXTURE0 + tex_num);
    glGenTextures(1, &m.tex_obj.handle);
    glBindTexture(GL_TEXTURE_2D, m.tex_obj.handle);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, texture.width, texture.height, 0, texture.channels, texture.channel_type, texture.ptr());

  }
  for(auto& p : solar_system){
    if (p.mapped) {
      pixel_data texture_n = texture_loader::file(m_resource_path + "textures/"+p.name+"_normal.png");
      int tex_num = p.texture;
      glActiveTexture(GL_TEXTURE0);
      glGenTextures(1, &p.nor_obj.handle);
      glBindTexture(GL_TEXTURE_2D, p.nor_obj.handle);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, texture_n.width, texture_n.height, 0, texture_n.channels, texture_n.channel_type, texture_n.ptr());

    }
  }

}

void ApplicationSolar::initializeQuad() {
  // pushing back pos-coordinates and uv-coordinates (x,y,z,u,v)
  quad.push_back(-1.0f); // v1
  quad.push_back(-1.0f);
  quad.push_back(0.0f);
  quad.push_back(0.0f);
  quad.push_back(0.0f);

  quad.push_back(1.0f); // v2
  quad.push_back(-1.0f);
  quad.push_back(0.0f);
  quad.push_back(1.0f);
  quad.push_back(0.0f);

  quad.push_back(-1.0f); //v4
  quad.push_back(1.0f);
  quad.push_back(0.0f);
  quad.push_back(0.0f);
  quad.push_back(1.0f);

  quad.push_back(1.0f); // v3
  quad.push_back(1.0f);
  quad.push_back(0.0f);
  quad.push_back(1.0f);
  quad.push_back(1.0f);

}

/**
 * Fill the planet vector with planets and moons
 */
void ApplicationSolar::initializeBigBang() {
  // initializing planets
  planet sun {"sun", 4.0f, 1.0f, 0.0f, {1.0f,1.0f,0.8f}, 2, false};
  planet mercury {"mercury", 1.2f, 0.3f, 6.0f, {0.7f,0.7f,0.7f}, 3, false};
  planet venus {"venus", 1.3f, 0.5f, 12.0f, {0.0f,0.5f,0.6f}, 4, false};
  planet earth {"earth", 1.3f, 0.4f, 18.0f, {0.0f,0.2f,0.9f}, 5, false};
  planet mars {"mars", 1.2f, 0.8f, 24.0f, {1.0f,0.1f,0.1f}, 6, false};
  planet jupiter {"jupiter", 2.0f, 0.1f, 30.0f, {0.9f,0.2f,1.0f}, 7, false};
  planet saturn {"saturn", 2.5f, 0.34f, 36.0f, {0.0f,0.6f,1.4f}, 8, false};
  planet uranus {"uranus", 1.1f, 0.2f, 42.0f, {0.0f,0.0f,0.7f}, 9, false};
  planet neptune {"neptune", 1.1f, 0.36f, 48.0f, {0.0f,0.6f,1.7f} ,10, false};

  // initializing moon
  moon earthmoon {"moon", 0.3f, 2.0f, 2.0f, "earth", {0.0f,0.6f,0.1f}, 11, false};
  moon belt2 {"moon", 0.5f, 3.0f, 3.0f, "saturn", {0.4f,1.0f,0.1f}, 11, false};
  moon belt3 {"moon", 0.5f, 4.0f, 3.0f, "saturn", {0.2f,0.3f,1.0f}, 11, false};
  moon belt1 {"moon", 0.5f, 2.0f, 3.0f, "saturn", {0.0f,0.0f,1.0f}, 11, false};
  moon belt4 {"moon", 0.5f, 5.0f, 3.0f, "saturn", {0.2f,0.2f,1.0f}, 11, false};
  moon belt6 {"moon", 0.5f, 6.0f, 3.0f, "saturn", {0.5f,0.6f,1.0f}, 11, false};
  moon belt7 {"moon", 0.5f, 7.0f, 3.0f, "saturn", {0.4f,1.0f,0.0f}, 11, false};
  moon belt8 {"moon", 0.5f, 8.0f, 3.0f, "saturn", {1.0f,1.0f,0.0f}, 11, false};


  solar_system.insert(solar_system.end(),
               {sun,mercury,venus,earth,mars,jupiter,saturn,uranus,neptune});
  moon_system.insert(moon_system.end(),{earthmoon,belt1,belt2,belt3,belt4});
}

/**
 * Fill the orbit vector with circle points
 */
void ApplicationSolar::initializeOrbits() {
  for (unsigned int i = 0; i < 359; ++i) {
    orbits.push_back((GLfloat)(cos((i * M_PI)/180)));
    orbits.push_back(0.0f);
    orbits.push_back((GLfloat)(-sin((i * M_PI)/180)));
  }
}

/*----------------------------------------------------------------------------*/
/////////////////////////////////// Misc. //////////////////////////////////////
/*----------------------------------------------------------------------------*/

/**
 * Distribute the stars equally and push them into the vector
 * @param amount an integer for the number of stars too be displayed
 */
void ApplicationSolar::distributeStars(unsigned int amount) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<float> dis(-50.0f,50.0f);
  // [x1,y1,z1,r1,g1,b1,x2,y2,z2,r2,g2,b2,...,xn,yn,zn,rn,gn,bn]
  for (unsigned int i = 0; i < 6 * amount; ++i) {
    stars.push_back(dis(gen));
  }
}

/**
 * Calculate orbit depending on the planet's distance to origin
 * @param p a planet object
 */
void ApplicationSolar::getOrbit(planet const& p) const{
  float dist = p.distance_to_origin;
  glm::fmat4 model_matrix = glm::scale(glm::fmat4{}, {dist, dist, dist});
  glUseProgram(m_shaders.at("orbit").handle);
  glUniformMatrix4fv(m_shaders.at("orbit").u_locs.at("ModelMatrix"), 1, 
                     GL_FALSE, glm::value_ptr(model_matrix));
}

/**
 * Calculate orbit depending on the moon's distance to orbiting planet
 * @param m a moon object
 */
void ApplicationSolar::getOrbit(moon const& m) const {
  planet origin;
  for (auto const& p : solar_system) {
    if (m.orbiting == p.name) {
      origin = p;
      glm::fmat4 model_matrix = glm::rotate(glm::fmat4{}, 
                                float(glfwGetTime()* origin.rotation_speed), 
                                {0.0f,1.0f,0.0f});
      model_matrix = glm::translate(model_matrix, 
                                   {0.0f,0.0f,-1.0f*origin.distance_to_origin});
      model_matrix = glm::scale(model_matrix, 
                                {m.distance_to_origin,
                                 m.distance_to_origin,
                                 m.distance_to_origin});
      glUseProgram(m_shaders.at("orbit").handle);
      glUniformMatrix4fv(m_shaders.at("orbit").u_locs.at("ModelMatrix"), 1, 
                         GL_FALSE, glm::value_ptr(model_matrix));
    }
  }
}


void ApplicationSolar::uploadTextures(planet const& p) const {
  glActiveTexture(GL_TEXTURE0 + p.texture);
  glBindTexture(GL_TEXTURE_2D, p.tex_obj.handle);

  int color_sampler_location = glGetUniformLocation(m_shaders.at(activeShader).handle, "ColorTex");
  glUseProgram(m_shaders.at(activeShader).handle);
  glUniform1i(color_sampler_location, p.texture);

  if (p.name == "sun") {
    int color_sampler_location = glGetUniformLocation(m_shaders.at("sun").handle, "ColorTex");
    glUseProgram(m_shaders.at("sun").handle);
    glUniform1i(color_sampler_location, p.texture);
  }

  if (p.mapped) {
  	glActiveTexture(GL_TEXTURE0);
  	glBindTexture(GL_TEXTURE_2D, p.nor_obj.handle);

    int color_sampler_location = glGetUniformLocation(m_shaders.at(activeShader + "_normal").handle, "NormalTex");
    glUseProgram(m_shaders.at(activeShader + "_normal").handle);
    glUniform1i(color_sampler_location, p.texture);

  	glActiveTexture(GL_TEXTURE0 + p.texture);
  	glBindTexture(GL_TEXTURE_2D, p.tex_obj.handle);
    
    color_sampler_location = glGetUniformLocation(m_shaders.at(activeShader + "_normal").handle, "ColorTex");
    glUseProgram(m_shaders.at(activeShader + "_normal").handle);
    glUniform1i(color_sampler_location, p.texture);
  }

}

void ApplicationSolar::uploadTextures(moon const& m) const {
  glActiveTexture(GL_TEXTURE0 + m.texture);
  glBindTexture(GL_TEXTURE_2D, m.tex_obj.handle);

  int color_sampler_location = glGetUniformLocation(m_shaders.at(activeShader).handle, "ColorTex");
  glUseProgram(m_shaders.at(activeShader).handle);
  glUniform1i(color_sampler_location, m.texture);
}

/*----------------------------------------------------------------------------*/
///////////////////////////////// Destructor ///////////////////////////////////
/*----------------------------------------------------------------------------*/

ApplicationSolar::~ApplicationSolar() {
  glDeleteBuffers(1, &planet_object.vertex_BO);
  glDeleteBuffers(1, &planet_object.element_BO);
  glDeleteVertexArrays(1, &planet_object.vertex_AO);

  glDeleteBuffers(1, &star_object.vertex_BO);
  glDeleteBuffers(1, &star_object.element_BO);
  glDeleteVertexArrays(1, &star_object.vertex_AO);

  glDeleteBuffers(1, &orbit_object.vertex_BO);
  glDeleteBuffers(1, &orbit_object.element_BO);
  glDeleteVertexArrays(1, &orbit_object.vertex_AO);

  glDeleteBuffers(1, &quad_object.vertex_BO);
  glDeleteBuffers(1, &quad_object.element_BO);
  glDeleteVertexArrays(1, &quad_object.vertex_AO);
}

/*----------------------------------------------------------------------------*/
//////////////////////////////////// Ḿain //////////////////////////////////////
/*----------------------------------------------------------------------------*/

int main(int argc, char* argv[]) {
  Launcher::run<ApplicationSolar>(argc, argv);
}