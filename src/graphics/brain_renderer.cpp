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
            r = 0.05f; g = 0.85f; b = 0.95f; // Electric Cyan
            break;
        case NeuropilID::CentralComplex:
            r = 1.00f; g = 0.78f; b = 0.15f; // Solar Gold / Amber
            break;
        case NeuropilID::MushroomBody:
            r = 0.95f; g = 0.20f; b = 0.75f; // Neon Magenta
            break;
        case NeuropilID::AntennalLobe:
            r = 0.20f; g = 0.95f; b = 0.35f; // Emerald Green
            break;
        case NeuropilID::SubesophagealZone:
            r = 0.45f; g = 0.40f; b = 1.00f; // Lavender Blue
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
    m_projectedPoints.resize(count);

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

void BrainRenderer::drawLineBlended(uint32_t* fb, int width, int height, int x0, int y0, int x1, int y1, uint8_t r, uint8_t g, uint8_t b, float alpha) {
    int dx = std::abs(x1 - x0);
    int dy = std::abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    float inv_a = 1.0f - alpha;
    uint32_t src_r = static_cast<uint32_t>(r * alpha);
    uint32_t src_g = static_cast<uint32_t>(g * alpha);
    uint32_t src_b = static_cast<uint32_t>(b * alpha);

    while (true) {
        if (x0 >= 0 && x0 < width && y0 >= 0 && y0 < height) {
            uint32_t dst = fb[y0 * width + x0];
            uint32_t dr = (dst & 0xFF);
            uint32_t dg = ((dst >> 8) & 0xFF);
            uint32_t db = ((dst >> 16) & 0xFF);

            uint8_t out_r = static_cast<uint8_t>(std::min(255u, src_r + static_cast<uint32_t>(dr * inv_a)));
            uint8_t out_g = static_cast<uint8_t>(std::min(255u, src_g + static_cast<uint32_t>(dg * inv_a)));
            uint8_t out_b = static_cast<uint8_t>(std::min(255u, src_b + static_cast<uint32_t>(db * inv_a)));

            fb[y0 * width + x0] = (0xFF << 24) | (out_b << 16) | (out_g << 8) | out_r;
        }

        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
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

    // 1. Perspective Project all neurons to screen space & cache
    for (size_t i = 0; i < m_vertices.size(); ++i) {
        const auto& v = m_vertices[i];
        ProjectedPoint& pt = m_projectedPoints[i];

        float dx = v.x - m_targetX;
        float dy = v.y - m_targetY;
        float dz = v.z - m_targetZ;

        float rx = dx * cos_y - dz * sin_y;
        float rz = dx * sin_y + dz * cos_y;

        float ry = dy * cos_p - rz * sin_p;
        float cam_z = dy * sin_p + rz * cos_p + m_distance;

        if (cam_z < 30.0f) {
            pt.valid = false;
            continue;
        }

        float inv_z = 1.0f / cam_z;
        int sx = static_cast<int>(half_w + (rx * fov_factor * inv_z));
        int sy = static_cast<int>(half_h - (ry * fov_factor * inv_z));

        if (sx < 0 || sx >= width || sy < 0 || sy >= height) {
            pt.valid = false;
            continue;
        }

        pt.sx = sx;
        pt.sy = sy;
        pt.inv_z = inv_z;
        pt.valid = true;
    }

    // 2. Render Active Synaptic Transmission Beams / Lines
    if (m_showAxonLines) {
        const auto& lines = engine.getActiveLines();
        for (const auto& l : lines) {
            if (l.src_idx >= m_projectedPoints.size() || l.dst_idx >= m_projectedPoints.size()) continue;

            const auto& p0 = m_projectedPoints[l.src_idx];
            const auto& p1 = m_projectedPoints[l.dst_idx];

            if (!p0.valid || !p1.valid) continue;

            float lr, lg, lb;
            getNeuropilColor(static_cast<NeuropilID>(l.neuropil_id), lr, lg, lb);

            // Lines pulse with bright core, scaled by line life intensity
            float alpha = std::clamp(l.intensity * 0.70f, 0.05f, 0.95f);
            uint8_t ur = static_cast<uint8_t>(std::clamp(lr * 255.0f, 0.0f, 255.0f));
            uint8_t ug = static_cast<uint8_t>(std::clamp(lg * 255.0f, 0.0f, 255.0f));
            uint8_t ub = static_cast<uint8_t>(std::clamp(lb * 255.0f, 0.0f, 255.0f));

            drawLineBlended(framebuffer, width, height, p0.sx, p0.sy, p1.sx, p1.sy, ur, ug, ub, alpha);
        }
    }

    // 3. Render 3D Point Cloud Nodes
    for (size_t i = 0; i < m_vertices.size(); ++i) {
        const auto& pt = m_projectedPoints[i];
        if (!pt.valid) continue;

        const auto& v = m_vertices[i];
        float glow = 0.0f;
        if (v.neuron_idx < neuron_count && states) {
            glow = states[v.neuron_idx].spike_luminance;
        }

        // Color blending: base neuropil color -> brilliant incandescent white on spike
        float r_f = std::clamp((v.r * (1.0f - glow) + 1.0f * glow) * 255.0f, 0.0f, 255.0f);
        float g_f = std::clamp((v.g * (1.0f - glow) + 0.96f * glow) * 255.0f, 0.0f, 255.0f);
        float b_f = std::clamp((v.b * (1.0f - glow) + 0.88f * glow) * 255.0f, 0.0f, 255.0f);

        uint8_t r = static_cast<uint8_t>(r_f);
        uint8_t g = static_cast<uint8_t>(g_f);
        uint8_t b = static_cast<uint8_t>(b_f);
        uint32_t point_color = (0xFF << 24) | (b << 16) | (g << 8) | r;

        int sx = pt.sx;
        int sy = pt.sy;

        if (glow > 0.25f) {
            // Glowing diamond halo for firing action potential
            framebuffer[sy * width + sx] = 0xFFFFFFFF; // Incandescent center
            if (sx > 0) framebuffer[sy * width + (sx - 1)] = point_color;
            if (sx + 1 < width) framebuffer[sy * width + (sx + 1)] = point_color;
            if (sy > 0) framebuffer[(sy - 1) * width + sx] = point_color;
            if (sy + 1 < height) framebuffer[(sy + 1) * width + sx] = point_color;
        } else {
            framebuffer[sy * width + sx] = point_color;
        }
    }
}

} // namespace flybrain
