#ifndef ApplicationATION_SOLAR_HPP
#define APPLICATION_SOLAR_HPP

#include "application.hpp"
#include "model.hpp"
#include "structs.hpp"
#include "pixel_data.hpp"


// gpu representation of model
class ApplicationSolar : public Application {
 public:
  // allocate and initialize objects
  ApplicationSolar(std::string const& resource_path);
  // free allocated objects
  ~ApplicationSolar();

  // update uniform locations and values
  void uploadUniforms();
  // update projection matrix
  void updateProjection();
  // react to key input
  void keyCallback(int key, int scancode, int action, int mods);
  //handle delta mouse movement input
  void mouseCallback(double pos_x, double pos_y);

  // draw all objects
  void render() const;

  void uploadPlanetTransforms(planet const& p) const;

  void uploadTextures(planet const& p) const;

  void uploadTextures(moon const& m) const;

  // calculate orbit for planets and moons
  void getOrbit(planet const& p) const;

  void getOrbit(moon const& m) const;


  void uploadMoonTransforms(moon m) const;

 protected:
  void distributeStars(unsigned int amount);
  void initializeBigBang();
  void initializeOrbits();
  void initializeShaderPrograms();
  void initializeGeometry();
  void initializeTextures();
  void initializeQuad();
  void updateView();
  void initializeFrameBuffer();

  // cpu representation of model
  model_object planet_object;
  model_object star_object;
  model_object orbit_object;
  model_object quad_object;
  texture_object tex_object;
  texture_object fb_object;
  texture_object rb_object;
  // vector storing all the planets
  std::vector<planet> solar_system;
  std::vector<moon> moon_system;
  std::vector<GLfloat> orbits; 
  std::vector<GLfloat> stars;
  std::vector<pixel_data> textures;
  std::vector<GLfloat> quad;

  std::string activeShader = "planet_cel";
};

#endif