import serial
import time
import matplotlib.pyplot as plt
import numpy as np
import keyboard
import re
import sys
from serial.tools import list_ports

# =======================================================================
# PYTHON SKRIPTA ZA ČITANJE BRAM PODATAKA PUTEM UART-a I FFT ANALIZU
# (FIXED: Tolerantno čitanje)
# =======================================================================

# --- UART Konfiguracija ---
# AUTOMATSKO PRONALAŽENJE PORTA
def find_usb_serial_port():
    ports = list_ports.comports()
    for port in ports:
        if 'USB' in port.device.upper():  
            return port.device
    return '/dev/ttyUSB0' # Default ako nista nije pronađeno

SERIAL_PORT = find_usb_serial_port()
BAUDRATE = 115200

ComPort = None
try:
    # Povećan timeout da bi se omogućilo duže čekanje
    ComPort = serial.Serial(SERIAL_PORT, timeout=0.5) 
    ComPort.baudrate = BAUDRATE
    ComPort.bytesize = serial.EIGHTBITS
    ComPort.parity   = serial.PARITY_NONE
    ComPort.stopbits = serial.STOPBITS_ONE
    print(f"[INFO] Povezano na: {ComPort.name} @ {BAUDRATE} bps")
except serial.SerialException as e:
    print(f"[ERROR] Neuspješno povezivanje na UART port '{SERIAL_PORT}': {e}")
    print("[ERROR] Podesite ispravan port i pokrenite ponovo.")
    sys.exit(1)

# --- HARDVERSKE KONSTANTE ---
BUFFER_SIZE = 4096 
ESTIMATED_FS = 65000000 / 128
# ... (Ostale konstante ostaju iste) ...
ADC_MASK = 0xFFF
ADC_CH0_SHIFT = 16
ADC_CH1_SHIFT = 0


# --- FUNKCIJE ZA ČITANJE I ANALIZU ---

def read_samples_from_stream():
    """
    Čita sirove heksadecimalne uzorke direktno iz UART stream-a.
    Tolerantno čita sve uzorke dok ne naiđe na kraj bloka ili timeout.
    """
    raw_samples = []
    
    print("\n[INFO] Citanje bafera je aktivirano. Ocekujem podatke...")
    start_time = time.time()
    
    # Tolerantan timeout za čitanje sporog transfera
    READING_TIMEOUT = 50.0 # Povecan na 50 sekundi
    
    while True:
        # Čita po liniju. Timeout je sada 0.5s.
        try:
            line = ComPort.readline().decode('utf-8', errors='ignore').strip()
        except serial.SerialTimeoutException:
            # Prekid ako je transfer završen, a nema više podataka
            break 
        
        # Ako nema linije i isteklo je vrijeme:
        if not line:
            if time.time() - start_time > READING_TIMEOUT:
                print(f"[WARNING] Timeout: Prekinuto citanje. Primljeno {len(raw_samples)}/{BUFFER_SIZE}.")
                break
            continue
            
        # Prikaz UART dijagnostike (samo ako nije hex podatak)
        is_hex_data = len(line) == 8 and re.match("^[0-9a-fA-F]+$", line)
        
        # Provjera završnog signala
        if line == "=== BRAM_TRANSFER_END ===":
            print(f"[INFO] Zavrseno citanje ({len(raw_samples)} uzoraka).")
            break
        
        if not line.startswith("===") and not is_hex_data:
            print(f"[UART] {line}")
            
        if is_hex_data:
            try:
                # Konvertuj heks string u integer
                raw_samples.append(int(line, 16))
            except ValueError:
                pass 

    read_time = time.time() - start_time
    print(f"[INFO] Ukupan transfer trajao {read_time:.2f}s.")
    
    return raw_samples, read_time


