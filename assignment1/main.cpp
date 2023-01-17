#include "al/app/al_App.hpp"
#include "al/graphics/al_Image.hpp"
#include "al/io/al_File.hpp"

using namespace al;
using namespace std;

static bool simulationMap[2405][1605];

typedef struct particle
{
  int id;
  int x, y;
  Vec2d velocity;
  Color color;
  bool hasUpdate;
  bool isWater;
} particle;


struct AnApp : App {
  Mesh original, current, cube, cylinder;
  // hint: add more meshes here: cube, cylindar, custom
  Mesh cubeAxis, cylinderAxis;
  Mesh target, pre;
  vector<particle> particles;

  bool simulating;
  bool animating;
  const float animationTime = 1.0f; // whole animation time
  float steptime;
  int axisShowing = 0;
  Vec3d mean;  // note: we will learn average pixel position

  const int boundHX = 2200;
  const int boundHY = 1400;
  const int boundLX = -200;
  const int boundLY = -200;

  int imageWidth;
  int imageHeight;

  Vec3d navTarget;

  Vec2i position2Simulation(float x, float y, bool hasMean = true) {
    int sX;
    int sY;
    if (hasMean) {
      sX = (x + mean.x) * imageWidth;
      sY = (y + mean.y) * imageHeight;
    } else {
      sX = (x) * imageWidth;
      sY = (y) * imageHeight;
    }
    
    if (sX > boundHX) {
      sX = boundHX;
    } else if(sX < boundLX) {
      sX = boundLX;
    }
    if (sY > boundHY) {
      sY = boundHY;
    } else if (sY < boundLY) {
      sY = boundLY;
    }
    return Vec2i(round(sX), round(sY));
  }

  Vec2d position2Screen(int x, int y) {
    double sX = (double)x / imageWidth - mean.x;
    double sY = (double)y / imageHeight - mean.y;
    return Vec2d(sX, sY);
  }

  void createCylinderAxis() {
    cylinderAxis.primitive(Mesh::LINES);
    int indexNum = 100;
    float step = 1.0f / indexNum;
    for (float i = 0.0f; i < 1; i += step) {
      cylinderAxis.vertex(cos(2 * M_PI * i), -0.5f, sin(2 * M_PI * i));
      cylinderAxis.color(Color(HSV(cos(2 * M_PI * i), 0.0f, sin(2 * M_PI * i))));

      cylinderAxis.vertex(cos(2 * M_PI * i), 0.5f, sin(2 * M_PI * i));
      cylinderAxis.color(Color(HSV(cos(2 * M_PI * i), 1.0f, sin(2 * M_PI * i))));
    }
    for (int i = 0; i < indexNum; i++) {
      if (i != indexNum - 1) {
        cylinderAxis.index(i * 2);
        cylinderAxis.index((i + 1) * 2);
        cylinderAxis.index(i * 2 + 1);
        cylinderAxis.index((i + 1) * 2 + 1);
      } else {
        cylinderAxis.index(i * 2);
        cylinderAxis.index(0);
        cylinderAxis.index(i * 2 + 1);
        cylinderAxis.index(1);
      }
    }
  }

  void createCubeAxis() {
    cubeAxis.primitive(Mesh::LINES);
    for (int i = 0; i <= 1; i++){
      for (int j = 0; j <= 1; j++){
        for (int k = 0; k <= 1; k++){
          cubeAxis.vertex(i - 0.5f, j - 0.5f, k - 0.5f);
          cubeAxis.color(i, j, k);
          if (i == 0) {
            cubeAxis.index(k + j * 2 + i * 4);
            cubeAxis.index(k + j * 2 + (i + 1) * 4);
          }
          if (j == 0) {
            cubeAxis.index(k + j * 2 + i * 4);
            cubeAxis.index(k + (j + 1) * 2 + i * 4);
          }
          if (k == 0) {
            cubeAxis.index(k + j * 2 + i * 4);
            cubeAxis.index(k + 1 + j * 2 + i * 4);
          }
        }
      }
    }
  }

