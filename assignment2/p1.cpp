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

using namespace al;
using namespace std;

Vec3f r() { return Vec3f(rnd::uniformS(), rnd::uniformS(), rnd::uniformS()); }
RGB c() { return RGB(rnd::uniform(), rnd::uniform(), rnd::uniform()); }

struct AnApp : App {
  Parameter drag{"drag", "", 0.01, 0, 0.1};
  Parameter G{"G", "", 6.67, 0, 30};
  Parameter minDistanceSqr{"minDistanceSqr", "", 0.2, 0.05, 0.5};
  Parameter timeStep{"timeStep", "", 1, 0, 10};
  Mesh position{Mesh::POINTS};
  std::vector<Vec3f> velocity;
  std::vector<Vec3f> force;

  void onCreate() override {
    for (int i = 0; i < 1000; i++) {
      position.vertex(r());
      position.color(c());
      velocity.push_back(r() * 0.01);
      force.push_back(0);
    }
    nav().pos(0, 0, 5);
  }

  void onAnimate(double dt) override {
    // calculate the force os gravity... F = am, F/m * dt = dv;
    for (int i = 0; i < velocity.size(); ++i) {
      for (int j = i + 1; j < velocity.size(); ++j) {
        float distanceSqr = (position.vertices()[i] - position.vertices()[j]).magSqr();
        if (distanceSqr < minDistanceSqr) {
          distanceSqr = minDistanceSqr;
        }
        Vec3f direction = (position.vertices()[i] - position.vertices()[j]).normalize();
        force[i] -= G * (float)1e-4 / distanceSqr * direction;
        force[j] += G * (float)1e-4 / distanceSqr * direction;
      }
    }
    // semi-implicit Euler integration
    //
    for (int i = 0; i < velocity.size(); ++i) {
      velocity[i] += -velocity[i] * drag.get() + force[i] * dt * timeStep;
      force[i] = 0;
    }
    for (int i = 0; i < position.vertices().size(); ++i) {
      position.vertices()[i] += velocity[i] * dt * timeStep;
    }
  }

  bool onKeyDown(Keyboard const& k) override {
    for (int i = 0; i < position.vertices().size(); ++i) {
      position.vertices()[i] = r();
      velocity[i] = r() * 0.01;
    }
    return true;
  }

  void onDraw(Graphics& g) override {
    g.clear(0.25);
    g.pointSize(20);
    g.meshColor();
    g.draw(position);
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