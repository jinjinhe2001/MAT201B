/*
Start Code
Allocore Example: Flocking

Description:
This is an example implementation of a flocking algorithm. The original flocking
algorithm [1] consists of three main interactions between flockmates ("boids"):

    1) Collision avoidance (of nearby flockmates)
    2) Velocity matching (of nearby flockmates)
    3) Flock centering (of nearby flockmates)

Here, we implement 1) and 2) only. Another change from the reference source is
the use of Gaussian functions rather than inverse-squared functions for
calculating the "nearness" of flockmates. This is done primarily to avoid
infinities, but also to give smoother motions. Lastly, we give each boid a
random walk motion which helps both dissolve and redirect the flocks.

[1] Reynolds, C. W. (1987). Flocks, herds, and schools: A distributed behavioral
    model. Computer Graphics, 21(4):25–34.

Author:
Lance Putnam, Oct. 2014
*/
// Implementation: Jinjin He

#include <cmath>
#include "al/app/al_App.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/math/al_Functions.hpp"
#include "al/math/al_Random.hpp"
#include "al/app/al_GUIDomain.hpp"
#include "instance.hpp"

using namespace al;

// A "boid" (play on bird) is one member of a flock.
struct myLight {
    Vec3f lightPos;
    Vec3f lightColor;
};

class Boid
{
public:
    Boid() {
        position = Vec3f(0.0f);
        faceTo = Vec3f(0.0f);
        velocity = Vec3f(0.0f);
        accelerate = Vec3f(0.0f);
    }
    // Each boid has a position and velocity.
    Vec3f position;
    Vec3f faceTo;
    Vec3f velocity;
    Vec3f accelerate;
    Vec3f sightPos;
    int sightWeight;

    // Update position based on velocity and delta time
    void update(float dt) { position += velocity * dt; }
};

struct MyApp : public App
{
    static const int instanceNum = 2000; // Number of boids
    vector<Boid> boids;
    Mesh heads, tails;
    Mesh box;
    VAOMesh bird;
    BufferObject buffer;
    BufferObject rotateBuffer[4];
    BufferObject colorBuffer;
    ParameterVec3 boxBound{"box", "", Vec3f(6.0f)};
    Parameter sight{"sight", 0.8, 0.1, 3.0};
    Parameter speedLimit{"speedLimit", 1.0, 0.1, 2.0};
    Parameter targetRadius{"targetRadius", 3.0f, 0.5f, 6.0f};
    Parameter foodAppeal{"foodAppeal", 1.0f, 0.1f, 3.0f};
    Parameter foodspeed{"foodspeed", 0.7f, 0.1f, 5.0f};
    Vec3f targetPos;
    float targetTheta;

    ShaderProgram shader_instancing;
    myLight light;

    void createbox()
    {
        box.primitive(Mesh::LINES);
        for (int i = 0; i <= 1; i++)
        {
            for (int j = 0; j <= 1; j++)
            {
                for (int k = 0; k <= 1; k++)
                {
                    box.vertex(i - 0.5f, j - 0.5f, k - 0.5f);
                    box.color(i, j, k);
                    if (i == 0)
                    {
                        box.index(k + j * 2 + i * 4);
                        box.index(k + j * 2 + (i + 1) * 4);
                    }
                    if (j == 0)
                    {
                        box.index(k + j * 2 + i * 4);
                        box.index(k + (j + 1) * 2 + i * 4);
                    }
                    if (k == 0)
                    {
                        box.index(k + j * 2 + i * 4);
                        box.index(k + 1 + j * 2 + i * 4);
                    }
                }
            }
        }
    }