  void onCreate() override {
    // note: File::currentPath() is going to be something like:
    //   /Users/foo/allolib_playground/code-folder/bin
    // and we will put an image file next to our .cpp in this folder
    //   /Users/foo/allolib_playground/code-folder
    // so we add the "../" to our image file name
    animating = false;
    simulating = false;
    const std::string filename = File::currentPath() + "../mpv-shot0005.jpg";
    cout << "loading: " << filename << endl;
    auto image = Image(filename);
    if (image.array().size() == 0) {
      cout << "failed to load image " << filename << endl;
      exit(1);
    }
    cout << "image size: " << image.width() << "x" << image.height() << endl;
    imageWidth = image.width();
    imageHeight = image.height();

    // get the RGB of each pixel in the image
    int index = 0;
    for (int column = 0; column < image.width(); ++column) {
      for (int row = 0; row < image.height(); ++row) {
        // here's how to look up a pixel in an image
        auto pixel = image.at(column, row);

        // pixel has .r .g .b and .a ~ each is an 8-bit unsigned integer
        // we need a float on (0, 1) so we divide by 255.0
        Color color(pixel.r / 255.0, pixel.g / 255.0, pixel.b / 255.0);

        // here we normalize the row and column while maintaining the
        // correct aspect ratio by dividing by width
        Vec3d position(1.0 * column / image.width(),
                       1.0 - 1.0 * row / image.width(), 0.0);
        mean += position;  // we will learn the average pixel position

        // add data to a mesh
        original.vertex(position);
        original.color(color);
        
        // rgb
        Vec3d cubePosition(pixel.r / 255.0 - 0.5f, pixel.g / 255.0 - 0.5f, pixel.b / 255.0 - 0.5f);
        cube.vertex(cubePosition);
        cube.color(color);

        // hsv
        HSV hsvColor = HSV(color);

        cylinder.vertex(Vec3d(hsvColor.s * cos(2 * M_PI * hsvColor.h), hsvColor.v - 0.5f, hsvColor.s * sin(2 * M_PI * hsvColor.h)));
        cylinder.color(color);

        // add data to a mesh
        current.vertex(position);
        current.color(color);

        particle p;
        p.id = index;
        p.color = color;
        p.velocity = 1.0f;
        p.hasUpdate = false;
        auto simPos = position2Simulation(position.x, position.y, false);
        p.x = simPos.x;
        p.y = simPos.y;
        simulationMap[simPos.x - boundLX][simPos.y - boundLY] = true;
        if (cos(2 * M_PI * hsvColor.h) < 0) {
          p.isWater = true;
        } else {
          p.isWater = false;
        }
        particles.push_back(p);
        index++;
        // hint: configure more meshes here
      }
    }
    createCubeAxis();
    createCylinderAxis();
    // here we center the image by subtracting the average position
    mean /= original.vertices().size();
    for (auto& v : original.vertices()) v -= mean;
    for (auto& v : current.vertices()) v -= mean;
    // configure the meshs to render as points
    original.primitive(Mesh::POINTS);
    current.primitive(Mesh::POINTS);
    cube.primitive(Mesh::POINTS);
    cylinder.primitive(Mesh::POINTS);

    // hint: configure the new meshes here

    // set the viewer position 7 units back
    nav().pos(0, 0, 3);
  }

