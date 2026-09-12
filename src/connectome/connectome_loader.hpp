#pragma once

#include "connectome_format.hpp"
#include <string>
#include <vector>
#include <memory>

namespace flybrain {

class ConnectomeLoader {
public:
    ConnectomeLoader();
    ~ConnectomeLoader();

    // Disallow copy, allow move
    ConnectomeLoader(const ConnectomeLoader&) = delete;
    ConnectomeLoader& operator=(const ConnectomeLoader&) = delete;
    ConnectomeLoader(ConnectomeLoader&&) noexcept;
    ConnectomeLoader& operator=(ConnectomeLoader&&) noexcept;

    /**
     * @brief Load connectome from a binary file (e.g. "romfs:/drosophila_full.bin")
     * @return true on success, false on error
     */
    bool loadFromFile(const std::string& filepath);

    /**
     * @brief Populate a reference/synthetic test connectome in-memory for testing
     * @param num_neurons Number of neurons to generate
     * @param avg_synapses_per_neuron Average fan-out
     */
    bool loadSyntheticReference(uint32_t num_neurons = 1000, uint32_t avg_synapses_per_neuron = 40);

    bool isLoaded() const { return m_loaded; }

    const ConnectomeHeader& getHeader() const { return m_header; }
    uint32_t getNeuronCount() const { return m_header.num_neurons; }
    uint32_t getSynapseCount() const { return m_header.num_synapses; }

    const NeuronRecord* getNeurons() const { return m_neurons.data(); }
    const SynapseRecord* getSynapses() const { return m_synapses.data(); }

    const std::vector<SensoryNeuronEntry>& getSensoryNeurons() const { return m_sensoryNeurons; }
    const std::vector<MotorNeuronEntry>& getMotorNeurons() const { return m_motorNeurons; }

    size_t getMemoryFootprintBytes() const;
    const std::string& getLastError() const { return m_lastError; }

private:
    bool m_loaded = false;
    ConnectomeHeader m_header{};
    std::vector<NeuronRecord> m_neurons;
    std::vector<SynapseRecord> m_synapses;
    std::vector<SensoryNeuronEntry> m_sensoryNeurons;
    std::vector<MotorNeuronEntry> m_motorNeurons;
    std::string m_lastError;
};

} // namespace flybrain
