#! /usr/bin/env python3

import pandas as pd
from matplotlib import pyplot as plt
from sys import argv

plt.rcParams["figure.autolayout"] = True
df = pd.read_csv(argv[1])
print("Contents in csv file:\n", df)
plt.plot(df.index, df.Sample)
plt.plot(df.index, df.Max)
plt.plot(df.index, df.Min)
plt.plot(df.index, df.Symbol)
plt.plot(df.index, df.I)
plt.show()
