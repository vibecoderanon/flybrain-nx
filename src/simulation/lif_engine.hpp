#pragma once

#include "../connectome/connectome_format.hpp"
#include "../connectome/connectome_loader.hpp"
#include <vector>
#include <atomic>
#include <thread>
#include <chrono>

namespace flybrain {

enum class SpeedMode : uint8_t {
    RawFidelity   = 0, // All 54.5M synapses (w >= 1), 100% anatomical fidelity
    RealTime1kHz  = 1, // ~16.2M synapses (w >= 2), 1,000 Hz real-time compute
    HighSpeed     = 2  // ~9.4M synapses (w >= 3), 2,000+ Hz fast-forward / battery save
};

struct NeuronState {
    float V;                  // Membrane potential (mV)
    float I_syn;              // Synaptic current
    uint16_t refractory_timer;// Remaining refractory ticks
    uint8_t spiked_this_tick; // 1 if fired in current step
    float spike_luminance;    // Visual decay factor (1.0f on spike -> 0.0f)
};

struct NeuropilStats {
    uint32_t spike_count_recent;
    float firing_rate_hz;
};

struct SimulationTelemetry {
    uint32_t ticks_per_second;
    uint32_t total_spikes_recent;
    uint32_t active_spiking_neurons;
    float mean_membrane_potential;
    float simulation_hz;
    SpeedMode current_speed_mode;
    uint32_t active_synapse_count;
    NeuropilStats neuropils[9]; // Indexed by NeuropilID
};

class LIFEngine {
public:
    LIFEngine();
    ~LIFEngine();

    // Disallow copy
    LIFEngine(const LIFEngine&) = delete;
    LIFEngine& operator=(const LIFEngine&) = delete;

    /**
     * @brief Initialize simulation state from a loaded connectome
     */
    bool init(const ConnectomeLoader& loader);

    /**
     * @brief Advance simulation by a single discrete millisecond
     */
    void step(float dt_ms = 1.0f);

    /**
     * @brief Run background simulation thread pinned to high-performance cores
     */
    void startAsync();
    void stopAsync();

    void setPaused(bool paused) { m_paused = paused; }
    bool isPaused() const { return m_paused; }

    void setSpeedMode(SpeedMode mode);
    SpeedMode getSpeedMode() const { return m_speedMode; }

    /**
     * @brief Sensory stimulation injectors
     */
    void injectCurrent(uint32_t neuron_idx, float current);
    void injectSensoryModality(uint8_t modality, uint8_t channel, float current);
    void injectOpticFlow(float horizontal_flow, float vertical_flow);
    void injectOdorPuff(float intensity);
    void injectTaste(bool sweet, float intensity);
    void injectPredatorLoom();

    /**
     * @brief Motor readout queries
     */
    float getMotorThrust() const { return m_motorThrust; }
    float getMotorYaw() const { return m_motorYaw; }
    bool  getMotorEscapeTriggered() const { return m_motorEscapeTriggered; }

    // Telemetry & State Access
    const NeuronState* getNeuronStates() const { return m_states.data(); }
    uint32_t getNeuronCount() const { return m_numNeurons; }
    SimulationTelemetry getTelemetry();

    // Constant biological parameters (Shiu et al. 2024 defaults)
    static constexpr float V_REST     = -52.0f; // mV
    static constexpr float V_THRESH   = -45.0f; // mV
    static constexpr float V_RESET    = -52.0f; // mV
    static constexpr float TAU_M      = 20.0f;  // Membrane time constant (ms)
    static constexpr float TAU_SYN    = 5.0f;   // Synaptic current decay (ms)
    static constexpr uint16_t REFRAC_TICKS = 2; // 2 ms refractory period

private:
    void workerLoop();
    void updateTelemetry(uint32_t spikes_in_second, uint32_t ticks_in_second);

    uint32_t m_numNeurons = 0;
    uint32_t m_numSynapses = 0;
    const NeuronRecord* m_neurons = nullptr;
    const SynapseRecord* m_synapses = nullptr;
    std::vector<SensoryNeuronEntry> m_sensoryList;
    std::vector<MotorNeuronEntry> m_motorList;

    std::vector<NeuronState> m_states;
    std::vector<std::atomic<float>> m_synapticInputAccumulator;

    std::atomic<SpeedMode> m_speedMode{SpeedMode::RealTime1kHz};
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_paused{false};
    std::thread m_workerThread;

    // Motor output integration
    float m_motorThrust = 0.0f;
    float m_motorYaw = 0.0f;
    bool m_motorEscapeTriggered = false;

    // Telemetry storage
    SimulationTelemetry m_telemetry{};
    std::chrono::steady_clock::time_point m_lastTelemetryTime;
    uint32_t m_ticksCounter = 0;
    uint32_t m_spikesCounter = 0;
    uint32_t m_neuropilSpikeCounters[9]{};
};

} // namespace flybrain
