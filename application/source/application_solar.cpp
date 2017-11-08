#include "application_solar.hpp"
#include "launcher.hpp"

#include "utils.hpp"
#include "shader_loader.hpp"
#include "model_loader.hpp"

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

/*----------------------------------------------------------------------------*/
///////////////////////////////// Constructor //////////////////////////////////
/*----------------------------------------------------------------------------*/

ApplicationSolar::ApplicationSolar(std::string const& resource_path)
 :Application{resource_path}
 ,planet_object{}
 ,star_object{}
 ,orbit_object{}
{ 
  initializeBigBang();
  distributeStars(starAmount);
  initializeOrbits();
  initializeGeometry();
  initializeShaderPrograms();
}


/*----------------------------------------------------------------------------*/
///////////////////////////////// Rendering ////////////////////////////////////
/*----------------------------------------------------------------------------*/

void ApplicationSolar::render() const {
  // bind star shader
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

  glBindVertexArray(0);
  
}

/*----------------------------------------------------------------------------*/
////////////////////////////// Transform upload ////////////////////////////////
/*----------------------------------------------------------------------------*/

/**
 * Uploads the transformation matrix to shader to create a planet
 * @param p a planet object
 */
void ApplicationSolar::uploadPlanetTransforms(planet p) const {
  glUseProgram(m_shaders.at("planet").handle);
  glm::fmat4 model_matrix;
  model_matrix = glm::rotate(model_matrix, 
                 float(glfwGetTime()* p.rotation_speed), 
                 glm::fvec3{0.0f, 1.0f, 0.0f});
  model_matrix = glm::translate(model_matrix, 
                 glm::fvec3 {0.0f, 0.0f, -1.0f*p.distance_to_origin});
  model_matrix = glm::scale(model_matrix, 
                 glm::fvec3 {p.size, p.size, p.size});

  glUniformMatrix4fv(m_shaders.at("planet").u_locs.at("ModelMatrix"),
                        1, GL_FALSE, glm::value_ptr(model_matrix));

  // extra matrix for normal transformation to keep them orthogonal to surface
  glm::fmat4 normal_matrix = 
          glm::inverseTranspose(glm::inverse(m_view_transform) * model_matrix);
  glUniformMatrix4fv(m_shaders.at("planet").u_locs.at("NormalMatrix"),
                     1, GL_FALSE, glm::value_ptr(normal_matrix));
}

/**
 * Uploads the transformation matrix to shader to create a moon
 * @param p a moon object
 */
void ApplicationSolar::uploadMoonTransforms(moon m) const {
  glUseProgram(m_shaders.at("planet").handle);
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

  glUniformMatrix4fv(m_shaders.at("planet").u_locs.at("ModelMatrix"),
                     1, GL_FALSE, glm::value_ptr(model_matrix));

  // extra matrix for normal transformation to keep them orthogonal to surface
  glm::fmat4 normal_matrix = glm::inverseTranspose(
                             glm::inverse(m_view_transform) * model_matrix);
  glUniformMatrix4fv(m_shaders.at("planet").u_locs.at("NormalMatrix"),
                     1, GL_FALSE, glm::value_ptr(normal_matrix));
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
  // upload matrix to gpu
  glUseProgram(m_shaders.at("planet").handle);
  glUniformMatrix4fv(m_shaders.at("planet").u_locs.at("ProjectionMatrix"),
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
}

/**
 * Handles mouse movements while providing the current x and y delta
 * @param pos_x a double for the x-movement
 * @param pos_y a double for the y-movement
 */ 
void ApplicationSolar::mouseCallback(double pos_x, double pos_y) {
  m_view_transform = glm::rotate(m_view_transform, -0.01f, {pos_y, pos_x, 0.0f});
    updateView();
}

/*----------------------------------------------------------------------------*/
////////////////////////////// Initialization //////////////////////////////////
/*----------------------------------------------------------------------------*/

/**
 * Loads all the shader programs
 */
void ApplicationSolar::initializeShaderPrograms() {
  // store shader program objects in container
  m_shaders.emplace("planet", 
                    shader_program{m_resource_path + "shaders/simple.vert",
                    m_resource_path + "shaders/simple.frag"});
  // request uniform locations for shader program
  m_shaders.at("planet").u_locs["NormalMatrix"] = -1;
  m_shaders.at("planet").u_locs["ModelMatrix"] = -1;
  m_shaders.at("planet").u_locs["ViewMatrix"] = -1;
  m_shaders.at("planet").u_locs["ProjectionMatrix"] = -1;

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
                                         model::NORMAL);
  model star_model = model{stars, (model::NORMAL+model::POSITION), {1}};
  model orbit_model = model{orbits, (model::POSITION), {1}};
  
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

}

/**
 * Fill the planet vector with planets and moons
 */
void ApplicationSolar::initializeBigBang() {
  // initializing planets
  planet sun {"sun", 4.0f, 1.0f, 0.0f};
  planet mercury {"mercury", 1.2f, 0.3f, 6.0f};
  planet venus {"venus", 1.3f, 0.5f, 12.0f};
  planet earth {"earth", 1.3f, 0.4f, 18.0f};
  planet mars {"mars", 1.2f, 0.8f, 24.0f};
  planet jupiter {"jupiter", 2.0f, 0.1f, 30.0f};
  planet saturn {"saturn", 2.5f, 0.34f, 36.0f};
  planet uranus {"uranus", 1.1f, 0.2f, 42.0f};
  planet neptune {"neptune", 1.1f, 0.36f, 48.0f};

  // initializing moon
  moon earthmoon {"moon", 0.3f, 2.0f, 2.0f, "earth"};
  moon belt1 {"belt1", 0.5f, 2.0f, 3.0f, "saturn"};
  moon belt2 {"belt2", 0.5f, 3.0f, 3.0f, "saturn"};
  moon belt3 {"belt3", 0.5f, 4.0f, 3.0f, "saturn"};
  moon belt4 {"belt4", 0.5f, 5.0f, 3.0f, "saturn"};
  moon belt6 {"belt4", 0.5f, 6.0f, 3.0f, "saturn"};
  moon belt7 {"belt4", 0.5f, 7.0f, 3.0f, "saturn"};
  moon belt8 {"belt4", 0.5f, 8.0f, 3.0f, "saturn"};

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
void ApplicationSolar::getOrbit(moon const& m) const{
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
}

/*----------------------------------------------------------------------------*/
//////////////////////////////////// Ḿain //////////////////////////////////////
/*----------------------------------------------------------------------------*/

int main(int argc, char* argv[]) {
  Launcher::run<ApplicationSolar>(argc, argv);
}