    void createBird() 
    {
        addCone(bird);
        bird.generateNormals();
        bird.update();

        buffer.bufferType(GL_ARRAY_BUFFER);
        buffer.usage(GL_DYNAMIC_DRAW);

        buffer.create();

        for (int i = 0; i < 4; i++) {
            rotateBuffer[i].bufferType(GL_ARRAY_BUFFER);
            rotateBuffer[i].usage(GL_DYNAMIC_DRAW);

            rotateBuffer[i].create();
        }
        
        colorBuffer.bufferType(GL_ARRAY_BUFFER);
        colorBuffer.usage(GL_DYNAMIC_DRAW);
        colorBuffer.create();

        shader_instancing.compile(instancing_vert, instancing_frag);

        auto& vao = bird.vao();
        vao.bind();
        vao.enableAttrib(4);
        vao.attribPointer(4, buffer, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glVertexAttribDivisor(4, 1);  // should be called with target vao bound
        vao.enableAttrib(5);
        vao.attribPointer(5, rotateBuffer[0], 4, GL_FLOAT, GL_FALSE, 0, 0);
        vao.enableAttrib(6);
        vao.attribPointer(6, rotateBuffer[1], 4, GL_FLOAT, GL_FALSE, 0, 0);
        vao.enableAttrib(7);
        vao.attribPointer(7, rotateBuffer[2], 4, GL_FLOAT, GL_FALSE, 0, 0);
        vao.enableAttrib(8);
        vao.attribPointer(8, rotateBuffer[3], 4, GL_FLOAT, GL_FALSE, 0, 0);
        glVertexAttribDivisor(5, 1);
        glVertexAttribDivisor(6, 1);
        glVertexAttribDivisor(7, 1);
        glVertexAttribDivisor(8, 1);
        vao.enableAttrib(9);
        vao.attribPointer(9, colorBuffer, 4, GL_FLOAT, GL_FALSE, 0, 0);
        glVertexAttribDivisor(9, 1);
    }

    void onInit() override
    {
        auto GUIdomain = GUIDomain::enableGUI(defaultWindowDomain());
        auto& gui = GUIdomain->newGUI();
        gui.add(boxBound);
        gui.add(sight);
        gui.add(speedLimit);
        gui.add(targetRadius);
        gui.add(foodAppeal);
        gui.add(foodspeed);
    }

    void onCreate()
    {
        for (int i = 0; i < instanceNum; i++) {
            Boid newBoid;
            boids.push_back(newBoid);
        }
        createBird();
        createbox();
        nav().pullBack(4);

        resetBoids();
        nav().pos(Vec3f(0, 15, 0));
        light.lightColor = Vec3f(1.0, 0.7, 0.4);
        light.lightPos = Vec3f(5, 5, 0);
        targetPos = Vec3f(targetRadius * cos(targetTheta), 0, targetRadius * sin(targetTheta));
    }

    // Randomize boid positions/velocities uniformly inside unit disc
    void resetBoids()
    {
        for (auto &b : boids)
        {
            b.position = rnd::ball<Vec3f>();
            b.velocity = rnd::ball<Vec3f>();
        }
    }

    void onAnimate(double dt_ms)
    {
        float dt = dt_ms;
        targetTheta += dt * foodspeed.get();
        if (targetTheta > 2 * M_PI) {
            targetTheta -= 2 * M_PI;
        }
        targetPos = Vec3f(targetRadius * cos(targetTheta), 0, targetRadius * sin(targetTheta));
        // Compute boid-boid interactions
        for (int i = 0; i < instanceNum - 1; ++i)
        {
            for (int j = i + 1; j < instanceNum; ++j)
            {
                // printf("checking boids %d and %d\n", i,j);

                auto ds = boids[i].position - boids[j].position;
                auto dist = ds.mag();

                // Collision avoidance
                float pushRadius = 0.05;
                float pushStrength = 1;
                float push = exp(-al::pow2(dist / pushRadius)) * pushStrength;

                auto pushVector = ds.normalized() * push;
                boids[i].velocity += pushVector;
                boids[j].velocity -= pushVector;

                // Velocity matching
                float matchRadius = 0.125;
                float nearness = exp(-al::pow2(dist / matchRadius));
                Vec3f veli = boids[i].velocity;
                Vec3f velj = boids[j].velocity;

                // Take a weighted average of velocities according to nearness
                boids[i].velocity = veli * (1 - 0.5 * nearness) + velj * (0.5 * nearness);
                boids[j].velocity = velj * (1 - 0.5 * nearness) + veli * (0.5 * nearness);

                // TODO: Flock centering
                if (dist < sight.get()) {
                    boids[i].sightPos += boids[j].position;
                    boids[j].sightPos += boids[i].position;
                    boids[i].sightWeight ++;
                    boids[j].sightWeight ++;
                }
            }
        }
        // Update boid independent behaviors
        vector<Vec3f> positions;
        vector<Vec4f> rotates[4];
        vector<Color> colors;
        for (auto &b : boids)
        {
            // Random "hunting" motion
            float huntUrge = 0.2;
            auto hunt = rnd::ball<Vec3f>();
            // Use cubed distribution to make small jumps more frequent
            hunt *= hunt.magSqr();
            b.velocity += hunt * huntUrge;

            Vec3f bound = boxBound.get();
            // Bound boid into a box
            if (b.position.x > bound.x || b.position.x < -bound.x)
            {
                b.position.x = b.position.x > 0 ? bound.x : -bound.x;
                b.velocity.x = -b.velocity.x;
            }
            if (b.position.y > bound.y || b.position.y < -bound.y)
            {
                b.position.y = b.position.y > 0 ? bound.y : -bound.y;
                b.velocity.y = -b.velocity.y;
            }
            if (b.position.z > bound.z || b.position.z < -bound.z)
            {
                b.position.z = b.position.z > 0 ? bound.z : -bound.z;
                b.velocity.z = -b.velocity.z;
            }
            auto ds = -b.position + (b.sightPos / b.sightWeight);
            auto dist = ds.mag();
            auto targetDir = targetPos - b.position;
            auto centerV = ds.normalize() * b.velocity.mag() * b.sightWeight / instanceNum + 
                            targetDir.normalized() / pow(targetDir.mag(), 2) * foodAppeal.get();
            b.velocity += centerV;
            float rate = 1.0f;
            if (b.velocity.mag() > speedLimit.get()) {
                rate = speedLimit.get() / b.velocity.mag();
                b.velocity *= rate;
            }
            colors.push_back(Color(0.5, (float)b.sightWeight / 40.0, (float)b.sightWeight / 20.0, 1.0f));
            b.sightPos = Vec3f(0, 0, 0);
            b.sightWeight = 0;
            b.update(dt);
            b.velocity -= centerV * rate;
            positions.push_back(b.position);
            auto q = Quatf::rotor(Vec3f(0.0f, 0.0f, 1.0f), b.velocity.normalized());
            Matrix4f rotateM;
            q.toMatrix(rotateM.elems());
            for (int i = 0; i < 4; i++) {
                rotates[i].push_back(Vec4f(rotateM[i * 4], rotateM[i * 4 + 1],
                                            rotateM[i * 4 + 2], rotateM[i * 4 + 3]));
            }
        }
        buffer.bind();
        buffer.data(boids.size() * 3 * sizeof(float), positions.data());
        for (int i = 0; i < 4; i++) {
            rotateBuffer[i].bind();
            rotateBuffer[i].data(boids.size() * 4 * sizeof(float), rotates[i].data());
        }
        colorBuffer.bind();
        buffer.data(boids.size() * 4 * sizeof(float), colors.data());
        nav().faceToward(Vec3f(0, 0, 0));
    }

    void onDraw(Graphics &g)
    {
        g.clear(0);
        g.depthTesting(true);
        g.shader(shader_instancing);
        g.shader().uniform("scale", 0.05f);
        g.shader().uniform("lightColor", light.lightColor);
        g.shader().uniform("lightPos", light.lightPos);
        g.shader().uniform("viewPos", nav().pos());
        g.update();
        bird.vao().bind();
        if (bird.indices().size()) {
            // when using index buffer, remember to bind it too
            bird.indexBuffer().bind();
            glDrawElementsInstanced(GL_TRIANGLES, bird.indices().size(),
                                    GL_UNSIGNED_INT, 0, boids.size());
        } else {
            glDrawArraysInstanced(GL_TRIANGLES, 0, bird.vertices().size(),
                                boids.size());
        }
        // g.stroke(1);
        g.color(1);
        g.pushMatrix();
        g.scale(boxBound.get() * 2.0f);
        g.draw(box);
        g.popMatrix();

        g.color(1);
        g.pushMatrix();
        g.translate(targetPos);
        g.scale(0.2f);
        g.draw(box);
        g.popMatrix();
    }

    bool onKeyDown(const Keyboard &k)
    {
        switch (k.key())
        {
        case 'r':
            resetBoids();
            break;
        case 'f':
            addFood();
            break;
        }
        return true;
    }
    
    void addFood() 
    {

    }
};

int main() { MyApp().start(); }
