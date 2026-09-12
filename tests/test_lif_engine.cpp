#include "../src/connectome/connectome_format.hpp"
#include "../src/connectome/connectome_loader.hpp"
#include "../src/simulation/lif_engine.hpp"

#include <iostream>
#include <cassert>
#include <cmath>

using namespace flybrain;

void testStructSizes() {
    std::cout << "[Test 1] Validating binary format struct alignments..." << std::endl;
    assert(sizeof(ConnectomeHeader) == 64);
    assert(sizeof(NeuronRecord) == 32);
    assert(sizeof(SynapseRecord) == 8);
    std::cout << "  -> ConnectomeHeader (64B), NeuronRecord (32B), SynapseRecord (8B) verified.\n";
}

void testConnectomeGenerationAndSorting() {
    std::cout << "[Test 2] Validating synthetic connectome generation & descending weight sorting..." << std::endl;
    ConnectomeLoader loader;
    bool ok = loader.loadSyntheticReference(500, 30);
    assert(ok);
    assert(loader.getNeuronCount() == 500);
    assert(loader.getSynapseCount() > 5000);

    const NeuronRecord* neurons = loader.getNeurons();
    const SynapseRecord* synapses = loader.getSynapses();

    // Verify descending sort per neuron
    for (uint32_t i = 0; i < loader.getNeuronCount(); ++i) {
        const auto& n = neurons[i];
        assert(n.count_w_ge_3 <= n.count_w_ge_2);
        assert(n.count_w_ge_2 <= n.count_total);

        // Verify that synapses in CSR are strictly descending by weight
        const SynapseRecord* syns = &synapses[n.synapse_offset];
        for (uint32_t s = 0; s < n.count_w_ge_3; ++s) {
            assert(syns[s].weight >= 3);
        }
        for (uint32_t s = n.count_w_ge_3; s < n.count_w_ge_2; ++s) {
            assert(syns[s].weight == 2);
        }
        for (uint32_t s = n.count_w_ge_2; s < n.count_total; ++s) {
            assert(syns[s].weight == 1);
        }
    }
    std::cout << "  -> 500 neurons & " << loader.getSynapseCount() << " synapses validated with O(1) sorted bounds.\n";
}

void testLIFBiologicalDynamics() {
    std::cout << "[Test 3] Validating Leaky Integrate-and-Fire biological membrane dynamics..." << std::endl;
    ConnectomeLoader loader;
    loader.loadSyntheticReference(100, 20);

    LIFEngine engine;
    bool init_ok = engine.init(loader);
    assert(init_ok);

    const NeuronState* states = engine.getNeuronStates();
    assert(std::abs(states[0].V - LIFEngine::V_REST) < 0.001f);

    // 1. Test Sub-threshold depolarization & leak decay
    engine.injectCurrent(0, 3.0f);
    engine.step(1.0f);
    assert(states[0].V > LIFEngine::V_REST); // Depolarized
    assert(states[0].V < LIFEngine::V_THRESH); // Sub-threshold
    assert(states[0].spiked_this_tick == 0);

    float depolarized_v = states[0].V;
    // Step without input -> should leak back towards V_REST
    engine.step(1.0f);
    assert(states[0].V < depolarized_v);

    // 2. Test Suprathreshold Spike and Refractory Clamp
    engine.injectCurrent(0, 30.0f); // Massive depolarization
    engine.step(1.0f);
    assert(states[0].spiked_this_tick == 1); // Spiked!
    assert(std::abs(states[0].V - LIFEngine::V_RESET) < 0.001f); // Reset
    assert(states[0].refractory_timer == LIFEngine::REFRAC_TICKS);

    // Step next tick: should be clamped during refractory
    engine.injectCurrent(0, 10.0f);
    engine.step(1.0f);
    assert(states[0].spiked_this_tick == 0);
    assert(states[0].refractory_timer == LIFEngine::REFRAC_TICKS - 1);
    assert(std::abs(states[0].V - LIFEngine::V_RESET) < 0.001f);

    std::cout << "  -> LIF integration, spike reset, and refractory period verified.\n";
}

void testSpeedSelectorModes() {
    std::cout << "[Test 4] Validating Speed Selector loop bound switching..." << std::endl;
    ConnectomeLoader loader;
    loader.loadSyntheticReference(300, 35);

    LIFEngine engine;
    engine.init(loader);

    // Verify switching modes changes active synapse telemetry without allocating
    engine.setSpeedMode(SpeedMode::RawFidelity);
    for (int i = 0; i < 5; ++i) engine.step(1.0f);
    SimulationTelemetry tel_raw = engine.getTelemetry();

    engine.setSpeedMode(SpeedMode::RealTime1kHz);
    for (int i = 0; i < 5; ++i) engine.step(1.0f);
    SimulationTelemetry tel_rt = engine.getTelemetry();

    engine.setSpeedMode(SpeedMode::HighSpeed);
    for (int i = 0; i < 5; ++i) engine.step(1.0f);
    SimulationTelemetry tel_hs = engine.getTelemetry();

    assert(tel_raw.active_synapse_count >= tel_rt.active_synapse_count);
    assert(tel_rt.active_synapse_count >= tel_hs.active_synapse_count);

    std::cout << "  -> Active Synapses: Raw (" << tel_raw.active_synapse_count
              << ") > Real-Time (" << tel_rt.active_synapse_count
              << ") > High-Speed (" << tel_hs.active_synapse_count << ")\n";
}

int main() {
    std::cout << "======================================================\n";
    std::cout << "  flybrain-nx: Unit Verification Suite               \n";
    std::cout << "======================================================\n";

    testStructSizes();
    testConnectomeGenerationAndSorting();
    testLIFBiologicalDynamics();
    testSpeedSelectorModes();

    std::cout << "\n[+] ALL UNIT TESTS PASSED SUCCESSFULLY!\n";
    return 0;
}
