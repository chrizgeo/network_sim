import matplotlib.pyplot as plt
from pandas import read_csv
import csv

""" return a pandas dataframe """
def load_data(filename):
    try:
        with open(filename, newline='') as datafile:
            return read_csv(filename, sep=',', keep_default_na=False, na_values=[''])
    except FileNotFoundError as e:
        raise e

dataFrame = load_data("statistics.csv")
print(dataFrame)
print(dataFrame.dtypes)
dataFrame.plot(x='channelnum',y='utilisation',kind='line')
dataFrame.plot(x='channelnum',y='deadlineMissRatio',kind='line')
plt.show()

