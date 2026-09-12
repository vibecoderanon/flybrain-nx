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

struct ProjectedPoint {
    int sx, sy;
    float inv_z;
    bool valid;
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

    void toggleAxonLines() { m_showAxonLines = !m_showAxonLines; }
    bool areAxonLinesEnabled() const { return m_showAxonLines; }

    /**
     * @brief Render the 3D point cloud & active synaptic transmission lines
     * @param framebuffer Pointer to width * height * 4 RGBA bytes
     * @param engine Reference to active simulation engine
     */
    void renderSoftware(uint32_t* framebuffer, int width, int height, const LIFEngine& engine);

    float getCameraYaw() const { return m_yaw; }
    float getCameraPitch() const { return m_pitch; }
    float getCameraDistance() const { return m_distance; }

private:
    std::vector<BrainVertex> m_vertices;
    std::vector<ProjectedPoint> m_projectedPoints;
    int m_screenWidth = 1280;
    int m_screenHeight = 720;
    bool m_showAxonLines = true;

    // Orbit Camera Parameters
    float m_yaw = 0.0f;
    float m_pitch = 0.25f;
    float m_distance = 650.0f;
    float m_targetX = 0.0f;
    float m_targetY = -30.0f;
    float m_targetZ = 0.0f;

    // Color lookup helper
    static void getNeuropilColor(NeuropilID id, float& r, float& g, float& b);

    // Fast rasterization line helper
    static void drawLineBlended(uint32_t* fb, int width, int height, int x0, int y0, int x1, int y1, uint8_t r, uint8_t g, uint8_t b, float alpha);
};

} // namespace flybrain
