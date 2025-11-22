# uart_capture_with_config_fft.py
import serial, time, sys
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from io import StringIO

PORT   = "/dev/ttyUSB0"
BAUD   = 115200
PROMPT = "uberClock>"
HEADER = "index,value"
FS_HZ  = 65e6   # 65 MHz sample clock

# ---- choose the NCO tuning word you send (19-bit phase accumulator) ----
K_NCO = 800  # expect f ≈ FS * K / 2^19  -> ≈ 65e6*800/524288 ≈ 99.18 kHz

PRE_CMDS = [
    "input_select 1",                # <-- NCO as source (0=ADC)
    f"phase_nco {K_NCO}",
    "hs_cfg 0x40400000 262144",      # base/size (words, not samples)
    "hs_clear",
    "hs_mode 1",                     # snapshot
    "hs_arm",
]
CAPTURE_CMD = "hs_dump_csv 0 4096"   # dump 4096 *samples* starting at sample 0

def open_ser(): return serial.Serial(PORT, BAUD, timeout=0.2)

def read_until_prompt(ser, timeout_s=2.0):
    t0 = time.time(); buf=[]
    while time.time()-t0 < timeout_s:
        line = ser.readline().decode(errors="ignore").strip()
        if not line: continue
        buf.append(line)
        if line.startswith(PROMPT): return "\n".join(buf)
    return "\n".join(buf)

def send_cmd(ser, cmd, show=False):
    ser.write((cmd+"\n").encode("ascii")); ser.flush()
    out = read_until_prompt(ser, timeout_s=3.0)
    if show: print(f"--- {cmd} ---\n{out}\n")
    return out

# --- NEW: poll hs_stat until done=1 ---
def hs_stat_text(ser):
    ser.write(b"hs_stat\n"); ser.flush()
    lines=[]
    t_end = time.time()+0.4
    while time.time() < t_end:
        line = ser.readline().decode(errors="ignore").strip()
        if not line: continue
        lines.append(line)
        if line.startswith(PROMPT): break
    return "\n".join(lines)

def wait_hs_done(ser, timeout_s=1.0):
    t0 = time.time()
    while time.time()-t0 < timeout_s:
        if "done=1" in hs_stat_text(ser):
            return True
        time.sleep(0.001)
    return False

def grab_csv_block(ser, capture_cmd, header=HEADER, wait_header_s=3.0):
    ser.reset_input_buffer(); ser.reset_output_buffer()
    ser.write((capture_cmd+"\n").encode("ascii")); ser.flush()
    start = time.time(); lines=[]
    while time.time()-start < wait_header_s:
        line = ser.readline().decode(errors="ignore").strip()
        if not line: continue
        if line.startswith(header):
            lines.append(line); break
    if not lines: return None
    idle=0
    while True:
        line = ser.readline().decode(errors="ignore").strip()
        if not line:
            idle += 1
            if idle > 10: break
            continue
        idle = 0
        if line.startswith(PROMPT): break
        lines.append(line)
    return "\n".join(lines)

def dominant_freq_fft(y, fs_hz):
    N = len(y)
    if N < 8: return 0.0, 0.0
    y = y - np.mean(y)
    w = np.hanning(N)
    Y = np.fft.rfft(y * w)
    freqs = np.fft.rfftfreq(N, 1.0/fs_hz)
    mag = np.abs(Y);
    if len(mag) > 1: mag[0] = 0.0
    k = int(np.argmax(mag))
    return float(freqs[k]), float(mag[k])

def main():
    try:
        ser = open_ser()
    except Exception as e:
        print(f"[ERROR] Could not open {PORT}: {e}")
        print("Close any other terminal app that might be using the port.")
        sys.exit(1)

    with ser:
        time.sleep(0.1)
        ser.reset_input_buffer(); ser.reset_output_buffer()
        read_until_prompt(ser, timeout_s=0.5)

        # configure + arm snapshot
        for c in PRE_CMDS:
            send_cmd(ser, c, show=True)

        # >>> WAIT FOR SNAPSHOT TO FINISH <<<
        if not wait_hs_done(ser, timeout_s=1.0):
            print("[ERROR] HS snapshot didn’t finish in time")
            sys.exit(2)

        # now it's safe to read the buffer
        csv_text = grab_csv_block(ser, CAPTURE_CMD)
        if not csv_text or HEADER not in csv_text:
            print("[ERROR] Did not receive CSV.")
            sys.exit(2)

    # parse CSV
    df = pd.read_csv(StringIO(csv_text))
    df["index"] = pd.to_numeric(df["index"], errors="coerce")
    df["value"] = pd.to_numeric(df["value"], errors="coerce")
    df = df.dropna(subset=["index", "value"]).astype({"index": "int64", "value": "int16"})

    # If your firmware now prints SIGNED 16-bit (recommended), keep as-is:
    s = df["value"].to_numpy().astype(np.int16)

    # If your firmware still prints zero-extended 12-bit, use this instead:
    # s = df["value"].astype(np.int32).to_numpy()
    # s[s >= 2048] -= 4096
    # s = s.astype(np.int16)

    # time plot
    plt.figure()
    plt.plot(s)
    plt.xlabel("Sample index")
    plt.ylabel("Value (signed 12b)")
    plt.title(CAPTURE_CMD)
    plt.grid(True); plt.tight_layout()
    plt.savefig("capture.png", dpi=150)
    print("Saved plot to capture.png")

    # spectrum + dominant frequency
    f_peak, mag = dominant_freq_fft(s.astype(float), FS_HZ)
    f_expected = FS_HZ * (K_NCO / (2**19))
    print(f"Dominant frequency (FFT): {f_peak:,.3f} Hz  |  Expected ≈ {f_expected:,.3f} Hz")

    N = len(s)
    w = np.hanning(N)
    Y = np.fft.rfft((s - np.mean(s)) * w)
    freqs = np.fft.rfftfreq(N, 1.0/FS_HZ)
    plt.figure()
    plt.semilogy(freqs, np.abs(Y) + 1e-12)
    plt.xlabel("Frequency (Hz)")
    plt.ylabel("|FFT|")
    plt.title(f"Spectrum (N={N})  Peak ≈ {f_peak:,.3f} Hz")
    plt.grid(True); plt.tight_layout()
    plt.savefig("spectrum.png", dpi=150)
    print("Saved spectrum to spectrum.png")

    df.to_csv("capture.csv", index=False)
    print("Saved CSV to capture.csv")

if __name__ == "__main__":
    main()
