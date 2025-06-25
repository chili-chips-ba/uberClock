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
buffer_size = 100
fs = 300  # Hz

# --- Priprema grafa sa dva subplot-a ---
plt.ion()
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 8))

# Gornji graf - vremenski domen
ax1.set_ylim(-1.0, 1.0)
ax1.set_title(f"Rekonstrukcija signala iz {buffer_size} uzoraka (Fs = {fs} Hz)")
ax1.set_xlabel("Vrijeme [s]")
ax1.set_ylabel("Amplituda")
ax1.grid(True)

# Donji graf - frekvencijski domen (FFT)
ax2.set_title("FFT - Frekvencijski spektar")
ax2.set_xlabel("Frekvencija [Hz]")
ax2.set_ylabel("Magnituda [dB]")
ax2.grid(True)

plt.tight_layout()

# --- Funkcija za čitanje 32-bitne vrijednosti sa adrese ---
def read_adc_value():
    # Ulazak u read mode
    ComPort.write(struct.pack('B', C_BUSR))
    ComPort.write(struct.pack('B', 0x18))  # ADDR0
    ComPort.write(struct.pack('B', 0x00))  # ADDR1
    ComPort.write(struct.pack('B', 0x00))  # ADDR2
    ComPort.write(struct.pack('B', 0x20))  # ADDR3
    
    data = ComPort.read(5)
    if len(data) != 5:
        return None
    
    raw = (
        (data[0] << 24) |
        (data[1] << 16) |
        (data[2] << 8)  |
        (data[3])
    )
    
    signed_value = ctypes.c_int32(raw).value
    scaled_value = signed_value / float(2**31)
    return scaled_value

# --- Inicijalizacija komunikacije ---
print("[INFO] Pokrećem komunikaciju...")
ComPort.write(struct.pack('B', C_SOP))
time.sleep(0.1)
print(f"[INFO] Prikupljam {buffer_size} uzoraka po ciklusu, Fs = {fs} Hz...")

try:
    while True:
        if keyboard.is_pressed('q'):
            print("[INFO] 'q' pritisnuto. Zatvaram...")
            break
        
        buffer = []
        while len(buffer) < buffer_size:
            val = read_adc_value()
            if val is not None:
                buffer.append(val)
        
        # Rekonstrukcija signala
        t_original = np.arange(buffer_size) / fs
        y_original = np.array(buffer)
        
        f_interp = interp1d(t_original, y_original, kind='quadratic')
        t_dense = np.linspace(t_original[0], t_original[-1], 1000)
        y_dense = f_interp(t_dense)
        
        # FFT analiza sa prozorom za bolju rezoluciju
        # Dodajemo Hanning prozor da smanjimo spektralne curenja
        windowed_signal = y_original * np.hanning(len(y_original))
        fft_data = np.fft.fft(windowed_signal)
        fft_freqs = np.fft.fftfreq(len(y_original), 1/fs)
        
        # Uzimamo samo pozitivne frekvencije
        positive_freqs = fft_freqs[:len(fft_freqs)//2]
        fft_magnitude = np.abs(fft_data[:len(fft_data)//2])
        
        # Normalizujemo i konvertujemo u dB
        fft_magnitude_norm = fft_magnitude / np.max(fft_magnitude + 1e-10)
        fft_db = 20 * np.log10(fft_magnitude_norm + 1e-10)
        
        # Crtanje vremenskog domena (gornji graf)
        ax1.clear()
        ax1.plot(t_dense, y_dense, label="Rekonstruisani signal", color='blue')
        ax1.plot(t_original, y_original, 'o', label="Uzorci", color='red', markersize=3)
        ax1.set_ylim(-1.0, 1.0)
        ax1.set_xlim(t_original[0], t_original[-1])
        ax1.set_title(f"Rekonstrukcija signala (Fs = {fs} Hz)")
        ax1.set_xlabel("Vrijeme [s]")
        ax1.set_ylabel("Amplituda")
        ax1.grid(True)
        ax1.legend()
        
        # Crtanje FFT spektra (donji graf) - fokus na 0-200Hz
        ax2.clear()
        
        # Ograničimo prikaz na 0-200Hz gdje očekujete signal
        freq_limit = 400
        freq_mask = positive_freqs <= freq_limit
        limited_freqs = positive_freqs[freq_mask]
        limited_fft_db = fft_db[freq_mask]
        
        ax2.plot(limited_freqs, limited_fft_db, color='green', linewidth=1.5)
        ax2.set_xlim(0, freq_limit)
        ax2.set_ylim(-60, 5)  # Fiksiran opseg za stabilniji prikaz
        ax2.set_title("FFT Spektar (0-200Hz)")
        ax2.set_xlabel("Frekvencija [Hz]")
        ax2.set_ylabel("Relativna magnituda [dB]")
        ax2.grid(True, alpha=0.3)
        
        # Pronađi i označi sve značajne peak-ove
        # Tražimo peak-ove iznad -20dB threshold-a
        threshold_db = -20
        peaks = []
        
        # Ignoriši DC (prva 3 bin-a) i pronađi peak-ove
        start_idx = 3
        for i in range(start_idx, len(limited_fft_db)-1):
            if (limited_fft_db[i] > threshold_db and 
                limited_fft_db[i] > limited_fft_db[i-1] and 
                limited_fft_db[i] > limited_fft_db[i+1]):
                peaks.append((limited_freqs[i], limited_fft_db[i]))
        
        # Sortiraj peak-ove po amplitudi (najveći prvi)
        peaks.sort(key=lambda x: x[1], reverse=True)
        
        # Prikaži top 3 peak-a
        colors = ['red', 'orange', 'purple']
        for idx, (freq, mag) in enumerate(peaks[:3]):
            ax2.axvline(x=freq, color=colors[idx], linestyle='--', alpha=0.8, linewidth=2)
            ax2.plot(freq, mag, 'o', color=colors[idx], markersize=8)
            ax2.text(freq, mag + 3, f'{freq:.1f}Hz\n{mag:.1f}dB', 
                    ha='center', va='bottom', fontweight='bold',
                    bbox=dict(boxstyle='round,pad=0.3', facecolor=colors[idx], alpha=0.3))
        
        plt.tight_layout()
        plt.pause(0.01)

except KeyboardInterrupt:
    print("[INFO] Keyboard interrupt detektovan. Izlazim...")

# --- Zatvaranje komunikacije ---
ComPort.write(struct.pack('B', C_EOP))
print("[INFO] Poslat C_EOP i zatvoren port.")
ComPort.close()
