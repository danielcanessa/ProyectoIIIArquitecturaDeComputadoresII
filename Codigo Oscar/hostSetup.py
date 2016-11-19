from sys import argv

filename = ".mpi_hostfile"

line1 = "localhost slots="+str(argv[1])
line2 = "mpiuser@192.168.0.100 slots=" + str(argv[2])
#line3 = "curso@172.19.12.175 slots=" + str(argv[3])
#line4 = "curso@172.19.12.57 slots=" + str(argv[4])

t = open(filename, 'w')


t.truncate()




t.write(line1)
t.write("\n")
t.write(line2)
#t.write("\n")
#t.write(line3)
#t.write("\n")



t.close()
 
