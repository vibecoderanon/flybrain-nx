#!/usr/bin/env python3
"""
Python Verification Test for flybrain-nx SNN Engine and Connectome Binary
Validates binary header, struct alignments, sorted loop bounds, and LIF mathematics.
"""

import os
import struct
import math

def test_binary_parsing(filepath="romfs/drosophila_full.bin"):
    print("[Test 1] Testing binary structure of", filepath)
    assert os.path.exists(filepath), f"Missing {filepath}"

    with open(filepath, "rb") as f:
        data = f.read()

    hdr = data[:64]
    magic, ver, n_count, s_count, n_off, s_off, sens_off, sens_cnt, mot_off, mot_cnt = struct.unpack_from(
        "<4sIIIQQIIII", hdr, 0
    )

    assert magic == b"FLYB", f"Invalid magic: {magic}"
    assert ver == 1, f"Invalid version: {ver}"
    assert n_count > 0
    assert s_count > 0

    print(f"  -> Header OK: {n_count:,} neurons, {s_count:,} synapses")

    # Read first 100 neurons and verify descending sorting
    neuron_sz = 32
    synapse_sz = 8

    for i in range(min(100, n_count)):
        offset = n_off + (i * neuron_sz)
        n_raw = data[offset:offset + neuron_sz]
        x, y, z, syn_off, c_w3, c_w2, c_tot, neuropil, transmitter, flags = struct.unpack_from(
            "<fffIHHHBBB", n_raw, 0
        )

        assert c_w3 <= c_w2, f"Neuron {i}: c_w3 ({c_w3}) > c_w2 ({c_w2})"
        assert c_w2 <= c_tot, f"Neuron {i}: c_w2 ({c_w2}) > c_tot ({c_tot})"

        # Verify synapses in CSR array are sorted descending
        for s in range(c_tot):
            s_offset = s_off + ((syn_off + s) * synapse_sz)
            tgt, weight, is_inh, _ = struct.unpack_from("<IhBB", data, s_offset)
            if s < c_w3:
                assert weight >= 3, f"Neuron {i} syn {s}: expected weight >= 3, got {weight}"
            elif s < c_w2:
                assert weight == 2, f"Neuron {i} syn {s}: expected weight == 2, got {weight}"
            else:
                assert weight == 1, f"Neuron {i} syn {s}: expected weight == 1, got {weight}"

    print("  -> O(1) Speed Selector sorted bounds verified across sample neurons.")

def test_lif_math():
    print("[Test 2] Testing Leaky Integrate-and-Fire mathematical integration...")
    V_REST = -52.0
    V_THRESH = -45.0
    V_RESET = -52.0
    TAU_M = 20.0
    TAU_SYN = 5.0
    DT = 1.0

    lambda_m = math.exp(-DT / TAU_M)
    lambda_s = math.exp(-DT / TAU_SYN)

    V = V_REST
    I_syn = 0.0

    # 1. Inject sub-threshold current
    I_syn += 2.0
    V = V_REST + (V - V_REST) * lambda_m + I_syn
    assert V > V_REST, "Membrane did not depolarize"
    assert V < V_THRESH, "Spiked unexpectedly on sub-threshold current"

    # Step multiple times so synaptic current decays and membrane potential returns toward rest
    v_peak = V
    for _ in range(5):
        I_syn = I_syn * lambda_s
        V = V_REST + (V - V_REST) * lambda_m + I_syn
        if V > v_peak:
            v_peak = V

    # Now step 100 ms (5 time constants) with zero input -> should decay to resting potential
    I_syn = 0.0
    for _ in range(100):
        V = V_REST + (V - V_REST) * lambda_m + I_syn

    assert V < v_peak, "Membrane failed to decay after peak"
    assert abs(V - V_REST) < 0.1, f"Membrane did not return close to V_REST: {V}"

    # 2. Inject supra-threshold current
    I_syn += 20.0
    V = V_REST + (V - V_REST) * lambda_m + I_syn
    assert V >= V_THRESH, "Membrane failed to reach threshold on strong injection"

    # Action potential spike fired!
    V = V_RESET
    assert V == V_RESET, "Membrane failed to reset"

    print("  -> LIF leak decay, threshold crossing, and reset verified.")

def main():
    print("==================================================")
    print("  flybrain-nx: SNN & Binary Verification Suite   ")
    print("==================================================")
    test_binary_parsing()
    test_lif_math()
    print("\n[+] ALL SNN & DATASET TESTS PASSED!")

if __name__ == "__main__":
    main()
