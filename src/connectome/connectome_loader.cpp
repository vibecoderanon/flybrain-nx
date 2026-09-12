#include "connectome_loader.hpp"
#include <fstream>
#include <iostream>
#include <cmath>
#include <random>
#include <algorithm>

namespace flybrain {

ConnectomeLoader::ConnectomeLoader() = default;
ConnectomeLoader::~ConnectomeLoader() = default;

ConnectomeLoader::ConnectomeLoader(ConnectomeLoader&&) noexcept = default;
ConnectomeLoader& ConnectomeLoader::operator=(ConnectomeLoader&&) noexcept = default;

bool ConnectomeLoader::loadFromFile(const std::string& filepath) {
    m_loaded = false;
    m_lastError.clear();

    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        m_lastError = "Could not open connectome file: " + filepath;
        return false;
    }

    // Read Header
    file.read(reinterpret_cast<char*>(&m_header), sizeof(ConnectomeHeader));
    if (!file || m_header.magic != CONNECTOME_MAGIC) {
        m_lastError = "Invalid magic bytes in connectome header (expected 'FLYB')";
        return false;
    }

    if (m_header.version != CONNECTOME_VERSION) {
        m_lastError = "Unsupported connectome version: " + std::to_string(m_header.version);
        return false;
    }

    if (m_header.num_neurons == 0 || m_header.num_synapses == 0) {
        m_lastError = "Invalid connectome header: zero neurons or synapses";
        return false;
    }

    // Allocate and read neurons
    try {
        m_neurons.resize(m_header.num_neurons);
        file.seekg(m_header.neuron_table_offset, std::ios::beg);
        file.read(reinterpret_cast<char*>(m_neurons.data()), m_header.num_neurons * sizeof(NeuronRecord));
        if (!file) {
            m_lastError = "Failed to read full neuron table";
            return false;
        }

        // Allocate and read synapses
        m_synapses.resize(m_header.num_synapses);
        file.seekg(m_header.synapse_table_offset, std::ios::beg);
        file.read(reinterpret_cast<char*>(m_synapses.data()), m_header.num_synapses * sizeof(SynapseRecord));
        if (!file) {
            m_lastError = "Failed to read full synapse table";
            return false;
        }

        // Read sensory neurons table if present
        if (m_header.sensory_count > 0 && m_header.sensory_table_offset > 0) {
            m_sensoryNeurons.resize(m_header.sensory_count);
            file.seekg(m_header.sensory_table_offset, std::ios::beg);
            file.read(reinterpret_cast<char*>(m_sensoryNeurons.data()), m_header.sensory_count * sizeof(SensoryNeuronEntry));
        }

        // Read motor neurons table if present
        if (m_header.motor_count > 0 && m_header.motor_table_offset > 0) {
            m_motorNeurons.resize(m_header.motor_count);
            file.seekg(m_header.motor_table_offset, std::ios::beg);
            file.read(reinterpret_cast<char*>(m_motorNeurons.data()), m_header.motor_count * sizeof(MotorNeuronEntry));
        }
    } catch (const std::exception& e) {
        m_lastError = std::string("Memory allocation failure: ") + e.what();
        m_neurons.clear();
        m_synapses.clear();
        return false;
    }

    m_loaded = true;
    return true;
}

