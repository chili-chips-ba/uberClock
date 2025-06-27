import serial
import struct
import time
import matplotlib.pyplot as plt
import numpy as np
import ctypes
import keyboard
from scipy.interpolate import interp1d

# --- Komande ---
C_SOP  = 0x12
C_EOP  = 0x14
C_BUSR = 0x0F

# --- UART Konfiguracija ---
ComPort = serial.Serial('/dev/ttyUSB0')
ComPort.baudrate = 115200
ComPort.bytesize = serial.EIGHTBITS
ComPort.parity   = serial.PARITY_NONE
ComPort.stopbits = serial.STOPBITS_ONE
ComPort.timeout  = 1

# --- Parametri ---
SAMPLES_ARRAY_ADDR = 0x10000000  # Adresa samples[] array-a
BUFFER_SIZE = 1024               # samples[1024]
ADC_BITS = 12                    # 12-bit ADC (11:0 biti)
ADC_MASK = 0xFFF                 # Maska za 11:0 bite (0xFFF = 4095)

# Parametri za frekvenciju uzorkovanja
ESTIMATED_FS = 555600  # Početna procjena (prilagoditi prema vašem sistemu)
BUFFER_TIME = BUFFER_SIZE / ESTIMATED_FS  # Vrijeme da se popuni buffer

# --- Priprema grafa ---
plt.ion()
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(14, 10))

def read_memory_address(address):
    """Čita 32-bitnu vrijednost sa memorijske adrese"""
    ComPort.write(struct.pack('B', C_BUSR))
    ComPort.write(struct.pack('B', (address >> 0) & 0xFF))
    ComPort.write(struct.pack('B', (address >> 8) & 0xFF))
    ComPort.write(struct.pack('B', (address >> 16) & 0xFF))
    ComPort.write(struct.pack('B', (address >> 24) & 0xFF))
    
    data = ComPort.read(5)
    if len(data) != 5:
        return None
    
    # Rekonstruiši 32-bit vrijednost
    raw = (data[3] << 24) | (data[2] << 16) | (data[1] << 8) | data[0]
    return raw

def read_full_buffer():
    """
    Čita cijeli samples[] buffer (1024 uzorka) i izvlači ADC kanal 2 (11:0 biti)
    """
    adc_samples = []
    
    print(f"[INFO] Čitanje {BUFFER_SIZE} uzoraka sa adrese 0x{SAMPLES_ARRAY_ADDR:08X}...")
    start_time = time.time()
    
    for i in range(BUFFER_SIZE):
        address = SAMPLES_ARRAY_ADDR + (i * 4)  # 4 bajta po uint32_t
        
        raw_value = read_memory_address(address)
        if raw_value is not None:
            # Izvuci 11:0 bite (ADC kanal 2)
            adc_value = raw_value & ADC_MASK
            
            # Konvertuj u signed vrijednost (-2048 do +2047 za 12-bit)
            #if adc_value > 2047:
            #   adc_signed = adc_value - 4096
            #else:
            #    adc_signed = adc_value
            
            # Skaliraj na -1.0 do +1.0
            #adc_scaled = adc_signed / 2048.0
            adc_scaled = adc_value
            adc_samples.append(adc_scaled)
        else:
            print(f"[ERROR] Neuspješno čitanje adrese 0x{address:08X}")
            break
        
        # Progress indicator
        if (i + 1) % 128 == 0:
            progress = (i + 1) / BUFFER_SIZE * 100
            print(f"[INFO] Progress: {progress:.1f}%")
    
    read_time = time.time() - start_time
    print(f"[INFO] Čitanje završeno: {len(adc_samples)} uzoraka u {read_time:.2f}s")
    
    return adc_samples, read_time

def estimate_sampling_frequency(samples, buffer_changes):
    """
    Procjenjuje frekvenciju uzorkovanja na osnovu promjena u buffer-u
    """
    if len(buffer_changes) < 2:
        return ESTIMATED_FS
    
    # Koristi vremenski interval između promjena
    time_diffs = np.diff(buffer_changes)
    avg_time = np.mean(time_diffs)
    
    if avg_time > 0:
        estimated_fs = BUFFER_SIZE / avg_time
        if 1000 < estimated_fs < 600000:  # Razumne granice
            return estimated_fs
    
    return ESTIMATED_FS

