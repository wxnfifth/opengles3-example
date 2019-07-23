import cv2
import numpy as np
import matplotlib.pyplot as plt


with open('build/m.txt', 'r') as f:
    line = f.readlines()
    
data = []
for d in line:
    data.append(int(d))

data = np.asarray(data, np.uint8)

data = np.reshape(data, (240,320,4))
print(data.shape)

plt.imshow(data)
plt.show()