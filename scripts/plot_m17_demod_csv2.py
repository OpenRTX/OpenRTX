#! /usr/bin/env python3

import pandas as pd
from matplotlib import pyplot as plt
from sys import argv

plt.rcParams["figure.autolayout"] = True
df = pd.read_csv(argv[1])
#df = df.head(n=10000)
print("Contents in csv file:\n", df)
plt.plot(df.index, df.RRCSignal)
plt.plot(df.index, df.SyncDetect)
plt.plot(df.index, df.QntMax)
plt.plot(df.index, df.QntMin)
plt.plot(df.index, df.Symbol)
plt.show()
