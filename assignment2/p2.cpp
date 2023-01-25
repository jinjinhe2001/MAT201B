// Karl Yerkes
// 2023-01-19
// MAT 201B
// Starter Code for Alssignment 2
// Jinjin He

#include "al/app/al_App.hpp"
#include "al/app/al_GUIDomain.hpp"
#include "al/graphics/al_Image.hpp"
#include "al/io/al_File.hpp"
#include "al/math/al_Random.hpp"
#include "al/graphics/al_Shapes.hpp"

using namespace al;
using namespace std;

Vec3f r() { return Vec3f(rnd::uniformS(), rnd::uniformS(), rnd::uniformS()); }
RGB c() { return RGB(rnd::uniform(), rnd::uniform(), rnd::uniform()); }

typedef struct Particle
{
  Vec3f velocity;
  Vec3f force;
  Vec3f position;
  Color color;
  float mass;
  float radius;
} Particle;

struct AnApp : App {
  Parameter drag{"drag", "", 0, 0, 0.1};
  Parameter G{"G", "", 6.67, 0, 30};
  Parameter minDistanceSqr{"minDistanceSqr", "", 9, 4, 25};
  Parameter timeStep{"timeStep", "", 1, 0, 10};
  Light light;
  Mesh position{Mesh::POINTS};
  Mesh sphere;
  Material material; 
  std::vector<Vec3f> velocity;
  std::vector<Particle> planets;
  Particle sun;

  void onCreate() override {
    addSphere(sphere, 1.0, 50, 50);
    sphere.generateNormals();
    sun.mass = 12000;
    sun.force = 0;
    sun.position = Vec3f(0, 0, 0);
    sun.velocity = Vec3f(0, 0, 0);
    for (int i = 0; i < 8; i++) {
      Particle temp;
      temp.force = 0;
      temp.position = r() * 4.0f;
      temp.mass = 100;
      while (temp.position.mag() < 5)
      {
        temp.position = r() * 7.0f;
      }
      temp.position.y = (double)random() / RAND_MAX * 2 - 1;
      temp.velocity = (temp.position - sun.position).normalize().cross(Vec3f(0,1,0));
      temp.color = c();
      temp.radius = 0.3f;
      planets.push_back(temp);
    }
    for (int i = 0; i < 10; i++) {
      Particle temp;
      temp.force = 0;
      temp.position = r() * 4.0f;
      temp.mass = 10;
      while (temp.position.mag() < 5)
      {
        temp.position = r() * 7.0f;
      }
      temp.position.y = (double)random() / RAND_MAX * 2 - 1;
      temp.velocity = (temp.position - sun.position).normalize().cross(Vec3f(0,1,0));;
      temp.color = c();
      temp.radius = 0.1f;
      planets.push_back(temp);
    }
    for (int i = 0; i < 20; i++) {
      Particle temp;
      temp.force = 0;
      temp.position = r() * 4.0f;
      temp.mass = 1;
      while (temp.position.mag() < 5)
      {
        temp.position = r() * 7.0f;
      }
      temp.position.y = (double)random() / RAND_MAX * 2 - 1;
      temp.velocity = (temp.position - sun.position).normalize().cross(Vec3f(0,1,0));
      temp.color = c();
      temp.radius = 0.05f;
      planets.push_back(temp);
    }
    nav().pos(0, 0, 20);
  }

  void onAnimate(double dt) override {
    // calculate the force os gravity...

    // semi-implicit Euler integration
    //
    sun.force = 0;
    for (int i = 0; i < planets.size(); i++) {
      planets[i].force = 0;
      float distanceSqr = (planets[i].position - sun.position).magSqr();
      Vec3f direction = (planets[i].position - sun.position).normalize();
      if (distanceSqr < minDistanceSqr) {
          distanceSqr = minDistanceSqr;
      }
      planets[i].force -= G * planets[i].mass * (float)1e-4 * sun.mass / distanceSqr * direction;
      sun.force += G * planets[i].mass * (float)1e-4 * sun.mass / distanceSqr * direction;
    }
    sun.velocity += sun.velocity * drag.get() + sun.force / sun.mass * dt * timeStep;
    sun.position += sun.velocity * dt * timeStep;
    for (int i = 0; i < planets.size(); i++) {
      for (int j = i + 1; j < planets.size(); j++) {
        float distanceSqr = (planets[i].position - planets[j].position).magSqr();
        if (distanceSqr < minDistanceSqr) {
          distanceSqr = minDistanceSqr;
        }
        Vec3f direction = (planets[i].position - planets[j].position).normalize();
        planets[i].force -= G * (float)1e-4 * planets[i].mass * planets[j].mass / distanceSqr * direction;
        planets[j].force += G * (float)1e-4 * planets[i].mass * planets[j].mass / distanceSqr * direction;
      }
    }
    for (int i = 0; i < planets.size(); i++) {
      planets[i].velocity += -planets[i].velocity * drag.get() + planets[i].force / planets[i].mass * dt * timeStep;
      planets[i].position += planets[i].velocity * dt * timeStep;
    }
    nav().faceToward(Vec3f(0, 0, 0));
  }

  bool onKeyDown(Keyboard const& k) override {
    for (int i = 0; i < position.vertices().size(); ++i) {
      position.vertices()[i] = r();
      velocity[i] = r() * 0.01;
    }
    return true;
  }

  void onDraw(Graphics& g) override {
    g.clear(0.1);
    gl::depthTesting(true);
    g.lighting(false);
    light.ambient(RGB(0));
    light.diffuse(RGB(1, 1, 0.5));
    light.pos(sun.position.x, sun.position.y, sun.position.z);
    
    g.pushMatrix();
    g.scale(Vec3f(1, 1, 1));
    g.translate(Vec3f(light.pos()));
    g.color(light.diffuse());
    g.draw(sphere);
    g.popMatrix();

    
    g.lighting(true);
    g.light(light);
    light.globalAmbient(Color(1.0f, 0.5f, 0));
    for (int i = 0; i < planets.size(); i++) {
      g.pushMatrix();
      g.translate(planets[i].position);
      g.scale(Vec3f(planets[i].radius, planets[i].radius, planets[i].radius));
      material.specular(light.diffuse() * 0.2f);
      material.shininess(128); 
      material.ambient(Color(1.0f, 1.0f, 1.0f) * planets[i].color);

      g.material(material);
      g.draw(sphere);
      g.popMatrix();
    }
  }

  void onInit() override {
    auto GUIdomain = GUIDomain::enableGUI(defaultWindowDomain());
    auto& gui = GUIdomain->newGUI();
    gui.add(drag);
    gui.add(G);
    gui.add(minDistanceSqr);
    gui.add(timeStep);
  }
};

int main() {
  AnApp app;
  app.start();
  return 0;
}