  void onAnimate(double dt) override {
    // hint: create an animation timer

    // note: meshname.vertices() is the array of vertices
    // note: meshname.vertices().size() is the number of vertices
    // note: meshname.vertices()[5] is the vertex (a Vec3d) at index 5
    // hint: consider Vec3d::lerp(...) function
    if (animating) {
      steptime += dt;
      if (steptime > animationTime) {
        steptime = animationTime;
        for (int i = 0; i < current.vertices().size(); i++) {
          Vec3d position = target.vertices().at(i);
          if (simulating) {
            position = Vec3d(position2Screen(particles[i].x, particles[i].y), position.z);
          }
          current.vertices().at(i).set(position);
          animating = false;
        }
        return;
      }
      for (int i = 0; i < current.vertices().size(); i++) {
        Vec3d position; 
        if (simulating) {
          position = lerp(pre.vertices().at(i), al::Mesh::Vertex(position2Screen(particles[i].x, particles[i].y), position.z), steptime / animationTime);
        } else {
          position = lerp(pre.vertices().at(i), target.vertices().at(i), steptime / animationTime);
        }
        current.vertices().at(i).set(position);
      }
      nav().pos().lerp(navTarget, steptime / animationTime);
    } else if (simulating) {
      for (int i = 0; i < current.vertices().size(); i++) {
        if (particles[i].isWater) {
          Vec3f position = current.vertices().at(i);
          Vec2i sPos(particles[i].x, particles[i].y);
          Vec2i newPos = sPos;
          bool update = false;
          float rand = (float)random()/RAND_MAX;
          if (sPos.y > boundLY) {
            for (int i = -1; i <= 1; i++) {
              if (sPos.x + i >= boundLX && sPos.x + i <= boundHX && rand > (i + 1) * 0.33f) {
                if (!simulationMap[sPos.x - boundLX + i][sPos.y - boundLY - 1]) {
                  newPos = Vec2i(sPos.x + i, sPos.y - 1);
                  update = true;
                }
              }
            }
          }
          if (!update) {
            for (int i = -1; i <= 1; i+=2) {
              if (sPos.x + i >= boundLX && sPos.x + i <= boundHX) {
                if (!simulationMap[sPos.x - boundLX + i][sPos.y - boundLY]) {
                  newPos = Vec2i(sPos.x + i, sPos.y);
                }
              }
            }
          }
          simulationMap[sPos.x - boundLX][sPos.y - boundLY] = false;
          simulationMap[newPos.x - boundLX][newPos.y - boundLY] = true;
          particles[i].x = newPos.x;
          particles[i].y = newPos.y;
          /*if (sPos.x != newPos.x || sPos.y != newPos.y){
            std::cout<<sPos << newPos <<endl;
          }*/
          auto temp = position2Screen(newPos.x, newPos.y);
          Vec3d newPosition = Vec3d(temp.x, temp.y, position.z);
          current.vertices().at(i).set(newPosition);
        }
      }
    }
    // animate changes to the CURRENT mesh using linear interpolation
    nav().faceToward(Vec3d(0, 0, 0));
  }

  bool onKeyDown(Keyboard const& k) override {
    switch (k.key()) {
      case '1': {
        navTarget = Vec3d(0, 0, 2.0f);
        animating = true;
        pre = current;
        target = original;
        steptime = 0.0f;
        axisShowing = 0;
        simulating = false;
      } break;
      case '2': {
        // note: trigger transition toward an RGB cube
        navTarget = Vec3d(0, 0, 3.0f);
        animating = true;
        pre = current;
        target = cube;
        steptime = 0.0f;
        axisShowing = 1;
        simulating = false;
      } break;
      case '3': {
        // note: trigger transition toward an HSV cylinder
        navTarget = Vec3d(0, 0, 4.0f);
        animating = true;
        pre = current;
        target = cylinder;
        steptime = 0.0f;
        axisShowing = 2;
        simulating = false;
      } break;
      case '4': {
        // note: trigger transition to your custom arrangement
        navTarget = Vec3d(0, 0, 4.0f);
        animating = true;
        pre = current;
        target = original;
        steptime = 0.0f;
        axisShowing = 0;
        simulating = true;
      } break;
    }
    return true;
  }

  void onDraw(Graphics& g) override {
    // note: you don't need to touch anything here
    g.clear(0.25);
    g.meshColor();
    g.draw(current);
    if (axisShowing == 1) {
      g.draw(cubeAxis);
    } else if(axisShowing == 2) {
      g.draw(cylinderAxis);
    }
  }
};

int main() {
  AnApp app;
  app.start();
  return 0;
}