bool ConnectomeLoader::loadSyntheticReference(uint32_t num_neurons, uint32_t avg_synapses_per_neuron) {
    m_loaded = false;
    m_lastError.clear();

    if (num_neurons == 0) num_neurons = 1000;
    if (avg_synapses_per_neuron == 0) avg_synapses_per_neuron = 30;

    std::mt19937 rng(42); // Deterministic seed for reproducible testing
    std::uniform_real_distribution<float> dist_pos(-300.0f, 300.0f);
    std::uniform_real_distribution<float> dist_01(0.0f, 1.0f);
    std::uniform_int_distribution<uint32_t> dist_target(0, num_neurons - 1);

    m_neurons.resize(num_neurons);
    m_synapses.clear();
    m_sensoryNeurons.clear();
    m_motorNeurons.clear();

    uint32_t current_synapse_offset = 0;

    // Temporary list for sorting per neuron
    struct TempSyn {
        uint32_t target;
        int16_t weight;
        uint8_t is_inhibitory;
    };
    std::vector<TempSyn> temp_syns;

    for (uint32_t i = 0; i < num_neurons; ++i) {
        NeuronRecord& n = m_neurons[i];

        // Biologically partition into neuropils
        // 0-35%: Optic Lobes, 35-50%: Central Complex, 50-65%: Mushroom Body, 
        // 65-80%: Antennal Lobe, 80-90%: SEZ, 90-100%: Descending Motor
        float fraction = static_cast<float>(i) / static_cast<float>(num_neurons);
        if (fraction < 0.35f) {
            n.neuropil_id = static_cast<uint8_t>(NeuropilID::OpticLobe);
            n.neurotransmitter = static_cast<uint8_t>(Neurotransmitter::Acetylcholine);
            n.flags = FLAG_SENSORY_VISUAL;
            // Position: bilateral lobes
            float side = (i % 2 == 0) ? -200.0f : 200.0f;
            n.x = side + dist_pos(rng) * 0.3f;
            n.y = dist_pos(rng) * 0.5f;
            n.z = dist_pos(rng) * 0.5f;
        } else if (fraction < 0.50f) {
            n.neuropil_id = static_cast<uint8_t>(NeuropilID::CentralComplex);
            n.neurotransmitter = (dist_01(rng) > 0.3f) ? static_cast<uint8_t>(Neurotransmitter::GABA) : static_cast<uint8_t>(Neurotransmitter::Acetylcholine);
            n.flags = FLAG_NONE;
            // Central ring / bridge
            float angle = dist_01(rng) * 6.283185f;
            float radius = 50.0f + dist_01(rng) * 20.0f;
            n.x = std::cos(angle) * radius;
            n.y = std::sin(angle) * radius;
            n.z = 10.0f + dist_pos(rng) * 0.1f;
        } else if (fraction < 0.65f) {
            n.neuropil_id = static_cast<uint8_t>(NeuropilID::MushroomBody);
            n.neurotransmitter = static_cast<uint8_t>(Neurotransmitter::Acetylcholine);
            n.flags = FLAG_NONE;
            n.x = dist_pos(rng) * 0.4f;
            n.y = 80.0f + dist_pos(rng) * 0.3f;
            n.z = 40.0f + dist_pos(rng) * 0.3f;
        } else if (fraction < 0.80f) {
            n.neuropil_id = static_cast<uint8_t>(NeuropilID::AntennalLobe);
            n.neurotransmitter = static_cast<uint8_t>(Neurotransmitter::Acetylcholine);
            n.flags = FLAG_SENSORY_ODOR;
            n.x = (i % 2 == 0 ? -60.0f : 60.0f) + dist_pos(rng) * 0.15f;
            n.y = -100.0f + dist_pos(rng) * 0.2f;
            n.z = -50.0f + dist_pos(rng) * 0.2f;
        } else if (fraction < 0.90f) {
            n.neuropil_id = static_cast<uint8_t>(NeuropilID::SubesophagealZone);
            n.neurotransmitter = static_cast<uint8_t>(Neurotransmitter::Acetylcholine);
            n.flags = FLAG_SENSORY_TASTE;
            n.x = dist_pos(rng) * 0.2f;
            n.y = -150.0f + dist_pos(rng) * 0.2f;
            n.z = -100.0f + dist_pos(rng) * 0.2f;
        } else {
            n.neuropil_id = static_cast<uint8_t>(NeuropilID::DescendingMotor);
            n.neurotransmitter = static_cast<uint8_t>(Neurotransmitter::Acetylcholine);
            n.flags = (i % 2 == 0) ? FLAG_MOTOR_FORWARD : FLAG_MOTOR_STEER;
            n.x = dist_pos(rng) * 0.2f;
            n.y = -200.0f + dist_pos(rng) * 0.2f;
            n.z = -120.0f + dist_pos(rng) * 0.2f;
        }

        // Register sensory / motor indices
        if (n.flags & FLAG_SENSORY_VISUAL) {
            SensoryNeuronEntry s{};
            s.neuron_idx = i;
            s.modality = 0; // Visual
            s.sub_channel = (n.x < 0) ? 0 : 1; // Left/Right eye
            m_sensoryNeurons.push_back(s);
        } else if (n.flags & FLAG_SENSORY_ODOR) {
            SensoryNeuronEntry s{};
            s.neuron_idx = i;
            s.modality = 1; // Olfactory
            s.sub_channel = 0;
            m_sensoryNeurons.push_back(s);
        } else if (n.flags & FLAG_SENSORY_TASTE) {
            SensoryNeuronEntry s{};
            s.neuron_idx = i;
            s.modality = (i % 2 == 0) ? 2 : 3; // Sweet or Bitter
            s.sub_channel = 0;
            m_sensoryNeurons.push_back(s);
        }

        if (n.flags & FLAG_MOTOR_FORWARD) {
            MotorNeuronEntry m{};
            m.neuron_idx = i;
            m.motor_type = 0; // Forward
            m_motorNeurons.push_back(m);
        } else if (n.flags & FLAG_MOTOR_STEER) {
            MotorNeuronEntry m{};
            m.neuron_idx = i;
            m.motor_type = (n.x < 0) ? 1 : 2; // Steer Left / Right
            m_motorNeurons.push_back(m);
        }

        // Generate outgoing synapses
        uint32_t out_count = std::max<uint32_t>(5, static_cast<uint32_t>(dist_01(rng) * avg_synapses_per_neuron * 2));
        temp_syns.clear();
        temp_syns.reserve(out_count);

        for (uint32_t s = 0; s < out_count; ++s) {
            TempSyn syn{};
            syn.target = dist_target(rng);
            if (syn.target == i) continue; // Avoid self-loop

            // Weight distribution matching FlyWire empirical data:
            // ~60% w=1, ~25% w=2, ~15% w>=3
            float r = dist_01(rng);
            if (r < 0.60f) {
                syn.weight = 1;
            } else if (r < 0.85f) {
                syn.weight = 2;
            } else {
                syn.weight = static_cast<int16_t>(3 + static_cast<int>(dist_01(rng) * 12));
            }

            syn.is_inhibitory = (n.neurotransmitter == static_cast<uint8_t>(Neurotransmitter::GABA)) ? 1 : 0;
            temp_syns.push_back(syn);
        }

        // Sort descending by weight for O(1) Speed Selector loop bounds
        std::sort(temp_syns.begin(), temp_syns.end(), [](const TempSyn& a, const TempSyn& b) {
            return a.weight > b.weight;
        });

        // Compute counts for speed selector
        uint16_t c_w3 = 0;
        uint16_t c_w2 = 0;
        for (const auto& ts : temp_syns) {
            if (ts.weight >= 3) c_w3++;
            if (ts.weight >= 2) c_w2++;
        }

        n.synapse_offset = current_synapse_offset;
        n.count_w_ge_3 = c_w3;
        n.count_w_ge_2 = c_w2;
        n.count_total = static_cast<uint16_t>(temp_syns.size());

        for (const auto& ts : temp_syns) {
            SynapseRecord sr{};
            sr.target_neuron_idx = ts.target;
            sr.weight = ts.weight;
            sr.is_inhibitory = ts.is_inhibitory;
            sr.flags = 0;
            m_synapses.push_back(sr);
            current_synapse_offset++;
        }
    }

    // Populate Header
    m_header.magic = CONNECTOME_MAGIC;
    m_header.version = CONNECTOME_VERSION;
    m_header.num_neurons = num_neurons;
    m_header.num_synapses = static_cast<uint32_t>(m_synapses.size());
    m_header.neuron_table_offset = sizeof(ConnectomeHeader);
    m_header.synapse_table_offset = m_header.neuron_table_offset + (m_neurons.size() * sizeof(NeuronRecord));
    m_header.sensory_table_offset = m_header.synapse_table_offset + (m_synapses.size() * sizeof(SynapseRecord));
    m_header.sensory_count = static_cast<uint32_t>(m_sensoryNeurons.size());
    m_header.motor_table_offset = m_header.sensory_table_offset + (m_sensoryNeurons.size() * sizeof(SensoryNeuronEntry));
    m_header.motor_count = static_cast<uint32_t>(m_motorNeurons.size());

    m_loaded = true;
    return true;
}

size_t ConnectomeLoader::getMemoryFootprintBytes() const {
    size_t total = sizeof(ConnectomeHeader);
    total += m_neurons.capacity() * sizeof(NeuronRecord);
    total += m_synapses.capacity() * sizeof(SynapseRecord);
    total += m_sensoryNeurons.capacity() * sizeof(SensoryNeuronEntry);
    total += m_motorNeurons.capacity() * sizeof(MotorNeuronEntry);
    return total;
}

} // namespace flybrain
