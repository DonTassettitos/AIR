#!/usr/bin/env python3

# script parameters #

filename = "10000.csv"
image_name = "latency.png"

# auto configuration #

import os
import sys
import pandas
import seaborn
import matplotlib.pyplot as plt

if (len(sys.argv) >= 2): filename = sys.argv[1] 

try:
    script_dirpath = os.path.dirname(__file__)
    filepath = os.path.join(script_dirpath, sensors_filename)
except:
    filepath = filename

if (len(sys.argv) >= 3): image_name = sys.argv[2]

# pandas.options.mode.chained_assignment = None

# functions #
def getData(filename):
    '''
    Extracts the benchmark output :
    - query name (with baseline) -> string
    - number of processes -> integer [1, 2, 4]
    - window id -> integer [0 ; +oo]
    - latency for the current window -> double
    - average latency for the current observation -> double
    Separator is ', '
    '''

    df = pandas.read_csv(filename, sep=',', header=None)
    df.columns = ["query_baseline", "nof_processes", "window_id", "latency", "avg_latency"]

    for qb in df.query_baseline.unique():
        for nproc in df.nof_processes.unique():
            try:
                serie = df.loc[(df.query_baseline == qb) & (df.nof_processes == nproc)]
                m = min(serie.window_id)
                df.loc[(df.query_baseline == qb) & (df.nof_processes == nproc), "window_id"] -= m
            except:
                print("Unable to find serie " + str(qb) + " with " + str(nproc) + " processes")

    return df

def savefig(image_name, df, query="NQ5", nproc=1):
    # plot = seaborn.lineplot(data=df, y='latency', x=df.index, hue='query_baseline')
    if query == "NQ5":
        plot = seaborn.lineplot(data=df.loc[(df.nof_processes == nproc) & (df.query_baseline != "NQ8 (count)") & (df.query_baseline != "NQ8 (sort)") & (df.query_baseline != "NQ8 (flow-wrapping)")], y='latency', x='window_id', hue='query_baseline')

    if query == "NQ8":
        plot = seaborn.lineplot(data=df.loc[(df.nof_processes == nproc) & (df.query_baseline != "NQ5 (count)") & (df.query_baseline != "NQ5 (sort)") & (df.query_baseline != "NQ5 (flow-wrapping)")], y='latency', x='window_id', hue='query_baseline')
    
    plot.set(yscale="log")
    fig = plot.get_figure()
    fig.savefig(image_name)
    plt.clf() # don't keep it in memory

if __name__ == "__main__":
    df = getData(filename)
    savefig(image_name[:-4] + "-NQ5" + image_name[-4:], df, query="NQ5")
    savefig(image_name[:-4] + "-NQ8" + image_name[-4:], df, query="NQ8")

