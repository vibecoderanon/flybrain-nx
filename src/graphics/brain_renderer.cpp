#include "brain_renderer.hpp"
#include <cmath>
#include <algorithm>
#include <cstring>

namespace flybrain {

BrainRenderer::BrainRenderer() = default;
BrainRenderer::~BrainRenderer() = default;

void BrainRenderer::getNeuropilColor(NeuropilID id, float& r, float& g, float& b) {
    switch (id) {
        case NeuropilID::OpticLobe:
            r = 0.05f; g = 0.85f; b = 0.95f; // Cyan
            break;
        case NeuropilID::CentralComplex:
            r = 1.00f; g = 0.78f; b = 0.15f; // Amber / Gold
            break;
        case NeuropilID::MushroomBody:
            r = 0.95f; g = 0.20f; b = 0.75f; // Magenta
            break;
        case NeuropilID::AntennalLobe:
            r = 0.20f; g = 0.95f; b = 0.35f; // Emerald Green
            break;
        case NeuropilID::SubesophagealZone:
            r = 0.40f; g = 0.50f; b = 1.00f; // Periwinkle Blue
            break;
        case NeuropilID::DescendingMotor:
            r = 1.00f; g = 0.35f; b = 0.10f; // Flame Orange
            break;
        case NeuropilID::AscendingSensory:
            r = 1.00f; g = 0.95f; b = 0.20f; // Bright Yellow
            break;
        default:
            r = 0.35f; g = 0.40f; b = 0.50f; // Slate Gray
            break;
    }
}

bool BrainRenderer::init(const ConnectomeLoader& loader, int screen_width, int screen_height) {
    m_screenWidth = screen_width;
    m_screenHeight = screen_height;

    uint32_t count = loader.getNeuronCount();
    const NeuronRecord* neurons = loader.getNeurons();
    if (count == 0 || !neurons) return false;

    m_vertices.resize(count);
    for (uint32_t i = 0; i < count; ++i) {
        BrainVertex& v = m_vertices[i];
        v.x = neurons[i].x;
        v.y = neurons[i].y;
        v.z = neurons[i].z;
        v.neuron_idx = i;
        getNeuropilColor(static_cast<NeuropilID>(neurons[i].neuropil_id), v.r, v.g, v.b);
    }

    resetCamera();
    return true;
}

void BrainRenderer::updateCamera(float d_yaw, float d_pitch, float d_zoom) {
    m_yaw += d_yaw;
    m_pitch = std::clamp(m_pitch + d_pitch, -1.50f, 1.50f);
    m_distance = std::clamp(m_distance + d_zoom, 150.0f, 1800.0f);
}

void BrainRenderer::resetCamera() {
    m_yaw = 0.0f;
    m_pitch = 0.25f;
    m_distance = 650.0f;
    m_targetX = 0.0f;
    m_targetY = -30.0f;
    m_targetZ = 0.0f;
}

void BrainRenderer::renderSoftware(uint32_t* framebuffer, int width, int height, const LIFEngine& engine) {
    if (!framebuffer || m_vertices.empty() || width <= 0 || height <= 0) return;

    // Clear background to dark obsidian navy (#0b0f19)
    const uint32_t bg_color = 0xFF0B0F19;
    std::fill_n(framebuffer, width * height, bg_color);

    const float cos_y = std::cos(m_yaw);
    const float sin_y = std::sin(m_yaw);
    const float cos_p = std::cos(m_pitch);
    const float sin_p = std::sin(m_pitch);

    const float fov_factor = static_cast<float>(height) * 0.90f;
    const float half_w = width * 0.5f;
    const float half_h = height * 0.5f;

    const NeuronState* states = engine.getNeuronStates();
    const uint32_t neuron_count = engine.getNeuronCount();

    for (const auto& v : m_vertices) {
        // Model to Camera coordinates (relative to target center)
        float dx = v.x - m_targetX;
        float dy = v.y - m_targetY;
        float dz = v.z - m_targetZ;

        // Yaw rotation around Y axis
        float rx = dx * cos_y - dz * sin_y;
        float rz = dx * sin_y + dz * cos_y;

        // Pitch rotation around X axis
        float ry = dy * cos_p - rz * sin_p;
        float cam_z = dy * sin_p + rz * cos_p + m_distance;

        // Near-plane clipping
        if (cam_z < 30.0f) continue;

        // Perspective projection
        float inv_z = 1.0f / cam_z;
        int sx = static_cast<int>(half_w + (rx * fov_factor * inv_z));
        int sy = static_cast<int>(half_h - (ry * fov_factor * inv_z));

        // Screen boundary check
        if (sx < 1 || sx >= width - 1 || sy < 1 || sy >= height - 1) continue;

        // Fetch dynamic spike luminance
        float glow = 0.0f;
        if (v.neuron_idx < neuron_count && states) {
            glow = states[v.neuron_idx].spike_luminance;
        }

        // Color interpolation: base neuropil color blending towards incandescent white on spike
        float r_f = std::clamp((v.r * (1.0f - glow) + 1.0f * glow) * 255.0f, 0.0f, 255.0f);
        float g_f = std::clamp((v.g * (1.0f - glow) + 0.95f * glow) * 255.0f, 0.0f, 255.0f);
        float b_f = std::clamp((v.b * (1.0f - glow) + 0.85f * glow) * 255.0f, 0.0f, 255.0f);

        uint8_t r = static_cast<uint8_t>(r_f);
        uint8_t g = static_cast<uint8_t>(g_f);
        uint8_t b = static_cast<uint8_t>(b_f);

        uint32_t point_color = (0xFF << 24) | (b << 16) | (g << 8) | r;

        // Plot point (single pixel for distant, 3x3 diamond if glowing spike)
        if (glow > 0.3f) {
            // Draw glowing halo
            framebuffer[sy * width + sx] = point_color;
            framebuffer[(sy - 1) * width + sx] = point_color;
            framebuffer[(sy + 1) * width + sx] = point_color;
            framebuffer[sy * width + (sx - 1)] = point_color;
            framebuffer[sy * width + (sx + 1)] = point_color;
        } else {
            framebuffer[sy * width + sx] = point_color;
        }
    }
}

} // namespace flybrain
