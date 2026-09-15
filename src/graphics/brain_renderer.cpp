#include "brain_renderer.hpp"
#include "../ui/draw_utils.hpp"
#include <cmath>
#include <algorithm>
#include <cstring>
#include <array>

namespace flybrain {

namespace {
struct NeuropilAcc {
    float sum_x = 0.0f;
    float sum_y = 0.0f;
    float sum_z = 0.0f;
    uint32_t count = 0;
};
}

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

    std::array<NeuropilAcc, 9> acc{};
    NeuropilAcc acc_optic_left{};
    NeuropilAcc acc_optic_right{};

    for (uint32_t i = 0; i < count; ++i) {
        BrainVertex& v = m_vertices[i];
        v.x = neurons[i].x;
        v.y = neurons[i].y;
        v.z = neurons[i].z;
        v.neuron_idx = i;
        v.neuropil_id = neurons[i].neuropil_id;
        getNeuropilColor(static_cast<NeuropilID>(neurons[i].neuropil_id), v.r, v.g, v.b);

        uint8_t nid = neurons[i].neuropil_id;
        if (nid == static_cast<uint8_t>(NeuropilID::OpticLobe)) {
            if (neurons[i].x < 0.0f) {
                acc_optic_left.sum_x += neurons[i].x;
                acc_optic_left.sum_y += neurons[i].y;
                acc_optic_left.sum_z += neurons[i].z;
                acc_optic_left.count++;
            } else {
                acc_optic_right.sum_x += neurons[i].x;
                acc_optic_right.sum_y += neurons[i].y;
                acc_optic_right.sum_z += neurons[i].z;
                acc_optic_right.count++;
            }
        }
        if (nid < 9) {
            acc[nid].sum_x += neurons[i].x;
            acc[nid].sum_y += neurons[i].y;
            acc[nid].sum_z += neurons[i].z;
            acc[nid].count++;
        }
    }

    m_centroids.clear();

    // 1. Bilateral Optic Lobes (physically separated on X axis, preventing (0,0,0) centroid collision)
    if (acc_optic_left.count > 0) {
        float inv = 1.0f / static_cast<float>(acc_optic_left.count);
        m_centroids.emplace_back(NeuropilID::OpticLobe, "L. OPTIC LOBE", acc_optic_left.sum_x * inv, acc_optic_left.sum_y * inv, acc_optic_left.sum_z * inv, 0xFF06B6D4, true, false);
    }
    if (acc_optic_right.count > 0) {
        float inv = 1.0f / static_cast<float>(acc_optic_right.count);
        m_centroids.emplace_back(NeuropilID::OpticLobe, "R. OPTIC LOBE", acc_optic_right.sum_x * inv, acc_optic_right.sum_y * inv, acc_optic_right.sum_z * inv, 0xFF06B6D4, false, true);
    }

    auto addCentroid = [&](NeuropilID id, const char* name, uint32_t col) {
        uint8_t nid = static_cast<uint8_t>(id);
        if (nid < 9 && acc[nid].count > 0) {
            float inv = 1.0f / static_cast<float>(acc[nid].count);
            m_centroids.emplace_back(id, name, acc[nid].sum_x * inv, acc[nid].sum_y * inv, acc[nid].sum_z * inv, col, false, false);
        }
    };

