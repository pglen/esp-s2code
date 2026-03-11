import matplotlib.pyplot as plt
import numpy as np
import sys

def loadnums(fname):

    arr = []
    fp = open(fname)
    buff = fp.read()
    fff = buff.find("app_main")
    lll = buff[fff:].split()
    for aa in lll:
        try:
            num = int(aa)
            arr.append(num)
        except:
            continue;
        #print(num, end = " ")

    fp.close()
    return arr

fname = 'data.txt'
if len(sys.argv) > 1:
    fname = sys.argv[1]

arrx = loadnums(fname)
#print(arrx)
#sys.exit(0)

# Load data from file (assuming two columns, space/tab delimited)
#data = np.genfromtxt(fname, missing_values=0, filling_values=0).flatten()
data = np.array(arrx)
#x = data[:, 0]  # First column
#y = data[:, 1]  # Second column
#print(data)
#sys.exit()

# Plot the data
plt.plot(data, linestyle='-', label='My Data')
plt.xlabel('X Axis Label')
plt.ylabel('Y Axis Label')
plt.title('Plot with Matplotlib')
plt.grid(True)
plt.legend()

# Save the figure (optional)
plt.savefig('my_plot.png')

# Show the plot
plt.show()
