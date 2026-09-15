#include "brain_renderer.hpp"
#include "../ui/draw_utils.hpp"
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

    struct Acc { float sum_x = 0; float sum_y = 0; float sum_z = 0; uint32_t count = 0; };
    Acc acc[9]{};

    for (uint32_t i = 0; i < count; ++i) {
        BrainVertex& v = m_vertices[i];
        v.x = neurons[i].x;
        v.y = neurons[i].y;
        v.z = neurons[i].z;
        v.neuron_idx = i;
        v.neuropil_id = neurons[i].neuropil_id;
        getNeuropilColor(static_cast<NeuropilID>(neurons[i].neuropil_id), v.r, v.g, v.b);

        uint8_t nid = neurons[i].neuropil_id;
        if (nid < 9) {
            acc[nid].sum_x += neurons[i].x;
            acc[nid].sum_y += neurons[i].y;
            acc[nid].sum_z += neurons[i].z;
            acc[nid].count++;
        }
    }

    m_centroids.clear();
    auto addCentroid = [&](NeuropilID id, const char* name, uint32_t col) {
        uint8_t nid = static_cast<uint8_t>(id);
        if (nid < 9 && acc[nid].count > 0) {
            float inv = 1.0f / static_cast<float>(acc[nid].count);
            m_centroids.push_back({id, name, acc[nid].sum_x * inv, acc[nid].sum_y * inv, acc[nid].sum_z * inv, col});
        }
    };

    addCentroid(NeuropilID::OpticLobe, "[OPTIC LOBES]", 0xFF06B6D4);
    addCentroid(NeuropilID::CentralComplex, "[CENTRAL COMPLEX]", 0xFFF59E0B);
    addCentroid(NeuropilID::MushroomBody, "[MUSHROOM BODY]", 0xFFF43F5E);
    addCentroid(NeuropilID::SubesophagealZone, "[MOTOR SEZ]", 0xFFF97316);

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

