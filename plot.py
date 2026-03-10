import csv
import sys
import matplotlib.pyplot as plt

RUNS = [
    ("period_attack_jitter.csv",      "Run 1: Period-based, jitter=30 µs"),
    ("preceded_id_attack_jitter.csv", "Run 2: Preceded ID, jitter=30 µs"),
]
OUTPUT_FILE = "figure_compare.png"


def load_csv(path):
    time_ms, victim_tec, adv_tec, victim_state = [], [], [], []
    try:
        with open(path, newline="") as f:
            for row in csv.DictReader(f):
                time_ms.append(float(row["time_ms"]))
                victim_tec.append(int(row["victim_tec"]))
                adv_tec.append(int(row["adversary_tec"]))
                victim_state.append(int(row["victim_state"]))
    except FileNotFoundError:
        print(f"Error: '{path}' not found. Run the simulation first.")
        sys.exit(1)
    return time_ms, victim_tec, adv_tec, victim_state


for i, (csv_path, label) in enumerate(RUNS):
    time_ms, victim_tec, adv_tec, victim_state = load_csv(csv_path)

    fig, ax = plt.subplots(figsize=(8, 5))
    fig.suptitle("TEC during bus-off attack", fontsize=14)

    ax.plot(time_ms, victim_tec, color="blue",  linewidth=1.8, label="Victim TEC")
    ax.plot(time_ms, adv_tec,    color="red", linestyle="--", linewidth=1.8, label="Adversary TEC")

    # Threshold lines
    ax.axhline(y=127, color="grey", linestyle=":", linewidth=1.0, alpha=0.7)
    ax.axhline(y=255, color="grey", linestyle=":", linewidth=1.0, alpha=0.7)

    # Phase 1 2 boundary
    for t, s in zip(time_ms, victim_state):
        if s >= 1:
            ax.axvline(x=t, color="orange", linestyle="--", linewidth=0.9, alpha=0.8)
            ax.text(t + (time_ms[-1] - time_ms[0]) * 0.01, 15, "Ph 1→2", fontsize=7.5, color="darkorange")
            break

    # Bus-off marker
    for t, s in zip(time_ms, victim_state):
        if s == 2:
            ax.axvline(x=t, color="red", linestyle=":", linewidth=1.2, alpha=0.7)
            ax.text(t - (time_ms[-1] - time_ms[0]) * 0.13, 15,
                    f"Bus-Off\n({t:.1f} ms)", fontsize=7.5, color="red")
            break

    ax.set_title(label, fontsize=10)
    ax.set_xlabel("Elapsed Time [ms]", fontsize=9)
    ax.set_ylabel("TEC", fontsize=9)
    ax.set_xlim(left=0)
    ax.set_ylim(bottom=-10, top=max(max(victim_tec), max(adv_tec)) + 30)
    ax.legend(loc="upper left", fontsize=9)
    ax.grid(True, alpha=0.3)

    out = OUTPUT_FILE.replace(".png", f"_{i+1}.png")
    plt.tight_layout()
    plt.savefig(out, dpi=150, bbox_inches="tight")
    print(f"Figure saved to: {out}")

plt.show()
