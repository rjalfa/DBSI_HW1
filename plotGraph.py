import matplotlib.pyplot as plt

s10 = list(map(float,open("LH10_U_S").readlines()))
s70 = list(map(float,open("LH70_U_S").readlines()))
h70 = list(map(float,open("LH70_U_H").readlines()))
h10 = list(map(float,open("LH10_U_H").readlines()))
cs10, = plt.plot(s10)
cs70, = plt.plot(s70)
ch10, = plt.plot(h10)
ch70, = plt.plot(h70)

plt.suptitle('Disk Utilization')
plt.xlabel('Number of records')
plt.ylabel('Disk Utilization')
plt.legend([cs10,cs70,ch10,ch70],['Uniform Dataset, Bucket size: 10','Uniform Dataset, Bucket size: 70','Highbit Dataset, Bucket size: 10','Highbit Dataset, Bucket size: 70'])
plt.show()