#include "lif_engine.hpp"
#include <cmath>
#include <iostream>
#include <algorithm>

namespace flybrain {

LIFEngine::LIFEngine() = default;

LIFEngine::~LIFEngine() {
    stopAsync();
}

bool LIFEngine::init(const ConnectomeLoader& loader) {
    stopAsync();

    m_numNeurons = loader.getNeuronCount();
    m_numSynapses = loader.getSynapseCount();
    m_neurons = loader.getNeurons();
    m_synapses = loader.getSynapses();
    m_sensoryList = loader.getSensoryNeurons();
    m_motorList = loader.getMotorNeurons();

    if (m_numNeurons == 0 || !m_neurons || !m_synapses) {
        return false;
    }

    // Allocate neuron states and double-buffered input accumulators
    m_states.resize(m_numNeurons);
    m_synapticInputAccumulator = std::vector<std::atomic<float>>(m_numNeurons);
    m_activeLines.clear();
    m_activeLines.reserve(MAX_ACTIVE_LINES);

    for (uint32_t i = 0; i < m_numNeurons; ++i) {
        m_states[i].V = V_REST;
        m_states[i].I_syn = 0.0f;
        m_states[i].refractory_timer = 0;
        m_states[i].spiked_this_tick = 0;
        m_states[i].spike_luminance = 0.0f;
        m_synapticInputAccumulator[i].store(0.0f, std::memory_order_relaxed);
    }

    m_lastTelemetryTime = std::chrono::steady_clock::now();
    m_ticksCounter = 0;
    m_spikesCounter = 0;
    std::fill(std::begin(m_neuropilSpikeCounters), std::end(m_neuropilSpikeCounters), 0);

    // Initialize tile neuron cluster mappings (up to 100 tiles for 10x10)
    m_tileNeuronClusters.resize(100);
    for (uint32_t t = 0; t < 100; ++t) {
        // Distribute evenly across connectome with Central Complex / Optic bias
        m_tileNeuronClusters[t] = (t * 7919) % m_numNeurons;
    }
    m_picrossFocusNeuron = m_tileNeuronClusters[0];

    return true;
}

void LIFEngine::setSpeedMode(SpeedMode mode) {
    m_speedMode.store(mode, std::memory_order_release);
}

void LIFEngine::injectCurrent(uint32_t neuron_idx, float current) {
    if (neuron_idx < m_numNeurons) {
        m_synapticInputAccumulator[neuron_idx].fetch_add(current, std::memory_order_relaxed);
    }
}

void LIFEngine::injectSensoryModality(uint8_t modality, uint8_t channel, float current) {
    for (const auto& s : m_sensoryList) {
        if (s.modality == modality && (channel == 0xFF || s.sub_channel == channel)) {
            injectCurrent(s.neuron_idx, current);
        }
    }
}

void LIFEngine::injectOpticFlow(float horizontal_flow, float vertical_flow) {
    // Left eye vs right eye differential stimulation for yaw visual drift
    float left_stim  = std::max(0.0f, -horizontal_flow) * 18.0f;
    float right_stim = std::max(0.0f,  horizontal_flow) * 18.0f;
    float pitch_stim = std::abs(vertical_flow) * 10.0f;

    injectSensoryModality(0, 0, left_stim + pitch_stim);  // Left compound eye
    injectSensoryModality(0, 1, right_stim + pitch_stim); // Right compound eye
}

void LIFEngine::injectOdorPuff(float intensity) {
    // Stimulate antennal olfactory receptor neurons
    injectSensoryModality(1, 0, intensity * 24.0f);
}

void LIFEngine::injectTaste(bool sweet, float intensity) {
    // 2=Sweet (proboscis extension / forward approach), 3=Bitter (avoidance)
    uint8_t modality = sweet ? 2 : 3;
    injectSensoryModality(modality, 0, intensity * 28.0f);
}

void LIFEngine::injectPredatorLoom() {
    // Giant Fiber emergency reflex: bilateral visual burst
    injectSensoryModality(0, 0xFF, 55.0f);
}

void LIFEngine::bindPicrossBoard(PicrossBoard* board) {
    m_picrossBoard = board;
}

void LIFEngine::updatePicrossSensoryFeedback() {
    if (!m_picrossBoard || m_numNeurons == 0) return;

    int h = m_picrossBoard->getHeight();
    int w = m_picrossBoard->getWidth();

    // 1. Evaluate Rows: dopamine reward if satisfied, GABAergic inhibition if contradiction
    for (int r = 0; r < h; ++r) {
        if (m_picrossBoard->isRowSatisfied(r)) {
            // Mushroom body dopamine burst (positive reinforcement)
            for (int c = 0; c < w; ++c) {
                uint32_t n_idx = m_tileNeuronClusters[(r * w + c) % m_tileNeuronClusters.size()];
                injectCurrent(n_idx, 3.5f);
            }
        } else if (m_picrossBoard->isRowContradiction(r)) {
            // GABAergic aversive burst (suppress candidate neurons in this row)
            for (int c = 0; c < w; ++c) {
                uint32_t n_idx = m_tileNeuronClusters[(r * w + c) % m_tileNeuronClusters.size()];
                injectCurrent(n_idx, -8.0f);
            }
        }
    }

    // 2. Evaluate Columns
    for (int c = 0; c < w; ++c) {
        if (m_picrossBoard->isColSatisfied(c)) {
            for (int r = 0; r < h; ++r) {
                uint32_t n_idx = m_tileNeuronClusters[(r * w + c) % m_tileNeuronClusters.size()];
                injectCurrent(n_idx, 3.5f);
            }
        } else if (m_picrossBoard->isColContradiction(c)) {
            for (int r = 0; r < h; ++r) {
                uint32_t n_idx = m_tileNeuronClusters[(r * w + c) % m_tileNeuronClusters.size()];
                injectCurrent(n_idx, -8.0f);
            }
        }
    }
}

bool LIFEngine::stepPicrossSolver(int& out_r, int& out_c, CellState& out_action) {
    if (!m_picrossBoard || m_picrossBoard->isSolved()) return false;

    updatePicrossSensoryFeedback();

    // Find next deterministic/probabilistic deduction from attractor state
    if (m_picrossBoard->findNextDeduction(out_r, out_c, out_action)) {
        int w = m_picrossBoard->getWidth();
        uint32_t cluster_idx = (out_r * w + out_c) % m_tileNeuronClusters.size();
        m_picrossFocusNeuron = m_tileNeuronClusters[cluster_idx];

        // Trigger action potential in target coordinate neuron
        injectCurrent(m_picrossFocusNeuron, 35.0f);

        // Motor branch:
        if (out_action == CellState::Filled) {
            // Proboscis extension / SEZ feeding motor burst
            injectTaste(true, 1.2f);
        } else {
            // Grooming / leg scratching motor burst
            injectTaste(false, 0.8f);
        }

        // Apply deduction to board
        m_picrossBoard->setCell(out_r, out_c, out_action);
        return true;
    }

    return false;
}

void LIFEngine::step(float dt_ms) {
    if (m_numNeurons == 0) return;

    // Decay parameters
    const float lambda_m = std::exp(-dt_ms / TAU_M);   // ~0.9512
    const float lambda_s = std::exp(-dt_ms / TAU_SYN); // ~0.8187

    SpeedMode active_mode = m_speedMode.load(std::memory_order_acquire);
    uint32_t active_spikes = 0;
    float total_V = 0.0f;

    // 1. Spontaneous biological baseline noise (sparse Poisson-like hum)
    // 0.2% probability per tick for each of ~20 random neurons to receive gentle noise
    for (int k = 0; k < 25; ++k) {
        m_stepRandState ^= (m_stepRandState << 13);
        m_stepRandState ^= (m_stepRandState >> 17);
        m_stepRandState ^= (m_stepRandState << 5);
        uint32_t target = m_stepRandState % m_numNeurons;
        m_synapticInputAccumulator[target].fetch_add(1.8f, std::memory_order_relaxed);
    }

    // Local temporary buffer of spiking neurons to propagate after integration
    std::vector<uint32_t> spiking_neurons;
    spiking_neurons.reserve(m_numNeurons / 16);

    for (uint32_t i = 0; i < m_numNeurons; ++i) {
        NeuronState& state = m_states[i];
        const NeuronRecord& n = m_neurons[i];

        // Consume accumulated external sensory/synaptic input atomically
        float ext_input = m_synapticInputAccumulator[i].exchange(0.0f, std::memory_order_relaxed);

        // Update synaptic current decay and external drive
        state.I_syn = (state.I_syn * lambda_s) + ext_input;

        if (state.refractory_timer > 0) {
            state.refractory_timer--;
            state.V = V_RESET;
            state.spiked_this_tick = 0;
        } else {
            // Integrate Leaky Integrate-and-Fire equation
            state.V = V_REST + (state.V - V_REST) * lambda_m + state.I_syn;

            if (state.V >= V_THRESH) {
                // Neuron fires action potential!
                state.V = V_RESET;
                state.refractory_timer = REFRAC_TICKS;
                state.spiked_this_tick = 1;
                state.spike_luminance = 1.0f;

                spiking_neurons.push_back(i);
                active_spikes++;

                uint8_t n_id = n.neuropil_id;
                if (n_id < 9) {
                    m_neuropilSpikeCounters[n_id]++;
                }
            } else {
                state.spiked_this_tick = 0;
            }
        }

        // Smoothly decay visual luminance for 3D glow display
        state.spike_luminance = std::max(0.0f, state.spike_luminance * 0.85f);
        total_V += state.V;
    }

    // 2. Synaptic Propagation Phase (O(1) loop bounds based on speed mode)
    // Age existing synaptic lines first
    for (auto it = m_activeLines.begin(); it != m_activeLines.end(); ) {
        it->intensity -= 0.18f;
        if (it->intensity <= 0.0f) {
            it = m_activeLines.erase(it);
        } else {
            ++it;
        }
    }

    for (uint32_t spike_idx : spiking_neurons) {
        const NeuronRecord& n = m_neurons[spike_idx];

        uint32_t syn_count = 0;
        switch (active_mode) {
            case SpeedMode::RawFidelity:
                syn_count = n.count_total;
                break;
            case SpeedMode::RealTime1kHz:
                syn_count = n.count_w_ge_2;
                break;
            case SpeedMode::HighSpeed:
                syn_count = n.count_w_ge_3;
                break;
        }

        const SynapseRecord* syn_base = &m_synapses[n.synapse_offset];
        uint32_t lines_added_for_this_spike = 0;

        for (uint32_t s = 0; s < syn_count; ++s) {
            const SynapseRecord& syn = syn_base[s];

            // Unitary EPSP scaling factor calibrated to prevent epileptic runaway
            float w = static_cast<float>(syn.weight) * EPSP_SCALE;
            if (syn.is_inhibitory) {
                w = -w * GABA_SCALE;
            }
            m_synapticInputAccumulator[syn.target_neuron_idx].fetch_add(w, std::memory_order_relaxed);

            // Record active synaptic beam for 3D visualizer (sample up to 4 primary lines per spike)
            if (lines_added_for_this_spike < 4 && m_activeLines.size() < MAX_ACTIVE_LINES) {
                SynapticLineEvent line{};
                line.src_idx = spike_idx;
                line.dst_idx = syn.target_neuron_idx;
                line.intensity = 1.0f;
                line.neuropil_id = n.neuropil_id;
                m_activeLines.push_back(line);
                lines_added_for_this_spike++;
            }
        }
    }

    // 3. Motor Readout Integration
    float forward_spikes = 0.0f;
    float steer_left_spikes = 0.0f;
    float steer_right_spikes = 0.0f;
    bool escape_fired = false;

    for (const auto& m : m_motorList) {
        if (m_states[m.neuron_idx].spiked_this_tick) {
            if (m.motor_type == 0) forward_spikes += 1.0f;
            else if (m.motor_type == 1) steer_left_spikes += 1.0f;
            else if (m.motor_type == 2) steer_right_spikes += 1.0f;
            else if (m.motor_type == 3) escape_fired = true;
        }
    }

    m_motorThrust = (m_motorThrust * 0.95f) + (forward_spikes * 0.05f);
    m_motorYaw    = (m_motorYaw * 0.95f) + ((steer_right_spikes - steer_left_spikes) * 0.05f);
    m_motorEscapeTriggered = escape_fired;

    // Update Telemetry counters
    m_ticksCounter++;
    m_spikesCounter += active_spikes;

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastTelemetryTime).count();
    if (elapsed >= 1000) {
        updateTelemetry(m_spikesCounter, m_ticksCounter);
        m_lastTelemetryTime = now;
        m_ticksCounter = 0;
        m_spikesCounter = 0;
    }
}

