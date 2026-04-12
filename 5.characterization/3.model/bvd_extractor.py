import os
import json
import numpy as np
from scipy.optimize import curve_fit

# 1. Parsing function
def read_s2p(filepath):
    freqs, s21_mag_lin = [], []
    with open(filepath, 'r') as f:
        for line in f:
            if line.startswith('!') or line.startswith('#') or not line.strip():
                continue
            parts = line.split()
            if len(parts) >= 9:
                freqs.append(float(parts[0]))
                s21_mag_db = float(parts[3])
                s21_mag_lin.append(10 ** (s21_mag_db / 20.0))
    return np.array(freqs), np.array(s21_mag_lin)

# 2. Mathematical model (BVD equivalent circuit)
def bvd_s21_magnitude(f, R1, L1, C0, fs):
    w = 2 * np.pi * f
    Z_sys = 50.0
    C1 = 1 / ((2 * np.pi * fs)**2 * L1)
    Zm = R1 + 1j * w * L1 + 1 / (1j * w * C1)
    Z0 = 1 / (1j * w * C0)
    Zeq = (Zm * Z0) / (Zm + Z0)
    S21 = (2 * Z_sys) / (2 * Z_sys + Zeq)
    return np.abs(S21)

# 3. Extraction function for a single mode
def extract_parameters_for_mode(base_dir, target_mode):
    print(f"Extracting parameters for {target_mode}...")
    R1_list, L1_list, C0_list, fs_list = [], [], [], []
    successful_files = 0
    
    for root, dirs, files in os.walk(base_dir):
        if target_mode in root:
            for file in files:
                if file.endswith('.s2p'):
                    filepath = os.path.join(root, file)
                    freqs, s21_mag = read_s2p(filepath)
                    if len(freqs) == 0: continue
                        
                    peak_idx = np.argmax(s21_mag)
                    fs_guess = freqs[peak_idx]
                    
                    initial_guess = [50.0, 0.01, 3e-12] 
                    bounds = ([1.0, 1e-4, 1e-13], [500.0, 10.0, 1e-10])
                    fit_func = lambda f, R1, L1, C0: bvd_s21_magnitude(f, R1, L1, C0, fs_guess)
                    
                    try:
                        popt, _ = curve_fit(fit_func, freqs, s21_mag, p0=initial_guess, bounds=bounds)
                        R1_list.append(popt[0])
                        L1_list.append(popt[1])
                        C0_list.append(popt[2])
                        fs_list.append(fs_guess)
                        successful_files += 1
                    except Exception:
                        pass

    if successful_files == 0:
        print(f"  -> No valid data for {target_mode}.")
        return None
        
    R1_global = np.median(R1_list)
    L1_global = np.median(L1_list)
    C0_global = np.median(C0_list)
    fs_ref = np.median(fs_list)
    C1_global = 1 / ((2 * np.pi * fs_ref)**2 * L1_global)
    
    print(f"  -> Success! Processed {successful_files} files.")
    return {
        'R': float(R1_global),
        'L': float(L1_global),
        'C': float(C1_global),
        'f': float(fs_ref),
        'C0_median': float(C0_global)
    }

# 4. Main Batch Processing script
if __name__ == "__main__":
    # Dynamically find the path to '1.raw_data' relative to this script
    current_dir = os.path.dirname(os.path.abspath(__file__))
    base_data_directory = os.path.abspath(os.path.join(current_dir, '..', '1.raw_data'))
    
    modes_to_process = ['C100', 'B100', 'A100', 'C300', 'B300']
    
    final_parameters = {"modes": {}}
    c0_all_modes = []

    print(f"Looking for data in: {base_data_directory}\n")
    
    for mode in modes_to_process:
        result = extract_parameters_for_mode(base_data_directory, mode)
        if result:
            final_parameters["modes"][mode] = {
                'R': result['R'],
                'L': result['L'],
                'C': result['C'],
                'f': result['f']
            }
            c0_all_modes.append(result['C0_median'])
            
    # Calculate global C0 as the average of all modes' median C0
    if c0_all_modes:
        final_parameters["C0_global"] = float(np.mean(c0_all_modes))
    else:
        final_parameters["C0_global"] = 8.36e-12

    # Save to bvd_parameters.txt in standard JSON format
    output_file = os.path.join(current_dir, "bvd_parameters.txt")
    with open(output_file, "w") as f:
        json.dump(final_parameters, f, indent=4)
        
    print(f"\nAll done! Parameters saved to: {output_file}")