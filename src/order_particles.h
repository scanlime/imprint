/*
 * Complex particle system.
 * Basic rules give order to things.
 * Sometimes too rigid, like a crystal.
 *
 * (c) 2014 Micah Elizabeth Scott
 * http://creativecommons.org/licenses/by/3.0/
 */

#pragma once

#include <vector>
#include "lib/particle.h"
#include "lib/prng.h"
#include "lib/texture.h"


class OrderParticles : public ParticleEffect
{
public:
    OrderParticles();
    void reseed(unsigned seed);

    virtual void beginFrame(const FrameInfo &f);

    float vibration;
    int symmetry;

private:
    static const unsigned numParticles = 60;
    static const float relativeSize = 0.08;
    // static const unsigned numParticles = 500;
    // static const float relativeSize = 0.04;
    static const float intensity = 0.4;
    static const float stepSize = 1.0 / 500;
    static const float seedRadius = 1.0;
    static const float interactionSize = 0.2;
    static const float angleGain = 0.01;

    float timeDeltaRemainder;

    void runStep(const FrameInfo &f);
};


/*****************************************************************************************
 *                                   Implementation
 *****************************************************************************************/


inline OrderParticles::OrderParticles()
    : timeDeltaRemainder(0)
{
    reseed(42);
    vibration = 0;
    symmetry = 3;
}

inline void OrderParticles::reseed(unsigned seed)
{
    appearance.resize(numParticles);

    PRNG prng;
    prng.seed(seed);

    for (unsigned i = 0; i < appearance.size(); i++) {
        Vec2 p = prng.ringVector(1e-4, seedRadius);
        appearance[i].point = Vec3(p[0], 0, p[1]);
    }
}

inline void OrderParticles::beginFrame(const FrameInfo &f)
{    
    float t = f.timeDelta + timeDeltaRemainder;
    int steps = t / stepSize;
    timeDeltaRemainder = t - steps * stepSize;

    while (steps > 0) {
        runStep(f);
        steps--;
    }
}

inline void OrderParticles::runStep(const FrameInfo &f)
{
    PRNG prng;
    prng.seed(20);

    // Update dynamics
    for (unsigned i = 0; i < appearance.size(); i++) {

        float fade = 1.0;

        appearance[i].intensity = intensity * fade;
        appearance[i].radius = f.modelDiameter * relativeSize * fade;
        appearance[i].color = Vec3(1,1,1);

        prng.remix(appearance[i].point[0] * 1e8);
        prng.remix(appearance[i].point[2] * 1e8);

        // Vibration
        Vec2 v = prng.ringVector(0.5, 1) * vibration;
        appearance[i].point += Vec3(v[0], 0, v[1]);
    }

    // Rebuild index
    ParticleEffect::beginFrame(f);

    // Particle interactions
    for (unsigned i = 0; i < appearance.size(); i++) {

        Vec3 &p = appearance[i].point;
        std::vector<std::pair<size_t, Real> > hits;
        nanoflann::SearchParams params;
        params.sorted = false;

        float searchRadius2 = sq(interactionSize * f.modelDiameter);
        unsigned numHits = index.tree.radiusSearch(&p[0], searchRadius2, hits, params);

        for (unsigned i = 0; i < numHits; i++) {
            ParticleAppearance &hit = appearance[hits[i].first];
            float q2 = hits[i].second / searchRadius2;
            if (q2 < 1.0f) {
                // These particles influence each other
                Vec3 d = hit.point - p;

                // Angular 'snap' force, operates at a distance
                float angle = atan2(d[2], d[0]);
                const float angleIncrement = 2 * M_PI / symmetry;
                float snapAngle = roundf(angle / angleIncrement) * angleIncrement;
                float angleDelta = fabsf(snapAngle - angle);

                // Spin perpendicular to 'd'
                Vec3 da = (angleGain * angleDelta / (1.0f + q2)) * Vec3( d[2], 0, -d[0] );
                p += da;
                hit.point -= da;
            }
        }
    }
}