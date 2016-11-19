#!/usr/bin/python
import os
from sys import argv


nodos=0
cores_m = int(argv[1])
cores_s1 = int(argv[2])
#cores_s2 = 16
#cores_s3 = 16

#CONFIGURACION DEL HOSTFILE
procesos = cores_m + cores_s1 #+ cores_s2 +cores_s3 

if cores_m>0:
	nodos= nodos+1
if cores_s1>0:
	nodos= nodos+1
print ("Cluster de "+str(nodos)+" nodos con maestro: "+ str(cores_m-1)+ "cpus y slave1: "+str(cores_s1)+" cpus")

config = "python hostSetup.py " + str(cores_m)+" " +str(cores_s1)#+" "+str(cores_s2)+" "+str(cores_s3)  
fp = os.popen(config)
instruc = "mpirun -np "+str(procesos)+" --hostfile .mpi_hostfile ./conBanco" 
fp = os.system(instruc)

