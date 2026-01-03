import serial
import time
import matplotlib.pyplot as plt
import numpy as np
import keyboard
import re
import sys
from serial.tools import list_ports

# --- KONSTANTE ---
V_REF = 3.3  # Napon referenci (promijeni ako je 2.5 ili slično)
BUFFER_SIZE = 4096 
ESTIMATED_FS = 65000000
ADC_MASK = 0xFFF
ADC_CH0_SHIFT = 16
ADC_CH1_SHIFT = 0

def find_usb_serial_port():
    ports = list_ports.comports()
    for port in ports:
        if 'USB' in port.device.upper(): return port.device
    return '/dev/ttyUSB0'

SERIAL_PORT = find_usb_serial_port()
BAUDRATE = 115200

ComPort = None
try:
    ComPort = serial.Serial(SERIAL_PORT, timeout=0.5) 
    ComPort.baudrate = BAUDRATE
    ComPort.bytesize = serial.EIGHTBITS
    ComPort.parity   = serial.PARITY_NONE
    ComPort.stopbits = serial.STOPBITS_ONE
    print(f"[INFO] Povezano na: {ComPort.name} @ {BAUDRATE} bps")
except Exception as e:
    print(f"[ERROR] Neuspješno povezivanje: {e}")
    sys.exit(1)

def read_samples_from_stream():
    raw_samples = []
    print("\n[INFO] Citanje bafera je aktivirano. Ocekujem podatke...")
    start_time = time.time()
    READING_TIMEOUT = 50.0 
    
    while True:
        try:
            line = ComPort.readline().decode('utf-8', errors='ignore').strip()
        except: break 
        
        if not line:
            if time.time() - start_time > READING_TIMEOUT: break
            continue
            
        is_hex_data = len(line) == 8 and re.match("^[0-9a-fA-F]+$", line)
        if line == "=== BRAM_TRANSFER_END ===":
            print(f"[INFO] Zavrseno citanje ({len(raw_samples)} uzoraka).")
            break
        
        if not line.startswith("===") and not is_hex_data:
            print(f"[UART] {line}")
            
        if is_hex_data:
            try:
                raw_samples.append(int(line, 16))
            except ValueError: pass 

    read_time = time.time() - start_time
    print(f"[INFO] Ukupan transfer trajao {read_time:.2f}s.")
    return raw_samples, read_time

def unpack_and_analyze(raw_samples, fs):
    # Konverzija u VOLTE direktno prilikom raspakivanja
    # Formula: (raw / 4095) * V_REF
    samples_ch0 = [((raw >> ADC_CH0_SHIFT) & ADC_MASK) / 4095.0 * V_REF for raw in raw_samples]
    samples_ch1 = [((raw >> ADC_CH1_SHIFT) & ADC_MASK) / 4095.0 * V_REF for raw in raw_samples]
        
        
    def analyze_fft(samples):
        if len(samples) < 64: return 0, None, None
        N = len(samples)
        # Uklanjanje DC komponente (srednje vrijednosti) da peak ne bude na 0Hz
        samples_ac = np.array(samples) - np.mean(samples)
        windowed = samples_ac * np.hanning(N)
        fft_data = np.fft.fft(windowed)
        fft_freqs = np.fft.fftfreq(N, 1/fs)
        
        pos_freqs = fft_freqs[:N//2]
        pos_fft_mag = np.abs(fft_data[:N//2])
        
        # Ignorišemo prve binove (DC ostatak) pri traženju peaka
        start_idx = max(1, N // 100)
        peak_idx = np.argmax(pos_fft_mag[start_idx:]) + start_idx
        peak_freq = pos_freqs[peak_idx]
        
        # Normalizacija za dB prikaz
        fft_mag_norm = pos_fft_mag / (np.max(pos_fft_mag) + 1e-10)
        fft_db = 20 * np.log10(fft_mag_norm + 1e-10)
        return peak_freq, pos_freqs, fft_db

    freq0, freqs0, fft_db0 = analyze_fft(samples_ch0)
    freq1, freqs1, fft_db1 = analyze_fft(samples_ch1)
    return samples_ch0, samples_ch1, freq0, freq1, freqs0, fft_db0, freqs1, fft_db1

# --- Grafika ---
plt.ion()
fig, axs = plt.subplots(4, 1, figsize=(14, 16))
plt.subplots_adjust(hspace=0.4) 
cycle_count = 0

try:
    while True:
        incoming_data = ComPort.read_all()
        if incoming_data:
            decoded_text = incoming_data.decode('utf-8', errors='ignore')
            if "=== BRAM_TRANSFER_START ===" in decoded_text:
                raw_samples, read_duration = read_samples_from_stream()
                
                if len(raw_samples) < 100: continue
                cycle_count += 1
                
                (ch0_v, ch1_v, freq0, freq1, freqs0, fft_db0, freqs1, fft_db1) = unpack_and_analyze(raw_samples, ESTIMATED_FS)
                print(f"[REZULTAT] CH0 Peak: {freq0:.1f}Hz | CH1 Peak: {freq1:.1f}Hz")

                time_axis = np.arange(len(ch0_v)) / ESTIMATED_FS

                for i in range(0, len(ch0_v)):
                    ch0_v[i] -= V_REF/2
                for i in range(0, len(ch1_v)):
                    ch1_v[i] -= V_REF/2 
                
                # Plotovanje CH0
                axs[0].clear()
                axs[0].plot(time_axis, ch0_v, 'b-', linewidth=0.7)
                axs[0].set_title(f"ADC kanal 0 [V] | Peak: {freq0:.1f} Hz")
                axs[0].set_ylabel("Napon [V]")
                axs[0].grid(True, alpha=0.3)
                
                axs[1].clear()
                axs[1].plot(freqs0, fft_db0, 'b-')
                axs[1].set_ylim(-80, 5)
                axs[1].set_title("Kanal 0 FFT Spektar")

                # Plotovanje CH1
                axs[2].clear()
                axs[2].plot(time_axis, ch1_v, 'r-', linewidth=0.7)
                axs[2].set_title(f"ADC kanal 1 [V] | Peak: {freq1:.1f} Hz")
                axs[2].set_ylabel("Napon [V]")
                axs[2].grid(True, alpha=0.3)
                
                axs[3].clear()
                axs[3].plot(freqs1, fft_db1, 'r-')
                axs[3].set_ylim(-80, 5)
                axs[3].set_title("Kanal 1 FFT Spektar")

                plt.draw()
            else:
                lines = decoded_text.split('\n')
                for line in lines:
                    if line.strip(): print(f"[UART] {line.strip()}")
                        
        plt.pause(0.01)
        if keyboard.is_pressed('q'): break
            
except Exception as e:
    print(f"\n[ERROR] {e}")
finally:
    if ComPort: ComPort.close()