def quadratic_interpolation(x_points, y_points, num_interpolated=2048):
    """
    Kvadratna interpolacija između tačaka
    """
    if len(x_points) < 3:
        return x_points, y_points
    
    try:
        # Kreiraj interpolacijski objekat (kvadratna interpolacija)
        interp_func = interp1d(x_points, y_points, kind='quadratic', 
                              bounds_error=False, fill_value='extrapolate')
        
        # Kreiraj nove x vrijednosti za interpolaciju
        x_new = np.linspace(x_points[0], x_points[-1], num_interpolated)
        y_new = interp_func(x_new)
        
        return x_new, y_new
    except Exception as e:
        print(f"[WARNING] Greška u interpolaciji: {e}")
        return x_points, y_points

def analyze_adc_signal(samples, fs):
    """
    FFT analiza ADC signala
    """
    if len(samples) < 64:
        return None, None, None
    
    # Ukloni DC komponentu
    samples_ac = np.array(samples) - np.mean(samples)
    
    # Primijeni Hanning prozor
    windowed = samples_ac * np.hanning(len(samples_ac))
    
    # FFT
    fft_data = np.fft.fft(windowed)
    fft_freqs = np.fft.fftfreq(len(samples), 1/fs)
    
    # Samo pozitivne frekvencije
    pos_freqs = fft_freqs[:len(fft_freqs)//2]
    fft_mag = np.abs(fft_data[:len(fft_data)//2])
    
    if len(fft_mag) < 10:
        return None, None, None
    
    # Pronađi peak (ignoriši prvih 1% bin-ova za DC)
    start_idx = max(1, len(fft_mag) // 100)
    peak_idx = np.argmax(fft_mag[start_idx:]) + start_idx
    peak_freq = pos_freqs[peak_idx]
    
    # Konvertuj u dB
    fft_mag_norm = fft_mag / (np.max(fft_mag) + 1e-10)
    fft_db = 20 * np.log10(fft_mag_norm + 1e-10)
    
    return peak_freq, pos_freqs, fft_db

def detect_buffer_activity(current_samples, previous_samples):
    """
    Detektuje da li se buffer mijenja (nova aktivnost)
    """
    if previous_samples is None or len(previous_samples) != len(current_samples):
        return True
    
    # Provjeri razliku
    diff = np.array(current_samples) - np.array(previous_samples)
    max_change = np.max(np.abs(diff))
    
    return max_change > 0.001  # Prag promjene

# --- Inicijalizacija ---
print("=" * 60)
print("RISC-V ADC Buffer Reader - Puni Buffer Mod sa Interpolacijom")
print("=" * 60)

ComPort.write(struct.pack('B', C_SOP))
time.sleep(0.1)

print(f"[INFO] Samples array adresa: 0x{SAMPLES_ARRAY_ADDR:08X}")
print(f"[INFO] Buffer veličina: {BUFFER_SIZE} uzoraka")
print(f"[INFO] ADC bits: {ADC_BITS} (maska: 0x{ADC_MASK:03X})")
print(f"[INFO] Procijenjena Fs: {ESTIMATED_FS} Hz")
print(f"[INFO] Buffer refresh time: {BUFFER_TIME:.3f}s")

# Test čitanja
print("\n[INFO] Test čitanja...")
test_value = read_memory_address(SAMPLES_ARRAY_ADDR)
if test_value is None:
    print("[ERROR] Neuspješno test čitanje! Provjerite konekciju i adresu.")
    exit(1)

test_adc = test_value & ADC_MASK
print(f"[INFO] Test OK - Raw: 0x{test_value:08X}, ADC: {test_adc}")

# Glavna petlja
previous_samples = None
buffer_timestamps = []
estimated_fs = ESTIMATED_FS
cycle_count = 0

try:
    while True:
        print(f"\n{'='*20} CIKLUS {cycle_count + 1} {'='*20}")
        
        # Provjeri tipku 'q'
        if keyboard.is_pressed('q'):
            print("[INFO] 'q' pritisnuto - izlazim...")
            break
        
        cycle_start = time.time()
        
        # ČITAJ CIJELI BUFFER
        current_samples, read_duration = read_full_buffer()
        
        if len(current_samples) < BUFFER_SIZE:
            print(f"[WARNING] Nepotpuno čitanje: {len(current_samples)}/{BUFFER_SIZE}")
            time.sleep(1)
            continue
        
        # Provjeri aktivnost buffer-a
        buffer_active = detect_buffer_activity(current_samples, previous_samples)
        
        if buffer_active:
            buffer_timestamps.append(time.time())
            print("[INFO] Buffer aktivnost detektovana - novi podaci!")
            
            # Procijeni frekvenciju uzorkovanja
            if len(buffer_timestamps) >= 2:
                estimated_fs = estimate_sampling_frequency(current_samples, buffer_timestamps)
                # Ažuriraj buffer refresh time
                BUFFER_TIME = BUFFER_SIZE / estimated_fs
        else:
            print("[INFO] Nema promjene u buffer-u")
        
        # FFT ANALIZA
        peak_freq, freqs, fft_db = analyze_adc_signal(current_samples, estimated_fs)
        
        # CRTANJE GRAFOVA
        if len(current_samples) > 0:
            # Pripremi podatke za prikaz (zadnjih 512 uzoraka)
            display_samples = current_samples[-512:]
            time_axis = np.arange(len(display_samples)) / estimated_fs
            
            # Graf 1: ADC Signal sa interpolacijom
            ax1.clear()
            
            # Crtaj originalne uzorke kao crvene tačke
            ax1.plot(time_axis, display_samples, 'ro', markersize=4, alpha=0.7, label='ADC Uzorci')
            
            # Kvadratna interpolacija
            if len(display_samples) >= 3:
                x_interp, y_interp = quadratic_interpolation(time_axis, display_samples, num_interpolated=2048)
                ax1.plot(x_interp, y_interp, 'b-', linewidth=1.5, alpha=0.8, label='Kvadratna Interpolacija')
            
            # Ukloni y limite da se vidi pravi signal
            ax1.set_xlim(0, time_axis[-1])
            
            title = f"ADC Kanal 2 - Fs: {estimated_fs:.0f}Hz"
            if peak_freq:
                title += f" | Signal Peak: {peak_freq:.1f}Hz"
            title += f" | Ciklus: {cycle_count + 1}"
            
            ax1.set_title(title)
            ax1.set_xlabel("Vrijeme [s]")
            ax1.set_ylabel("ADC Amplituda (raw)")
            ax1.legend()
            ax1.grid(True, alpha=0.3)
            
            # Graf 2: FFT Spektar
            if freqs is not None and fft_db is not None:
                ax2.clear()
                
                # Ograniči frekvenciju
                freq_limit = min(300000, estimated_fs/2)
                freq_mask = freqs <= freq_limit
                
                plot_freqs = freqs[freq_mask]
                plot_fft = fft_db[freq_mask]
                
                ax2.plot(plot_freqs, plot_fft, 'g-', linewidth=1.5)
                ax2.set_xlim(0, freq_limit)
                ax2.set_ylim(-80, 10)
                
                # OZNAČI ADC SIGNAL PEAK
                if peak_freq and peak_freq <= freq_limit:
                    ax2.axvline(x=peak_freq, color='red', linestyle='--', linewidth=3)
                    ax2.plot(peak_freq, -5, 'ro', markersize=12)
                    ax2.text(peak_freq, 5, f'ADC SIGNAL\n{peak_freq:.1f} Hz', 
                            ha='center', va='bottom', fontweight='bold', fontsize=12,
                            bbox=dict(boxstyle='round,pad=0.5', facecolor='yellow', alpha=0.9))
                
                ax2.set_title("FFT Spektar - ADC Kanal 2")
                ax2.set_xlabel("Frekvencija [Hz]")
                ax2.set_ylabel("Magnituda [dB]")
                ax2.grid(True, alpha=0.3)
            
            plt.tight_layout()
            plt.pause(0.1)
        
        # Sačuvaj trenutne uzorke za sljedeće poređenje
        previous_samples = current_samples.copy()
        cycle_count += 1
        
        # ČEKAJ da se buffer osvježi
        print(f"[INFO] Čekam {BUFFER_TIME:.3f}s da se buffer osvježi...")
        
        # Zatvaranje komunikacije (kao što ste rekli)
        ComPort.write(struct.pack('B', C_EOP))
        time.sleep(BUFFER_TIME)
        
        # Ponovo otvaranje komunikacije
        ComPort.write(struct.pack('B', C_SOP))
        time.sleep(0.1)

except KeyboardInterrupt:
    print("\n[INFO] Keyboard interrupt")

# Zatvaranje
ComPort.write(struct.pack('B', C_EOP))
ComPort.close()

print("\n" + "="*60)
print("ZAVRŠENO")
print("="*60)
print(f"Ukupno ciklusa: {cycle_count}")
print(f"Finalna procjena Fs: {estimated_fs:.0f} Hz")
if buffer_timestamps:
    print(f"Buffer refresh rate: {len(buffer_timestamps)/((buffer_timestamps[-1] - buffer_timestamps[0]) or 1):.2f} Hz")
print("="*60)
