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
  Parameter asymmetrical{"asymmetrical", "", 0.5, 0, 1};
  Parameter sunMass{"sunMass", "", 12000, 1000, 30000};
  Parameter sunAsymmetrical{"sunAsymmetrical", "", 1.0, 0.1 ,50.0};

  Light light[2];
  Mesh position{Mesh::POINTS};
  Mesh sphere;
  Material material; 
  std::vector<Vec3f> velocity;
  std::vector<Particle> planets;
  std::vector<Particle> suns;

  void onCreate() override {
    addSphere(sphere, 1.0, 50, 50);
    sphere.generateNormals();
    for (int i = 0; i < 2; i++) {
      Particle sun;
      sun.mass = sunMass;
      sun.force = 0;
      sun.position = Vec3f(i * 5 - 2.5, 0, 0);
      sun.velocity = Vec3f(0, 0, 0);
      suns.push_back(sun);
    }
    suns[0].velocity = (suns[0].position - suns[1].position).normalize().cross(Vec3f(0, 1, 0));
    suns[1].velocity = -(suns[0].position - suns[1].position).normalize().cross(Vec3f(0, 1, 0));
    for (int i = 0; i < 15; i++) {
      Particle temp;
      temp.force = 0;
      temp.position = r() * 7.0f;
      temp.position.y = (double)random() / RAND_MAX * 2 - 1;
      temp.mass = 100;
      while (temp.position.mag() < 5)
      {
        temp.position = r() * 7.0f;
        temp.position.y = (double)random() / RAND_MAX * 2 - 1;
      }
      std::cout<<temp.position<<std::endl;
      temp.position.y = (double)random() / RAND_MAX * 2 - 1;
      temp.velocity = (temp.position - Vec3f(0, 0, 0)).normalize().cross(Vec3f(0,1,0));
      temp.color = c();
      temp.radius = 0.3f;
      planets.push_back(temp);
    }

    for (int i = 0; i < 10; i++) {
      Particle temp;
      temp.force = 0;
      temp.position = r() * 7.0f;
      temp.position.y = (double)random() / RAND_MAX * 2 - 1;
      temp.mass = 10;
      while (temp.position.mag() < 5)
      {
        temp.position = r() * 7.0f;
        temp.position.y = (double)random() / RAND_MAX * 2 - 1;
      }
      temp.position.y = (double)random() / RAND_MAX * 2 - 1;
      temp.velocity = (temp.position - Vec3f(0, 0, 0)).normalize().cross(Vec3f(0,1,0));;
      temp.color = c();
      temp.radius = 0.1f;
      planets.push_back(temp);
    }
    for (int i = 0; i < 20; i++) {
      Particle temp;
      temp.force = 0;
      temp.position = r() * 7.0f;
      temp.position.y = (double)random() / RAND_MAX * 2 - 1;
      temp.mass = 1;
      while (temp.position.mag() < 5)
      {
        temp.position = r() * 7.0f;
        temp.position.y = (double)random() / RAND_MAX * 2 - 1;
      }
      temp.position.y = (double)random() / RAND_MAX * 2 - 1;
      temp.velocity = (temp.position).normalize().cross(Vec3f(0,1,0));
      temp.color = c();
      temp.radius = 0.05f;
      planets.push_back(temp);
    }
    nav().pos(0, 0, 25);
  }

  void onAnimate(double dt) override {
    // calculate the force os gravity...

    // semi-implicit Euler integration
    //
    for(int i = 0; i < suns.size(); i++) {
      suns[i].force = 0;
    }
    {
      float distanceSqr = (suns[0].position - suns[1].position).magSqr();
      Vec3f direction = (suns[0].position - suns[1].position).normalize();
      if (distanceSqr < minDistanceSqr) {
          distanceSqr = minDistanceSqr;
      }
      suns[0].force -= G * suns[0].mass * (float)1e-4 * suns[1].mass / distanceSqr * direction;
      suns[1].force += G * suns[0].mass * (float)1e-4 * suns[1].mass / distanceSqr * direction;
    }
    for (int i = 0; i < planets.size(); i++) {
      planets[i].force = 0;
      for(int j = 0; j < suns.size(); j++) {
        float distanceSqr = (planets[i].position - suns[j].position).magSqr();
        Vec3f direction = (planets[i].position - suns[i].position).normalize();
        if (distanceSqr < minDistanceSqr) {
            distanceSqr = minDistanceSqr;
        }
        planets[i].force -= G * planets[i].mass * (float)1e-4 * suns[j].mass / distanceSqr * direction;
        suns[j].force += G * planets[i].mass * (float)1e-4 * suns[j].mass / distanceSqr * direction * sunAsymmetrical;
      }
    }
    for(int j = 0; j < suns.size(); j++) {
      suns[j].velocity += suns[j].velocity * drag.get() + suns[j].force / suns[j].mass * dt * timeStep;
      suns[j].position += suns[j].velocity * dt * timeStep;
    }
    for (int i = 0; i < planets.size(); i++) {
      for (int j = i + 1; j < planets.size(); j++) {
        float distanceSqr = (planets[i].position - planets[j].position).magSqr();
        if (distanceSqr < minDistanceSqr) {
          distanceSqr = minDistanceSqr;
        }
        Vec3f direction = (planets[i].position - planets[j].position).normalize();
        planets[i].force -= G * (float)1e-4 * planets[i].mass * planets[j].mass / distanceSqr * direction;
        planets[j].force += G * (float)1e-4 * planets[i].mass * planets[j].mass / distanceSqr * direction * asymmetrical;
      }
    }
    for (int i = 0; i < planets.size(); i++) {
      planets[i].velocity += -planets[i].velocity * drag.get() + planets[i].force / planets[i].mass * dt * timeStep;
      planets[i].position += planets[i].velocity * dt * timeStep;
    }
    nav().faceToward(Vec3f(0, 0, 0));
  }

  bool onKeyDown(Keyboard const& k) override {
    return true;
  }

  void onDraw(Graphics& g) override {
    g.clear(0.1);
    gl::depthTesting(true);
    g.lighting(false);
    for (int i = 0; i < suns.size(); i++) {
      light[i].ambient(RGB(0));
      if (i == 0)
        light[i].diffuse(RGB(1, 1, 0.5));
      else if (i == 1) 
        light[i].diffuse(RGB(0.5, 1, 1.0));
      light[i].pos(suns[i].position.x, suns[i].position.y, suns[i].position.z);
      
      g.pushMatrix();
      g.translate(Vec3f(light[i].pos()));
      g.scale(Vec3f(1, 1, 1));
      g.color(light[i].diffuse());
      g.draw(sphere);
      g.popMatrix();
    }
    
    g.lighting(true);
    g.light(light[0], 0);
    g.light(light[1], 1);
    light[0].ambient(Color(1.0f, 0.5f, 0));
    light[1].ambient(Color(0.0f, 0.5f, 1.0f));
    for (int i = 0; i < planets.size(); i++) {
      g.pushMatrix();
      g.translate(planets[i].position);
      g.scale(Vec3f(planets[i].radius, planets[i].radius, planets[i].radius));
      material.specular(light[0].diffuse() * 0.2f + 0.2f * light[1].diffuse());
      material.shininess(128); 
      material.ambient(planets[i].color * (light[0].diffuse() + light[1].diffuse()));

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
    gui.add(asymmetrical);
    gui.add(sunMass);
    gui.add(sunAsymmetrical);
  }
};

int main() {
  AnApp app;
  app.start();
  return 0;
}