void BrainRenderer::drawLineBlended(uint32_t* fb, int stride, int height, int x0, int y0, int x1, int y1,
                                    uint8_t r, uint8_t g, uint8_t b, float alpha,
                                    int min_x, int min_y, int max_x, int max_y) {
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
        if (x0 >= min_x && x0 < max_x && y0 >= min_y && y0 < max_y && y0 < height) {
            uint32_t dst = fb[y0 * stride + x0];
            uint32_t dr = (dst & 0xFF);
            uint32_t dg = ((dst >> 8) & 0xFF);
            uint32_t db = ((dst >> 16) & 0xFF);

            uint8_t out_r = static_cast<uint8_t>(std::min(255u, src_r + static_cast<uint32_t>(dr * inv_a)));
            uint8_t out_g = static_cast<uint8_t>(std::min(255u, src_g + static_cast<uint32_t>(dg * inv_a)));
            uint8_t out_b = static_cast<uint8_t>(std::min(255u, src_b + static_cast<uint32_t>(db * inv_a)));

            fb[y0 * stride + x0] = (0xFF << 24) | (out_b << 16) | (out_g << 8) | out_r;
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

void BrainRenderer::renderSoftware(uint32_t* framebuffer, int stride, int height, const LIFEngine& engine,
                                   int vp_x, int vp_y, int vp_w, int vp_h,
                                   int cognitive_phase) {
    if (!framebuffer || m_vertices.empty() || stride <= 0 || height <= 0 || vp_w <= 0 || vp_h <= 0) return;

    int max_x = std::min(stride, vp_x + vp_w);
    int max_y = std::min(height, vp_y + vp_h);

    // Clear background of this viewport to obsidian navy (#0b0f19)
    const uint32_t bg_color = 0xFF0B0F19;
    for (int y = vp_y; y < max_y; ++y) {
        std::fill_n(&framebuffer[y * stride + vp_x], max_x - vp_x, bg_color);
    }

    const float cos_y = std::cos(m_yaw);
    const float sin_y = std::sin(m_yaw);
    const float cos_p = std::cos(m_pitch);
    const float sin_p = std::sin(m_pitch);

    const float fov_factor = static_cast<float>(vp_h) * 0.90f;
    const float half_w = static_cast<float>(vp_x) + static_cast<float>(vp_w) * 0.5f;
    const float half_h = static_cast<float>(vp_y) + static_cast<float>(vp_h) * 0.5f;

    const NeuronState* states = engine.getNeuronStates();
    const uint32_t neuron_count = engine.getNeuronCount();
    const uint32_t focus_neuron = engine.getPicrossFocusNeuron();

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

        if (sx < vp_x || sx >= max_x || sy < vp_y || sy >= max_y) {
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

            drawLineBlended(framebuffer, stride, height, p0.sx, p0.sy, p1.sx, p1.sy,
                            ur, ug, ub, alpha, vp_x, vp_y, max_x, max_y);
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

        // Deliberation pathway spotlighting
        if (cognitive_phase == 1 && v.neuropil_id == static_cast<uint8_t>(NeuropilID::OpticLobe)) {
            glow = std::max(glow, 0.70f);
        } else if (cognitive_phase == 2 && v.neuropil_id == static_cast<uint8_t>(NeuropilID::CentralComplex)) {
            glow = std::max(glow, 0.75f);
        } else if (cognitive_phase == 3 && v.neuropil_id == static_cast<uint8_t>(NeuropilID::MushroomBody)) {
            glow = std::max(glow, 0.80f);
        } else if (cognitive_phase == 4 && (v.neuropil_id == static_cast<uint8_t>(NeuropilID::SubesophagealZone) ||
                                            v.neuropil_id == static_cast<uint8_t>(NeuropilID::DescendingMotor))) {
            glow = std::max(glow, 0.85f);
        }

        bool is_focus = (v.neuron_idx == focus_neuron);
        if (is_focus) {
            glow = std::max(glow, 0.85f);
        }

        // Color blending: base neuropil color -> brilliant incandescent white on spike
        float r_f = std::clamp((v.r * (1.0f - glow) + 1.0f * glow) * 255.0f, 0.0f, 255.0f);
        float g_f = std::clamp((v.g * (1.0f - glow) + 0.96f * glow) * 255.0f, 0.0f, 255.0f);
        float b_f = std::clamp((v.b * (1.0f - glow) + 0.88f * glow) * 255.0f, 0.0f, 255.0f);

        if (is_focus) {
            r_f = 255.0f; g_f = 215.0f; b_f = 0.0f; // Radiant Gold
        }

        uint8_t r = static_cast<uint8_t>(r_f);
        uint8_t g = static_cast<uint8_t>(g_f);
        uint8_t b = static_cast<uint8_t>(b_f);
        uint32_t point_color = (0xFF << 24) | (b << 16) | (g << 8) | r;

        int sx = pt.sx;
        int sy = pt.sy;

        if (glow > 0.25f || is_focus) {
            // Glowing diamond halo for firing action potential
            framebuffer[sy * stride + sx] = 0xFFFFFFFF; // Incandescent center
            if (sx > vp_x) framebuffer[sy * stride + (sx - 1)] = point_color;
            if (sx + 1 < max_x) framebuffer[sy * stride + (sx + 1)] = point_color;
            if (sy > vp_y) framebuffer[(sy - 1) * stride + sx] = point_color;
            if (sy + 1 < max_y) framebuffer[(sy + 1) * stride + sx] = point_color;

            if (is_focus) {
                // Outer ring for focus neuron
                if (sx > vp_x + 1) framebuffer[sy * stride + (sx - 2)] = 0xFFF59E0B;
                if (sx + 2 < max_x) framebuffer[sy * stride + (sx + 2)] = 0xFFF59E0B;
                if (sy > vp_y + 1) framebuffer[(sy - 2) * stride + sx] = 0xFFF59E0B;
                if (sy + 2 < max_y) framebuffer[(sy + 2) * stride + sx] = 0xFFF59E0B;
            }
        } else {
            framebuffer[sy * stride + sx] = point_color;
        }
    }

    // 4. Render 3D Anatomical Neuropil Badges
    for (const auto& c : m_centroids) {
        float dx = c.x - m_targetX;
        float dy = c.y - m_targetY;
        float dz = c.z - m_targetZ;

        float rx = dx * cos_y - dz * sin_y;
        float rz = dx * sin_y + dz * cos_y;
        float ry = dy * cos_p - rz * sin_p;
        float cam_z = dy * sin_p + rz * cos_p + m_distance;

        if (cam_z < 50.0f) continue;

        float inv_z = 1.0f / cam_z;
        int sx = static_cast<int>(half_w + (rx * fov_factor * inv_z));
        int sy = static_cast<int>(half_h - (ry * fov_factor * inv_z));

        // Display labels neatly within viewport bounds
        if (sx >= vp_x + 15 && sx <= max_x - 130 && sy >= vp_y + 70 && sy <= max_y - 20) {
            DrawUtils::drawCircleFilled(framebuffer, stride, height, sx, sy, 3, c.color);
            DrawUtils::drawString(framebuffer, stride, height, sx + 6, sy - 4, c.name, c.color, 1);
        }
    }
}

} // namespace flybrain
