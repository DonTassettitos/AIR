#!/usr/bin/env python3

# description : creates latency graphs with results data. (avg latency by node, nof dataflows, throughput)

# auto configuration #

import os
import sys
import pandas
import seaborn
import matplotlib.pyplot as plt

path_to_results = "default" # no / after the name
if (len(sys.argv) >= 2): path_to_results = sys.argv[1] 

# Functions

def load_results(directory_name):
    """
    Input : path to the results files (generated with merge-files.sh)
    Output : pandas table with all the results
    Side-effect : none

    DataFrame format :
    query & baseline, nof dataflows, window id, window latency, avg latency
    """

    nof_nodes=1
    df = pandas.DataFrame()
    while os.path.isfile(directory_name + '/results-' + str(nof_nodes) + '.csv'):
        filename = directory_name + '/results-' + str(nof_nodes) + '.csv'
        tmp = pandas.read_csv(filename, sep=',', header=None)
        tmp["nof_nodes"]=nof_nodes

        df = df.append(tmp)

        nof_nodes*=2

    df.columns=["query_baseline", "nof_dataflows", "window_id", "latency", "avg_latency", "throughput", "nof_nodes"]

    return df

def read_sliding_window(src_df):
    df = pandas.DataFrame()

    # get rows where query is NQ5
    for index, row in src_df.iterrows():
        if (row.query_baseline[:3] == "NQ5"):
            df = df.append(row)

    return df

def read_tumbling_window(src_df):
    df = pandas.DataFrame()

    # get rows where query is NQ5
    for index, row in src_df.iterrows():
        if (row.query_baseline[:3] == "NQ8"):
            df = df.append(row)

    return df

def read_node(nof_nodes, src_df):
    df = pandas.DataFrame(columns=["avg_latency", "nof_dataflows", "nof_nodes", "query_baseline", "throughput"])

    node = int(nof_nodes)
    baselines_list = src_df.query_baseline.unique()
    dataflows_list = src_df.nof_dataflows.unique()
    throughput_list = src_df.throughput.unique()

    # replace latency values by average latency
    for baseline in baselines_list:
        for dataflow in dataflows_list:
            for throughput in throughput_list:
                
                # also removes extreme values / outliers (discriminant value is 5, but we don't expect latencies higher than 2 while messages are in order)
                tmp = src_df.loc[(src_df.query_baseline == baseline) & (src_df.nof_dataflows == dataflow) & (src_df.nof_nodes == nof_nodes) & (src_df.throughput == throughput) & (src_df.latency < 5)]
                mean = tmp.latency.mean()
                
                df = df.append({"avg_latency":mean, "nof_dataflows":int(dataflow), "nof_nodes":int(node), "query_baseline":baseline, "throughput":int(throughput)}, ignore_index=True)
    return df

def read_dataflow(nof_dataflow, src_df):
    dataflow = int(nof_dataflow)
    return src_df.loc[src_df.nof_dataflows == dataflow]

def generate_node_image(src_df):
    return seaborn.lineplot(data=src_df, y='avg_latency', x='throughput', hue='query_baseline', markers=True)
    # return seaborn.lineplot(data=src_df, y='avg_latency', x='throughput', hue='query_baseline', size='nof_dataflows', markers=True)

def save_image(plot, filename):
    fig = plot.get_figure()
    fig.get_axes()[0].set(xscale="log", yscale="log")
    fig.savefig("graphs/" + filename)
    plt.clf() # don't keep it in memory

def write_missing(baseline, nodes, dataflow, throughput):
    f = open("missing.txt", 'a')

    baseline_format = {
        "NQ5 (count)":"NQ5",
        "NQ5 (flow-wrapping)":"NQ5FW",
        "NQ5 (sort)":"NQ5WS",
        "NQ8 (count)":"NQ8",
        "NQ8 (flow-wrapping)":"NQ8FW",
        "NQ8 (sort)":"NQ8WS",
    }

    line = baseline_format[baseline] + " " + str(int(nodes)) + " " + str(int(dataflow)) + " " + str(int(throughput)) + '\n'
    f.write(line)
    f.close()

def generate_all_nodes_images(src_df, suffix="-default"):
    node_list = src_df.nof_nodes.unique()

    for nof_nodes in node_list:
        df = read_node(nof_nodes, src_df)

        dataflow_list = df.nof_dataflows.unique()
        for dataflow in dataflow_list:
            df2 = read_dataflow(dataflow, df)

            if not df2.isnull().values.any():
                plot = generate_node_image(df2)
                fig = plot.get_figure()
                fig.get_axes()[0].set(xlabel="Throughput (events / sec)", ylabel="Average latency ( " + str(int(dataflow)) + " dataflows , sec)")
                save_image(plot, str(int(nof_nodes)) + '-' + str(int(dataflow)) + suffix + ".png")
            
            else:
                # plot doesn't contain all values for node/dataflow combination
                for index, row in df2.iterrows():
                    if pandas.isna(row["avg_latency"]):
                        qb = row["query_baseline"]
                        tp = row["throughput"]
                        write_missing(qb, nof_nodes, dataflow, tp)

def generate_sliding_window_images(src_df):
    df = read_sliding_window(src_df)
    generate_all_nodes_images(df, "-sliding")

def generate_tumbling_window_images(src_df):
    df = read_tumbling_window(src_df)
    generate_all_nodes_images(df, "-tumbling")

if __name__ == "__main__":
    df = load_results(path_to_results)

    # reset missing.txt file
    f = open("missing.txt", 'w')
    f.close()

    generate_sliding_window_images(df)
    generate_tumbling_window_images(df)
