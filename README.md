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

# Dependencies
- gcc
- make
- python3
- matplotlib

# Build
Just run make from the main directory.
> [!NOTE] It won't compile on Windows because of getopt being a POSIX fuction so either use WSL or switch to Linux ¯\\_(ツ)_/¯

# Output 
To see the plots first run the executable to generate the csv files then run
```
python3 plot.py
```

# Command line arguments
| flag | Arguments | Description |
|:----:|:---------:|-------------|
| -j | us | Jitter range in microseconds (default: 0) |
| -p | - | Use preceded ID Tx sync (default: period-based) |
| -o | file | Output CSV file (default: naive.csv)|
| -h | - | Show help |

Without arguments runs all three preset scenarios.