def unpack_and_analyze(raw_samples, fs):
    # ... (Ova funkcija ostaje ista) ...
    
    samples_ch0 = []
    samples_ch1 = []
    
    for raw in raw_samples:
        ch0_value = (raw >> ADC_CH0_SHIFT) & ADC_MASK
        samples_ch0.append(ch0_value)
        ch1_value = (raw >> ADC_CH1_SHIFT) & ADC_MASK
        samples_ch1.append(ch1_value)
        
    def analyze_fft(samples):
        if len(samples) < 64: return 0, 0, None, None
        N = len(samples)
        samples_ac = np.array(samples) - np.mean(samples)
        windowed = samples_ac * np.hanning(N)
        fft_data = np.fft.fft(windowed)
        fft_freqs = np.fft.fftfreq(N, 1/fs)
        pos_freqs = fft_freqs[:N//2]
        pos_fft_data = fft_data[:N//2]
        start_idx = max(1, N // 100)
        peak_idx = np.argmax(np.abs(pos_fft_data[start_idx:])) + start_idx
        peak_freq = pos_freqs[peak_idx]
        fft_mag_norm = np.abs(pos_fft_data) / (np.max(np.abs(pos_fft_data)) + 1e-10)
        fft_db = 20 * np.log10(fft_mag_norm + 1e-10)
        return peak_freq, pos_freqs, fft_db

    freq0, freqs0, fft_db0 = analyze_fft(samples_ch0)
    freq1, freqs1, fft_db1 = analyze_fft(samples_ch1)
    
    return samples_ch0, samples_ch1, freq0, freq1, freqs0, fft_db0, freqs1, fft_db1


# --- Glavna petlja ---
# ... (Ispis zaglavlja ostaje isti) ...

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
                print("\n[INFO] START signal primljen. Pokrecem citanje bafera...")
                
                # Cekaj na podatke (ova funkcija blokira dok ne procita sve)
                raw_samples, read_duration = read_samples_from_stream()
                
                # *** MODIFIKOVANA PROVJERA PLOTOVANJA ***
                MIN_SAMPLES_TO_PLOT = 100
                if len(raw_samples) < MIN_SAMPLES_TO_PLOT:
                    print(f"[WARNING] Premalo uzoraka ({len(raw_samples)}). Preskacem plotovanje.")
                    continue
                
                if len(raw_samples) != BUFFER_SIZE:
                    print(f"[WARNING] Plotujem sa {len(raw_samples)} umjesto {BUFFER_SIZE} uzoraka.")
                    
                cycle_count += 1
                
                # --- ANALIZA --- (ostaje ista)
                (ch0_samples, ch1_samples, freq0, freq1, freqs0, fft_db0, freqs1, fft_db1) = unpack_and_analyze(raw_samples, ESTIMATED_FS)

                print(f"[REZULTAT] CH0 Peak: {freq0:.1f}Hz | CH1 Peak: {freq1:.1f}Hz | Citano u: {read_duration*1000:.1f}ms")

                # --- CRTANJE GRAFOVA (Originalna logika) ---
                # ... (Logika crtanja grafova ostaje ista) ...
                time_axis = np.arange(len(ch0_samples)) / ESTIMATED_FS
                
                # A. Graf 1: CH0 VREF Time Domain
                axs[0].clear()
                axs[0].plot(time_axis, ch0_samples, 'b-', linewidth=1, alpha=0.8)
                axs[0].set_xlim(0, time_axis[-1])
                axs[0].set_title(f"Kanal 0 (VREF) - Vremenski domen | Detektovana Freq: {freq0:.1f} Hz | Ciklus: {cycle_count}")
                axs[0].set_xlabel("Vrijeme [s]")
                axs[0].set_ylabel("Amplituda (raw)")
                axs[0].grid(True, alpha=0.3)
                
                # B. Graf 2: CH0 VREF FFT Spektar
                axs[1].clear()
                if freqs0 is not None and fft_db0 is not None:
                    axs[1].plot(freqs0, fft_db0, 'b-', linewidth=1.5)
                    axs[1].axvline(x=freq0, color='red', linestyle='--', linewidth=2, label=f'Peak: {freq0:.1f} Hz')
                    axs[1].set_xlim(0, ESTIMATED_FS / 2)
                    axs[1].set_ylim(-80, 10)
                    axs[1].set_title("Kanal 0 (VREF) - FFT Spektar")
                    axs[1].set_xlabel("Frekvencija [Hz]")
                    axs[1].set_ylabel("Magnituda [dB]")
                    axs[1].grid(True, alpha=0.3)

                # C. Graf 3: CH1 VOUT Time Domain
                axs[2].clear()
                axs[2].plot(time_axis, ch1_samples, 'r-', linewidth=1, alpha=0.8)
                axs[2].set_xlim(0, time_axis[-1])
                axs[2].set_title(f"Kanal 1 (VOUT) - Vremenski domen | Detektovana Freq: {freq1:.1f} Hz")
                axs[2].set_xlabel("Vrijeme [s]")
                axs[2].set_ylabel("Amplituda (raw)")
                axs[2].grid(True, alpha=0.3)
                
                # D. Graf 4: CH1 VOUT FFT Spektar
                axs[3].clear()
                if freqs1 is not None and fft_db1 is not None:
                    axs[3].plot(freqs1, fft_db1, 'r-', linewidth=1.5)
                    axs[3].axvline(x=freq1, color='red', linestyle='--', linewidth=2, label=f'Peak: {freq1:.1f} Hz')
                    axs[3].set_xlim(0, ESTIMATED_FS / 2)
                    axs[3].set_ylim(-80, 10)
                    axs[3].set_title("Kanal 1 (VOUT) - FFT Spektar")
                    axs[3].set_xlabel("Frekvencija [Hz]")
                    axs[3].set_ylabel("Magnituda [dB]")
                    axs[3].grid(True, alpha=0.3)
                    
                plt.tight_layout()
                plt.draw()
            
            else:
                # Ispisuje standardnu dijagnostiku ako nije START signal
                lines = decoded_text.split('\n')
                for line in lines:
                    line = line.strip()
                    if line:
                        print(f"[UART] {line}")
                        
        plt.pause(0.01)
        
        if keyboard.is_pressed('q'):
            print("\n[INFO] 'q' pritisnuto - izlazim...")
            break
            
except KeyboardInterrupt:
    print("\n[INFO] Keyboard interrupt")
except Exception as e:
    print(f"\n[FATAL ERROR] Dogodila se greška: {e}")

# Zatvaranje
if ComPort and ComPort.is_open:
    ComPort.close()

print("\n" + "="*60)
print("ZAVRŠENO")
print("="*60)
print(f"Ukupno ciklusa čitanja: {cycle_count}")
