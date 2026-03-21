# CAN Bus-Off Attack
Simulation of a CAN Bus-Off Attack where a malicious ECU disconnects a healthy ECU from the CAN Bus via error counter manipulation.

# Description
This simulation implements the most important features of the CAN 2.0 specification such as correct handling of error flags TEC & REC.
It also simulates the jitter caused by signal noise of physical hardware.
There are two ways in which the adversary can synchronize its attacks.
- Period-based: The adversary listen for victim transmissions, then sends its own message according to the victim's message period.
This attack is subject to jitter.
- Preceded ID based: the adversary listens for a "preceded ID" (a message which always comes at a predefined time before the victim's message). It synchronizes its attack by waiting an Intra-Frame Space gap from the end of the preceded message. This way the attack is perfectly synchronized with the victim's message.
This attack is strong even with high jitter.

# Running the simulation

There are two ways to build and run the project.

## Option 1: Native (make)

**Dependencies:** gcc, make, python3, matplotlib

```sh
make
./bus_off_sim.out
python3 plot.py
```

> [!NOTE]
> Won't compile on Windows because `getopt` is a POSIX function — use WSL or Linux ¯\\_(ツ)_/¯

## Option 2: Docker

No dependencies required beyond Docker itself. Output files (CSVs and PNGs) are written to the `./output` folder on your host.

```sh
docker compose run --rm sim    # run the simulation
docker compose run --rm plot   # generate the plots
```

# Command line arguments
| flag | Arguments | Description |
|:----:|:---------:|-------------|
| -j | us | Jitter range in microseconds (default: 0) |
| -p | - | Use preceded ID Tx sync (default: period-based) |
| -o | file | Output CSV file (default: naive.csv)|
| -h | - | Show help |

Without arguments runs all three preset scenarios described in the report.