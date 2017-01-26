from random import randint
from sys import argv
N = 1000
if len(argv) >= 1:
	N = int(argv[1])

print N
for _ in xrange(N):
	print randint(1,N+1)

