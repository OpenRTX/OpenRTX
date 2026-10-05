#! /usr/bin/env python3
#
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors

import pandas as pd
from matplotlib import pyplot as plt
from sys import argv

plt.rcParams["figure.autolayout"] = True
df = pd.read_csv(argv[1])
print("Contents in csv file:\n", df)
plt.plot(df.index, df.Sample,              label="Sample")
plt.plot(df.index, df.Convolution / 10,    label="Conv")
plt.plot(df.index, df.Threshold / 10,      label="Conv. Th+")
plt.plot(df.index, df.Threshold * -1 / 10, label="Conv. Th-")
plt.plot(df.index, df.Index,               label="Index")
plt.plot(df.index, df.Max,                 label="Qnt. avg. +")
plt.plot(df.index, df.Min,                 label="Qnt. avg. -")
plt.plot(df.index, df.Symbol * 10,         label="Symbol")
plt.plot(df.index, df.I,                   label="I")
plt.plot(df.index, df.Flags * 100,         label="Flags")
plt.grid(True)
plt.legend(loc="upper left")
plt.suptitle(argv[1])
plt.show()