    addCentroid(NeuropilID::CentralComplex, "CENTRAL COMPLEX", 0xFFF59E0B);
    addCentroid(NeuropilID::MushroomBody, "MUSHROOM BODY", 0xFFF43F5E);
    addCentroid(NeuropilID::SubesophagealZone, "MOTOR SEZ", 0xFFF97316);

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
                                   int cognitive_phase, float anim_time,
                                   int target_r, int target_c) {
    if (!framebuffer || m_vertices.empty() || stride <= 0 || height <= 0 || vp_w <= 0 || vp_h <= 0) return;

    m_animTime = anim_time;
    int max_x = std::min(stride, vp_x + vp_w);
    int max_y = std::min(height, vp_y + vp_h);

    // 1. Clear background of this viewport to obsidian navy (#0a0e17)
    const uint32_t bg_color = 0xFF0A0E17;
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

    // 2. Perspective Project all neurons to screen space & cache
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

    // 3. Render Active Synaptic Conduits & Traveling Energy Packets
    if (m_showAxonLines) {
        const auto& lines = engine.getActiveLines();
        for (const auto& l : lines) {
            if (l.src_idx >= m_projectedPoints.size() || l.dst_idx >= m_projectedPoints.size()) continue;

            const auto& p0 = m_projectedPoints[l.src_idx];
            const auto& p1 = m_projectedPoints[l.dst_idx];

            if (!p0.valid || !p1.valid) continue;

            float lr, lg, lb;
            getNeuropilColor(static_cast<NeuropilID>(l.neuropil_id), lr, lg, lb);

            // Baseline axonal conduit: subtle translucent line
            float line_alpha = std::clamp(l.intensity * (l.is_causal ? 0.45f : 0.22f), 0.05f, 0.65f);
            uint8_t ur = static_cast<uint8_t>(std::clamp(lr * 255.0f, 0.0f, 255.0f));
            uint8_t ug = static_cast<uint8_t>(std::clamp(lg * 255.0f, 0.0f, 255.0f));
            uint8_t ub = static_cast<uint8_t>(std::clamp(lb * 255.0f, 0.0f, 255.0f));

            drawLineBlended(framebuffer, stride, height, p0.sx, p0.sy, p1.sx, p1.sy,
                            ur, ug, ub, line_alpha, vp_x, vp_y, max_x, max_y);

            // Animated Action Potential Energy Packet traveling along the axon conduit
            float pr = std::clamp(l.progress, 0.0f, 1.0f);
            int ex = static_cast<int>(p0.sx + (p1.sx - p0.sx) * pr);
            int ey = static_cast<int>(p0.sy + (p1.sy - p0.sy) * pr);

            if (ex >= vp_x + 1 && ex < max_x - 1 && ey >= vp_y + 1 && ey < max_y - 1) {
                // Incandescent electric pulse head
                framebuffer[ey * stride + ex] = 0xFFFFFFFF;
                uint32_t glow_col = (0xFF << 24) | (ub << 16) | (ug << 8) | ur;
                framebuffer[ey * stride + (ex - 1)] = glow_col;
                framebuffer[ey * stride + (ex + 1)] = glow_col;
                framebuffer[(ey - 1) * stride + ex] = glow_col;
                framebuffer[(ey + 1) * stride + ex] = glow_col;

                if (l.is_causal) {
                    // Trailing impulse spark for functional thought conduits
                    float trail_pr = std::max(0.0f, pr - 0.12f);
                    int tx = static_cast<int>(p0.sx + (p1.sx - p0.sx) * trail_pr);
                    int ty = static_cast<int>(p0.sy + (p1.sy - p0.sy) * trail_pr);
                    if (tx >= vp_x && tx < max_x && ty >= vp_y && ty < max_y) {
                        framebuffer[ty * stride + tx] = glow_col;
                    }
                }
            }
        }
    }

    // 4. Render 3D Point Cloud Nodes with Two-Tier Visual Hierarchy
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
        int sx = pt.sx;
        int sy = pt.sy;

        if (is_focus) {
            // Prominent Focus / Target Tile Neuron: Radiant Gold Multi-Stage Orb
            framebuffer[sy * stride + sx] = 0xFFFFFFFF; // Pure white core
            const uint32_t gold_halo = 0xFFF59E0B;
            const uint32_t bright_gold = 0xFFFDE047;

            if (sx > vp_x) framebuffer[sy * stride + (sx - 1)] = bright_gold;
            if (sx + 1 < max_x) framebuffer[sy * stride + (sx + 1)] = bright_gold;
            if (sy > vp_y) framebuffer[(sy - 1) * stride + sx] = bright_gold;
            if (sy + 1 < max_y) framebuffer[(sy + 1) * stride + sx] = bright_gold;

            // Diamond halo
            if (sx > vp_x + 1) framebuffer[sy * stride + (sx - 2)] = gold_halo;
            if (sx + 2 < max_x) framebuffer[sy * stride + (sx + 2)] = gold_halo;
            if (sy > vp_y + 1) framebuffer[(sy - 2) * stride + sx] = gold_halo;
            if (sy + 2 < max_y) framebuffer[(sy + 2) * stride + sx] = gold_halo;

            // Animated Pulsing Target Reticle
            float pulse = 0.5f + 0.5f * std::sin(m_animTime * 6.0f);
            int reticle_r = 5 + static_cast<int>(pulse * 2.0f);

            DrawUtils::drawCircleOutline(framebuffer, stride, height, sx, sy, reticle_r, 0xFFFBBF24);

            // Reticle crosshair corner ticks
            if (sx - reticle_r - 2 >= vp_x) framebuffer[sy * stride + (sx - reticle_r - 2)] = 0xFFFFFFFF;
            if (sx + reticle_r + 2 < max_x) framebuffer[sy * stride + (sx + reticle_r + 2)] = 0xFFFFFFFF;
            if (sy - reticle_r - 2 >= vp_y) framebuffer[(sy - reticle_r - 2) * stride + sx] = 0xFFFFFFFF;
            if (sy + reticle_r + 2 < max_y) framebuffer[(sy + reticle_r + 2) * stride + sx] = 0xFFFFFFFF;

            // If target tile coordinates are active, draw coordinate tag card
            if (target_r >= 0 && target_c >= 0) {
                char tag_buf[32];
                std::snprintf(tag_buf, sizeof(tag_buf), "TARGET (%d,%d)", target_r, target_c);
                int tag_x = std::clamp(sx + reticle_r + 6, vp_x + 8, max_x - 110);
                int tag_y = std::clamp(sy - 7, vp_y + 80, max_y - 20);

                DrawUtils::drawRectFilled(framebuffer, stride, height, tag_x, tag_y, 96, 16, 0xF00F172A);
                DrawUtils::drawRectOutline(framebuffer, stride, height, tag_x, tag_y, 96, 16, 1, 0xFFF59E0B);
                DrawUtils::drawString(framebuffer, stride, height, tag_x + 6, tag_y + 4, tag_buf, 0xFFFDE047, 1);
            }
        } else if (glow > 0.20f) {
            // Active Firing Macro-Node (3px to 5px glowing orb with incandescent core)
            float r_f = std::clamp((v.r * (1.0f - glow) + 1.0f * glow) * 255.0f, 0.0f, 255.0f);
            float g_f = std::clamp((v.g * (1.0f - glow) + 0.96f * glow) * 255.0f, 0.0f, 255.0f);
            float b_f = std::clamp((v.b * (1.0f - glow) + 0.88f * glow) * 255.0f, 0.0f, 255.0f);
            uint8_t ur = static_cast<uint8_t>(r_f);
            uint8_t ug = static_cast<uint8_t>(g_f);
            uint8_t ub = static_cast<uint8_t>(b_f);
            uint32_t point_color = (0xFF << 24) | (ub << 16) | (ug << 8) | ur;

            framebuffer[sy * stride + sx] = 0xFFFFFFFF; // Incandescent center
            if (sx > vp_x) framebuffer[sy * stride + (sx - 1)] = point_color;
            if (sx + 1 < max_x) framebuffer[sy * stride + (sx + 1)] = point_color;
            if (sy > vp_y) framebuffer[(sy - 1) * stride + sx] = point_color;
            if (sy + 1 < max_y) framebuffer[(sy + 1) * stride + sx] = point_color;

            if (glow > 0.65f) {
                // Outer halo for high-intensity action potential spikes
                if (sx > vp_x + 1) framebuffer[sy * stride + (sx - 2)] = point_color;
                if (sx + 2 < max_x) framebuffer[sy * stride + (sx + 2)] = point_color;
                if (sy > vp_y + 1) framebuffer[(sy - 2) * stride + sx] = point_color;
                if (sy + 2 < max_y) framebuffer[(sy + 2) * stride + sx] = point_color;
            }
        } else {
            // Subdued Translucent Anatomical Scaffold (Single 1x1 pixel)
            // Low-luminance ambient tint forms the biological 3D silhouette without clutter
            uint8_t ur = static_cast<uint8_t>(v.r * 50.0f + 10.0f);
            uint8_t ug = static_cast<uint8_t>(v.g * 60.0f + 14.0f);
            uint8_t ub = static_cast<uint8_t>(v.b * 75.0f + 22.0f);
            framebuffer[sy * stride + sx] = (0xFF << 24) | (ub << 16) | (ug << 8) | ur;
        }
    }

    // 5. PAM11 Dopamine Reward Shockwave
    float dopa = engine.getDopamineLevel();
    if (dopa > 0.15f) {
        for (const auto& c : m_centroids) {
            if (c.id == NeuropilID::MushroomBody) {
                float dx = c.x - m_targetX;
                float dy = c.y - m_targetY;
                float dz = c.z - m_targetZ;
                float rx = dx * cos_y - dz * sin_y;
                float rz = dx * sin_y + dz * cos_y;
                float ry = dy * cos_p - rz * sin_p;
                float cam_z = dy * sin_p + rz * cos_p + m_distance;
                if (cam_z > 50.0f) {
                    float inv_z = 1.0f / cam_z;
                    int cx = static_cast<int>(half_w + (rx * fov_factor * inv_z));
                    int cy = static_cast<int>(half_h - (ry * fov_factor * inv_z));
                    int wave_r = static_cast<int>((1.0f - dopa) * 55.0f) + 8;
                    if (cx - wave_r > vp_x && cx + wave_r < max_x && cy - wave_r > vp_y && cy + wave_r < max_y) {
                        DrawUtils::drawCircleOutline(framebuffer, stride, height, cx, cy, wave_r, 0xFF34D399);
                        if (wave_r > 6) {
                            DrawUtils::drawCircleOutline(framebuffer, stride, height, cx, cy, wave_r - 4, 0xFF10B981);
                        }
                    }
                }
                break;
            }
        }
    }

    // 6. Collision-Free Solid Neuropil Badges
    struct ProjectedBadge {
        const char* name;
        uint32_t color;
        int orig_sx;
        int orig_sy;
        int badge_sx;
        int badge_sy;
        bool is_active;
        const char* active_label;
    };
    std::vector<ProjectedBadge> badges;

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

        bool active = false;
        const char* active_text = "";
        if (c.id == NeuropilID::OpticLobe && cognitive_phase == 1) {
            active = true;
            active_text = c.is_left_optic ? "[► L. OPTIC SCAN ◄]" : "[► R. OPTIC SCAN ◄]";
        } else if (c.id == NeuropilID::CentralComplex && cognitive_phase == 2) {
            active = true;
            active_text = "[► COMPASS (CX) ◄]";
        } else if (c.id == NeuropilID::MushroomBody && cognitive_phase == 3) {
            active = true;
            active_text = "[► MUSHROOM MB ◄]";
        } else if (c.id == NeuropilID::SubesophagealZone && cognitive_phase == 4) {
            active = true;
            active_text = "[► MOTOR SEZ ◄]";
        }

        ProjectedBadge b{};
        b.name = c.name;
        b.color = c.color;
        b.orig_sx = sx;
        b.orig_sy = sy;
        b.badge_sx = sx;
        b.badge_sy = sy;
        b.is_active = active;
        b.active_label = active_text;
        badges.push_back(b);
    }

    // Sort badges vertically by orig_sy to resolve screen collisions
    std::sort(badges.begin(), badges.end(), [](const ProjectedBadge& a, const ProjectedBadge& b) {
        return a.orig_sy < b.orig_sy;
    });

    // Resolve collisions: enforce minimum 24px vertical separation
    for (size_t i = 1; i < badges.size(); ++i) {
        if (badges[i].badge_sy < badges[i - 1].badge_sy + 24) {
            badges[i].badge_sy = badges[i - 1].badge_sy + 24;
        }
    }

    // Render each badge as a solid dark pill card with clear text
    for (auto& b : badges) {
        const char* display_text = b.is_active ? b.active_label : b.name;
        int text_len = static_cast<int>(std::strlen(display_text));
        int card_w = text_len * 8 + 24;
        int card_h = 20;

        // Clamp within viewport bounds
        int bx = std::clamp(b.badge_sx - card_w / 2, vp_x + 12, max_x - card_w - 12);
        int by = std::clamp(b.badge_sy - card_h / 2, vp_y + 80, max_y - card_h - 10);

        // Leader line from 3D centroid point to badge
        if (std::abs(bx + card_w / 2 - b.orig_sx) > 10 || std::abs(by + card_h / 2 - b.orig_sy) > 10) {
            DrawUtils::drawLine(framebuffer, stride, height, b.orig_sx, b.orig_sy, bx + card_w / 2, by + card_h / 2,
                                b.is_active ? 0xFFFFFFFF : (b.color & 0x66FFFFFF));
        }

        // Solid background card: prevents any dots or lines from bleeding through text!
        DrawUtils::drawRectFilled(framebuffer, stride, height, bx, by, card_w, card_h, 0xF2090E18);

        // 1px Border (bright white if active, neuropil color if idle)
        uint32_t border_col = b.is_active ? 0xFFFFFFFF : b.color;
        DrawUtils::drawRectOutline(framebuffer, stride, height, bx, by, card_w, card_h, 1, border_col);

        // Status dot
        uint32_t dot_col = b.is_active ? 0xFF22C55E : b.color;
        DrawUtils::drawCircleFilled(framebuffer, stride, height, bx + 10, by + card_h / 2, 3, dot_col);

        // Text
        uint32_t text_col = b.is_active ? 0xFFFFFFFF : 0xFFE2E8F0;
        DrawUtils::drawString(framebuffer, stride, height, bx + 18, by + 6, display_text, text_col, 1);
    }
}

} // namespace flybrain
