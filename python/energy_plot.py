import numpy as np
import matplotlib.pyplot as plt
from string import split

def main():
    bands = 7
    for i in range(bands):
        energies = []
        max_vals = []
        with open("cardioid.r.energies.txt") as f:
            for line in f:
                energy = float(split(line)[5 + i * 6])
                max_val = float(split(line)[7 + i * 6])
                energies.append(energy)
                max_vals.append(max_val)

        m = max(energies)
        #energies = [e / m for e in energies]

        theta = np.linspace(0, 2 * np.pi, len(energies), False)

        ax = plt.subplot(241 + i, projection='polar')
        ax.plot(theta, energies, color='r', linewidth = 3)
        #ax.plot(theta, max_vals, color='b', linewidth = 3)
        #ax.set_rmax(max(max(energies), max(max_vals)))
        ax.grid(True)

    plt.show()

if __name__ == "__main__":
    main()