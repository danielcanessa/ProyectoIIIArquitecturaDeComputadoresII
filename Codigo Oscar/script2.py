#!/usr/bin/python
import os




print ("Cluster de 4 nodos 1 core cada uno")

cores_m = 4
cores_s1 = 16
cores_s2 = 16
cores_s3 = 16

procesos = cores_m + cores_s1 + cores_s2 +cores_s3 
config = "python script1.py " + str(cores_m)+" " +str(cores_s1)+" "+str(cores_s2)+" "+str(cores_s3)  
fp = os.popen(config)
instruc = "mpirun -np "+str(procesos)+" --hostfile .mpi_hostfile ./conBanco" 
fp = os.system(instruc)
