#!/bin/bash
count=1
while [[ $count -le 20 ]]
do
   echo "Se ejecuto prueba"
   mpirun -np 2 --hostfile .mpi_hostfile ./3D >> resultadosSerial.csv
   (( count++ ))
done
