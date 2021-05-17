import matplotlib.pyplot as plt
from pandas import read_csv
import csv
import math

""" return a pandas dataframe """
def load_data(filename):
    try:
        with open(filename, newline='') as datafile:
            return read_csv(filename, sep=',', keep_default_na=False, na_values=[''])
    except FileNotFoundError as e:
        raise e
    
def split_dataframe(df, chunk_size=25):
    chunks = list()
    num_chunks = math.ceil(len(df)/chunk_size)
    for i in range(num_chunks):
        chunks.append(df[i*chunk_size:(i+1)*chunk_size])
    return chunks, num_chunks
# Get the dataframe from the csv file.
df = load_data("statistics.csv")
df[['period', 'deadline']] = df[['period', 'deadline']].astype(str)
df_list, num_chunks = split_dataframe(df,25)
ax = plt.gca()
for i in range(num_chunks):
    label_string = df_list[i]['Qmethod'].any()
    if(label_string == "FCFS"):
        label_string = "EDF" + " P:" + df_list[i]['period'].any() + " D:" + df_list[i]['deadline'].any()
        df_list[i].plot(x='channelnum',y='utilisation',kind='line',label=label_string,ax=ax)
        ax.set_xticks(df_list[i].channelnum)
    elif(label_string == "PRIO"):
        label_string = label_string + " P:" + df_list[i]['period'].any() + " D:" + df_list[i]['deadline'].any()
        df_list[i].plot(x='channelnum',y='utilisation',kind='line',label=label_string,ax=ax,linestyle='--')
        ax.set_xticks(df_list[i].channelnum)
plt.show()

ax = plt.gca()
for i in range(num_chunks):
    label_string = df_list[i]['Qmethod'].any()
    if(label_string == "FCFS"):
        label_string = "EDF" + " P:" + df_list[i]['period'].any() + " D:" + df_list[i]['deadline'].any()
        df_list[i].plot(x='channelnum',y='deadlineMissRatio',kind='line',label=label_string,ax=ax)
        ax.set_xticks(df_list[i].channelnum)
    elif(label_string == "PRIO"):
        label_string = label_string + " P:" + df_list[i]['period'].any() + " D:" + df_list[i]['deadline'].any()
        df_list[i].plot(x='channelnum',y='deadlineMissRatio',kind='line',label=label_string,ax=ax,linestyle='--')
        ax.set_xticks(df_list[i].channelnum)
plt.show()