void LIFEngine::updateTelemetry(uint32_t spikes_in_second, uint32_t ticks_in_second) {
    m_telemetry.ticks_per_second = ticks_in_second;
    m_telemetry.simulation_hz = static_cast<float>(ticks_in_second);
    m_telemetry.total_spikes_recent = spikes_in_second;
    m_telemetry.current_speed_mode = m_speedMode.load(std::memory_order_relaxed);
    m_telemetry.active_line_count = static_cast<uint32_t>(m_activeLines.size());

    // Active synapse count depending on mode
    uint64_t total_active = 0;
    SpeedMode m = m_telemetry.current_speed_mode;
    for (uint32_t i = 0; i < m_numNeurons; ++i) {
        if (m == SpeedMode::RawFidelity) total_active += m_neurons[i].count_total;
        else if (m == SpeedMode::RealTime1kHz) total_active += m_neurons[i].count_w_ge_2;
        else total_active += m_neurons[i].count_w_ge_3;
    }
    m_telemetry.active_synapse_count = static_cast<uint32_t>(total_active);

    for (int p = 0; p < 9; ++p) {
        m_telemetry.neuropils[p].spike_count_recent = m_neuropilSpikeCounters[p];
        m_telemetry.neuropils[p].firing_rate_hz = static_cast<float>(m_neuropilSpikeCounters[p]);
        m_neuropilSpikeCounters[p] = 0;
    }
}

SimulationTelemetry LIFEngine::getTelemetry() {
    m_telemetry.active_line_count = static_cast<uint32_t>(m_activeLines.size());
    return m_telemetry;
}

void LIFEngine::startAsync() {
    if (m_running.load()) return;
    m_running.store(true);
    m_workerThread = std::thread(&LIFEngine::workerLoop, this);
}

void LIFEngine::stopAsync() {
    if (!m_running.load()) return;
    m_running.store(false);
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
}

void LIFEngine::workerLoop() {
    using namespace std::chrono;
    while (m_running.load()) {
        if (!m_paused.load()) {
            step(1.0f);
        } else {
            std::this_thread::sleep_for(milliseconds(10));
        }
    }
}

} // namespace flybrain
