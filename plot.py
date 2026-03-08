import csv
import sys
import matplotlib.pyplot as plt

CSV_FILE = "results.csv"
OUTPUT_FILE = "figure1.png"

time_ms = []
victim_tec = []
adv_tec = []
victim_state = []

try:
    with open(CSV_FILE, newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            time_ms.append(float(row["time_ms"]))
            victim_tec.append(int(row["victim_tec"]))
            adv_tec.append(int(row["adversary_tec"]))
            victim_state.append(int(row["victim_state"]))
except FileNotFoundError:
    print(f"Error: '{CSV_FILE}' not found. Run 'make run' first.")
    sys.exit(1)

# Phase boundaries
#victim enters error passive
phase1_end_ms = None
for t, s in zip(time_ms, victim_state):
    if s >= 1 and phase1_end_ms is None:
        phase1_end_ms = t

# Bus-off time
busoff_ms = None
for t, s in zip(time_ms, victim_state):
    if s == 2:
        busoff_ms = t
        break

# Plot
fig, ax = plt.subplots(figsize=(8, 5))

ax.plot(time_ms, victim_tec, color="blue", linestyle="-", linewidth=1.8, label="Victim's TEC", zorder=3)
ax.plot(time_ms, adv_tec, color="red", linestyle="--", linewidth=1.8, label="Adversary's TEC", zorder=3)

# Threshold lines
ax.axhline(y=127, color="grey", linestyle=":", linewidth=1.0, alpha=0.7, zorder=1)
ax.text(time_ms[-1] * 0.035, 130, "Error-Passive (TEC > 127)", fontsize=8, color="grey")

ax.axhline(y=255, color="grey", linestyle=":", linewidth=1.0, alpha=0.7, zorder=1)
ax.text(time_ms[-1] * 0.75, 258, "Bus-Off (TEC > 255)", fontsize=8, color="grey")

# Annotate phase boundary
if phase1_end_ms is not None:
    ax.axvline(x=phase1_end_ms, color="orange", linestyle="--", linewidth=0.9, alpha=0.8, zorder=2)
    ax.text(phase1_end_ms + time_ms[-1] * 0.01, 20, "Phase 1->2", fontsize=8, color="darkorange")

# Annotate bus-off event
if busoff_ms is not None:
    ax.axvline(x=170.2, color="red", linestyle=":", linewidth=1.2, alpha=0.6, zorder=2)
    ax.text(busoff_ms - time_ms[-1] * 0.12, 20, f"Bus off\n({busoff_ms:.1f} ms)", fontsize=8, color="red")

# Labels
ax.set_xlabel("Elapsed Time [ms]", fontsize=11)
ax.set_ylabel("Transmit Error Count (TEC)", fontsize=11)
ax.set_title("TECs during a bus-off attack\n", fontsize=11)
ax.legend(loc="upper left", fontsize=10)
ax.set_xlim(left=0)
ax.set_ylim(bottom=-10, top=max(max(victim_tec), max(adv_tec)) + 20)
ax.grid(True, alpha=0.3)

plt.tight_layout()
plt.savefig(OUTPUT_FILE, dpi=150, bbox_inches="tight")
print(f"Figure saved to : {OUTPUT_FILE}")
plt.show()