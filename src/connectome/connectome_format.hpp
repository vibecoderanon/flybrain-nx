#pragma once

#include <cstdint>
#include <cstddef>

namespace flybrain {

// Magic FourCC for flybrain connectome format: 'FLYB'
constexpr uint32_t CONNECTOME_MAGIC = 0x42594C46;
constexpr uint32_t CONNECTOME_VERSION = 1;

// Neuropil (Brain Region) Identifiers
enum class NeuropilID : uint8_t {
    Unknown          = 0,
    OpticLobe        = 1, // Medulla, Lobula, Lobula Plate (Vision / Motion)
    CentralComplex   = 2, // Ellipsoid Body, Protocerebral Bridge, Fan-shaped Body (Compass / Navigation)
    MushroomBody     = 3, // Kenyon Cells, Calyx, Lobes (Olfactory Learning & Memory)
    AntennalLobe     = 4, // Glomeruli (Primary Olfaction)
    SubesophagealZone= 5, // SEZ / GNAT (Taste / Gustatory / Feeding)
    DescendingMotor  = 6, // Descending Neurons to Ventral Nerve Cord (Motor Control)
    AscendingSensory = 7, // Sensory Afferents (Tactile, Proprioceptive)
    SuperiorNeuropils= 8  // Lateral Horn, Superior Protocerebrum
};

// Neurotransmitter Types
enum class Neurotransmitter : uint8_t {
    Unknown      = 0,
    Acetylcholine= 1, // Excitatory (+)
    GABA         = 2, // Inhibitory (-)
    Glutamate    = 3, // Inhibitory / Excitatory depending on receptor
    Dopamine     = 4, // Neuromodulatory
    Serotonin    = 5, // Neuromodulatory
    Octopamine   = 6  // Neuromodulatory (Invertebrate Noradrenaline)
};

// Sensory / Motor Functional Flags
enum NeuronFlags : uint8_t {
    FLAG_NONE             = 0,
    FLAG_SENSORY_VISUAL   = (1 << 0), // Compound eye photoreceptors / L1-L5
    FLAG_SENSORY_ODOR     = (1 << 1), // Antennal olfactory receptor neurons
    FLAG_SENSORY_TASTE    = (1 << 2), // Proboscis / leg gustatory receptors
    FLAG_MOTOR_FORWARD    = (1 << 3), // Forward thrust (e.g. DNp01)
    FLAG_MOTOR_STEER      = (1 << 4), // Yaw steering turn (e.g. DNb01)
    FLAG_MOTOR_ESCAPE     = (1 << 5), // Giant Fiber jump reflex
    FLAG_MOTOR_GROOM      = (1 << 6)  // Antennal / leg grooming (e.g. MDN)
};

#pragma pack(push, 1)

/**
 * @brief Binary Header for romfs:/drosophila_full.bin
 */
struct ConnectomeHeader {
    uint32_t magic;                 // 'FLYB' (0x42594C46)
    uint32_t version;               // 1
    uint32_t num_neurons;           // e.g. 139,255
    uint32_t num_synapses;          // e.g. 54,498,591
    uint64_t neuron_table_offset;   // Byte offset to NeuronRecord array
    uint64_t synapse_table_offset;  // Byte offset to SynapseRecord array
    uint32_t sensory_table_offset;  // Byte offset to SensoryNeuronEntry array
    uint32_t sensory_count;         // Number of designated sensory neurons
    uint32_t motor_table_offset;    // Byte offset to MotorNeuronEntry array
    uint32_t motor_count;           // Number of designated motor neurons
    uint8_t  reserved[16];          // Future expansion (header total = 64 bytes)
};

/**
 * @brief Individual Neuron Metadata & Outgoing Synapse Offsets (32 bytes)
 *
 * Synapses in the CSR array are pre-sorted descending by weight:
 *   [ Strong (w >= 3) | Moderate (w == 2) | Noise/Single-Vesicle (w == 1) ]
 *
 * To filter synapses at runtime in O(1):
 *   - High-Speed mode:   iterate [0 ... count_w_ge_3 - 1]
 *   - Real-Time 1kHz:    iterate [0 ... count_w_ge_2 - 1]
 *   - Raw Fidelity:      iterate [0 ... count_total - 1]
 */
struct NeuronRecord {
    float    x, y, z;               // Spatial 3D coordinates in Drosophila standard microns
    uint32_t synapse_offset;        // Index offset into global SynapseRecord array
    uint16_t count_w_ge_3;          // Synapses with weight >= 3 (High-Speed bound)
    uint16_t count_w_ge_2;          // Synapses with weight >= 2 (Real-Time 1kHz bound)
    uint16_t count_total;           // All outgoing synapses (Raw Fidelity bound)
    uint8_t  neuropil_id;           // NeuropilID enum
    uint8_t  neurotransmitter;      // Neurotransmitter enum
    uint8_t  flags;                 // NeuronFlags bitmask
    uint8_t  reserved[7];           // Alignment padding to 32 bytes
};

/**
 * @brief Individual Directed Synapse Record (8 bytes aligned)
 */
struct SynapseRecord {
    uint32_t target_neuron_idx;     // Target neuron index (0 to num_neurons - 1)
    int16_t  weight;                // Biological synaptic weight (number of synaptic connections)
    uint8_t  is_inhibitory;         // 1 if inhibitory, 0 if excitatory
    uint8_t  flags;                 // Reserved flags
};

/**
 * @brief Sensory Receptor Quick-Lookup Entry
 */
struct SensoryNeuronEntry {
    uint32_t neuron_idx;
    uint8_t  modality;              // 0=Visual, 1=Olfactory, 2=GustatorySweet, 3=GustatoryBitter, 4=Mechanosensory
    uint8_t  sub_channel;           // E.g. left eye vs right eye, or specific glomerulus ID
    uint16_t reserved;
};

/**
 * @brief Motor Descending Neuron Quick-Lookup Entry
 */
struct MotorNeuronEntry {
    uint32_t neuron_idx;
    uint8_t  motor_type;            // 0=ForwardSpeed, 1=SteerLeft, 2=SteerRight, 3=EscapeJump, 4=Groom
    uint8_t  reserved[3];
};

#pragma pack(pop)

static_assert(sizeof(ConnectomeHeader) == 64, "ConnectomeHeader must be 64 bytes");
static_assert(sizeof(NeuronRecord) == 32, "NeuronRecord must be 32 bytes");
static_assert(sizeof(SynapseRecord) == 8, "SynapseRecord must be 8 bytes");

} // namespace flybrain
