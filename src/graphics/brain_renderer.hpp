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
    uint8_t neuropil_id; // Anatomical neuropil partition
};

struct ProjectedPoint {
    int sx, sy;
    float inv_z;
    bool valid;
};

struct NeuropilCentroid {
    NeuropilID id;
    const char* name;
    float x, y, z;
    uint32_t color;
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
     * @param stride Screen framebuffer row width in pixels (e.g. 1280)
     * @param height Screen framebuffer total height (e.g. 720)
     * @param engine Reference to active simulation engine
     * @param vp_x Viewport X offset (e.g. 640 for right half split-screen)
     * @param vp_y Viewport Y offset (e.g. 0)
     * @param vp_w Viewport width (e.g. 640)
     * @param vp_h Viewport height (e.g. 680)
     * @param cognitive_phase Current fly thought stage (1=Scan, 2=Walk, 3=Inspect, 4=Actuate)
     */
    void renderSoftware(uint32_t* framebuffer, int stride, int height, const LIFEngine& engine,
                        int vp_x = 0, int vp_y = 0, int vp_w = 1280, int vp_h = 720,
                        int cognitive_phase = 0);

    float getCameraYaw() const { return m_yaw; }
    float getCameraPitch() const { return m_pitch; }
    float getCameraDistance() const { return m_distance; }

private:
    std::vector<BrainVertex> m_vertices;
    std::vector<ProjectedPoint> m_projectedPoints;
    std::vector<NeuropilCentroid> m_centroids;
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

    // Fast rasterization line helper with viewport clipping
    static void drawLineBlended(uint32_t* fb, int stride, int height, int x0, int y0, int x1, int y1,
                                uint8_t r, uint8_t g, uint8_t b, float alpha,
                                int min_x, int min_y, int max_x, int max_y);
};

} // namespace flybrain
