# work adapted from CS251 and 
# Data Science Using Python and R. (2019) - Chantal D. Larose 

# pip install these in terminal if you haven't 
# "pip install pandas"
# "pip install matplotlib"

import pandas as pd
import matplotlib.pyplot as plt

# Load the CSV file
outputData = pd.read_csv("output.csv")

# Plot Speed and SOC over Time
plt.figure(figsize=(12, 6))

# Speed Plot
plt.subplot(2, 1, 1)
plt.plot(outputData["Time"], outputData["Speed"], label="Speed in meters/second", color="red")
plt.ylabel("Speed in meters/second")
plt.title("EV Speed and SOC Over Time")
plt.grid(True)
plt.legend()

# SOC Plot
plt.subplot(2, 1, 2)
plt.plot(outputData["Time"], outputData["SOC"], label="State of Charge (%)", color="green")
plt.xlabel("Time in seconds")
plt.ylabel("SOC (%)")
plt.grid(True)
plt.legend()

plt.tight_layout()
plt.show()
