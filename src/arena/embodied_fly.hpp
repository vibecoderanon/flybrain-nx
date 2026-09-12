#pragma once

#include "../simulation/lif_engine.hpp"

namespace flybrain {

struct FlyBody {
    float x = 0.0f;             // Arena X coordinate (-500 to 500)
    float y = 0.0f;             // Arena Y coordinate (-500 to 500)
    float heading = 0.0f;       // Radians (0 = East, pi/2 = North)
    float speed = 0.0f;         // mm/s
    bool is_flying = false;
    bool is_grooming = false;
};

struct FoodPellet {
    float x;
    float y;
    float radius;
    bool consumed;
};

class EmbodiedFlyArena {
public:
    EmbodiedFlyArena();
    ~EmbodiedFlyArena();

    void init();

    /**
     * @brief Step the closed-loop virtual agent physics and feed sensory cues into the connectome
     */
    void update(LIFEngine& engine, float dt_sec);

    const FlyBody& getFly() const { return m_fly; }
    const std::vector<FoodPellet>& getFood() const { return m_food; }

    void addFood(float x, float y);
    void triggerPredatorSwatter(float x, float y);

private:
    FlyBody m_fly{};
    std::vector<FoodPellet> m_food;
    float m_arenaRadius = 450.0f;
};

} // namespace flybrain
