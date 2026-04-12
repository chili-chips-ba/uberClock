import os
import json
import numpy as np
import matplotlib.pyplot as plt

class MultiModeQuartzCrystal:
    def __init__(self, C0, modes_dict):
        """
        Class simulating a physical quartz crystal with multiple resonant modes.
        C0: Static case capacitance (Farads)
        modes_dict: Dictionary containing R (Ohms), L (Henries), C (Farads), and f (Hz) for each mode.
        """
        self.C0 = C0
        self.modes = modes_dict

    def get_impedance(self, frequencies):
        """ Calculates the total complex impedance of the crystal across given frequencies. """
        w = 2 * np.pi * frequencies
        Y_total = 1j * w * self.C0
        
        for mode_name, params in self.modes.items():
            R = params['R']
            L = params['L']
            C = params['C']
            Zm = R + 1j * w * L + 1 / (1j * w * C)
            Y_total += 1 / Zm
            
        return 1 / Y_total

if __name__ == "__main__":
    # 1. Load parameters from the auto-generated file
    current_dir = os.path.dirname(os.path.abspath(__file__))
    params_file = os.path.join(current_dir, "bvd_parameters.txt")
    
    if not os.path.exists(params_file):
        print(f"Error: {params_file} not found!")
        print("Please run 'python bvd_extractor.py' first to generate the parameters.")
        exit(1)
        
    with open(params_file, "r") as f:
        data = json.load(f)
        
    C0_global = data["C0_global"]
    my_modes = data["modes"]

    # 2. Instantiate the multi-mode crystal model
    my_oscillator = MultiModeQuartzCrystal(C0_global, my_modes)

    # 3. Model Testing & Visualization
    freqs = np.linspace(3e6, 12e6, 100000)
    Z_model = my_oscillator.get_impedance(freqs)
    Z_magnitude = np.abs(Z_model)
    
    plt.figure(figsize=(12, 6))
    plt.plot(freqs / 1e6, 20 * np.log10(Z_magnitude), color='blue', linewidth=1)
    
    for mode, params in my_modes.items():
        plt.axvline(x=params['f']/1e6, color='red', linestyle='--', alpha=0.5)
        plt.text(params['f']/1e6, np.max(20 * np.log10(Z_magnitude)), f" {mode}", 
                 rotation=90, verticalalignment='top', color='red')

    plt.title('Simulated Quartz Crystal Impedance (Multi-Mode Model)')
    plt.xlabel('Frequency [MHz]')
    plt.ylabel('Impedance Magnitude [dBΩ]')
    plt.xticks(np.arange(3.0, 12.5, 0.5))
    plt.grid(True, which='both', linestyle='--', linewidth=0.5)
    plt.tight_layout()
    plt.show()