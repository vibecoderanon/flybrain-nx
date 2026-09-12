#pragma once

#include "../connectome/connectome_format.hpp"
#include "../simulation/lif_engine.hpp"
#include <vector>
#include <cstdint>

namespace flybrain {

struct BrainVertex {
    float x, y, z;       // Model space coordinates
    float r, g, b;       // Base neuropil color
    uint32_t neuron_idx; // Mapping back to simulation state
};

class BrainRenderer {
public:
    BrainRenderer();
    ~BrainRenderer();

    /**
     * @brief Initialize vertex data and colors from loaded connectome
     */
    bool init(const ConnectomeLoader& loader, int screen_width = 1280, int screen_height = 720);

    /**
     * @brief Update camera controls from input
     * @param d_yaw Yaw delta in radians
     * @param d_pitch Pitch delta in radians
     * @param d_zoom Zoom distance delta
     */
    void updateCamera(float d_yaw, float d_pitch, float d_zoom);
    void resetCamera();

    /**
     * @brief Render the 3D point cloud into a 32-bit RGBA framebuffer (1280x720)
     * @param framebuffer Pointer to width * height * 4 RGBA bytes
     * @param engine Reference to active simulation engine for dynamic spike luminance
     */
    void renderSoftware(uint32_t* framebuffer, int width, int height, const LIFEngine& engine);

    float getCameraYaw() const { return m_yaw; }
    float getCameraPitch() const { return m_pitch; }
    float getCameraDistance() const { return m_distance; }

private:
    std::vector<BrainVertex> m_vertices;
    int m_screenWidth = 1280;
    int m_screenHeight = 720;

    // Orbit Camera Parameters
    float m_yaw = 0.0f;
    float m_pitch = 0.25f;
    float m_distance = 650.0f;
    float m_targetX = 0.0f;
    float m_targetY = -30.0f;
    float m_targetZ = 0.0f;

    // Color lookup helper
    static void getNeuropilColor(NeuropilID id, float& r, float& g, float& b);
};

} // namespace flybrain
