#include <mpi.h>
#include <iostream>
#include "omp.h"
#include <FreeImage.h>
#include <cmath>
#include <string>

using namespace std;
#define BPP 32 //Bits per pixel 
#define MASTER 0       /* id of the first process */ 
#define FROM_MASTER 1  /* setting a message type */ 
#define FROM_WORKER 2  /* setting a message type */ 


int width;
int height;
int depth;
int ***entrada;
int ***salida;
MPI_Status status;

int ***alloc3d(int l, int m, int n) {
    int *data = new int [l * m * n];
    int ***array = new int **[l];
    for (int i = 0; i < l; i++) {
        array[i] = new int *[m];
        for (int j = 0; j < m; j++) {
            array[i][j] = &(data[(i * m + j) * n]);
        }
    }
    return array;
}

void calcularMapeo(int ***salida, int ***entrada, int width, int height, int depth) {
    int i,j,k;
    for (i = 0; i < width; i++) {
        for (j = 0; j < height; j++) {
            double A = i - (height - 1 - j);
            double B = i + (height - 1 - j);
            double D = 2.1 - 0.003 * i;
            double E = 2.1 - 0.003 * (height - 1 - j);
            double divisor = pow(D, 2) + pow(E, 2);

            int newi = ((A * D)+(B * E)) / divisor;
            int newj = ((B * D)-(A * E)) / divisor;
            
            
            newj = newj >= 0 ? newj : 0;
            newi = newi >= 0 ? newi : 0;

            newj = newj < height ? newj : 0;
            newi = newi < width ? newi : 0;
            
            //Garantiza que se eligira el negro en la posicion quemada
            newj = newi == 0 ? 0 : newj;
            newi = newj == 0 ? 0 : newi;

            for (k = 0; k < depth; k++) {
                int x = entrada[newi][height - 1 - newj][k];
                salida[i][j][k] = x;
                
            }
        }
    }
}

/*
Este metodo es invocado por el rank maestro, rank=0
*/
int ProcesarMatrices(int numworkers) {

    int cantidad = 100;
   
    int dest = 1;
    int finish=0;

    int mtype = FROM_MASTER; //establece el tipo de mensaje como master

    //Distribuye el trabajo a los esclavos
    for (dest = 1; dest <= numworkers; dest++) {
		//envia la cantidad de fotos que puede realizar el CPU
        MPI_Send(&cantidad, 1, MPI_INT, dest, mtype, MPI_COMM_WORLD); 
    }

    /* Espera para que cada procesador termine */
    mtype = FROM_WORKER;
    for (dest = 1; dest <= numworkers; dest++) {
        MPI_Recv(&finish, 1, MPI_INT, dest, mtype, MPI_COMM_WORLD, &status);        
    }
}

/*
Este metodo es invocado por caulquier rank esclavo, rank!=0
*/
int WorkerProcesses(int rank) {

    int source = MASTER;
    int finish = 0;
    int cantidad = 0;

    int mtype = FROM_MASTER;
	
    //Recibe la cantidad de mapeos que va a hacer
    MPI_Recv(&cantidad, 1, MPI_INT, source, mtype, MPI_COMM_WORLD, &status); 

    printf("Cantidad mapeos: %d\n",cantidad);

    int currentRank=0;
    MPI_Comm_rank(MPI_COMM_WORLD, &currentRank);
    
	for (int times = 0; times < cantidad; times++) 
    { 
        
        printf("Iteracion: %d Rank: %d\n",times,currentRank );
        
        calcularMapeo(salida,entrada,width,height,depth);
        FIBITMAP* new_bitmap = FreeImage_Allocate(width, height, BPP);
	    int i,j;
        for (i = 0; i < width; i++) 
        {
            for (j = 0; j < height; j++) {
                RGBQUAD color;
                color.rgbBlue = salida[i][j][0];
                color.rgbGreen = salida[i][j][1];
                color.rgbRed = salida[i][j][2];
                FreeImage_SetPixelColor(new_bitmap, i, j, &color);
            }
        }
            
        char buff[4];
        sprintf(buff,"%d",rank);
        char out[20]; 
        strcpy(out,"output");
        strcat(out,buff);
        strcat(out,".bmp");            
        FreeImage_Save(FIF_BMP, new_bitmap,out);
    }
    
    mtype = FROM_WORKER;
    finish = 1;
    MPI_Send(&finish, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD); 
    
}



int main(int argc, char ** argv) {

  
    FreeImage_Initialise();
    
    FREE_IMAGE_FORMAT formato = FreeImage_GetFileType("foto6.png", 0);
    
    FIBITMAP* bitmap = FreeImage_Load(formato, "foto6.png");
    
    FIBITMAP* temp = FreeImage_ConvertTo32Bits(bitmap);
   
    width = FreeImage_GetWidth(temp);
    height = FreeImage_GetHeight(temp);
   
    FreeImage_Unload(bitmap);
    
    bitmap = temp;

    depth = 3;

    int i, j, size, rank, k;
    int numworkers, mtype;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);



    numworkers = size - 1;

    printf("Num ranks: %d\n", rank);
    printf("Cluster size: %d\n", size);
    printf("Num workers: %d\n", numworkers);

    entrada = alloc3d(width, height, depth);
   
    //inicializa el arreglo cubico entrada con 0s
    for (i = 0; i < width; i++) {
        for (j = 0; j < height; j++) {
            for (k = 0; k < depth; k++) {
                entrada[i][j][k] = 0;
            }
        }
    }

    salida = alloc3d(width, height, depth);


    /********* Read Image **********/
    //llena el arreglo cubico entrada con los colores rgb de la imagen 
    for (i = 0; i < width; i++) {
        for (j = 0; j < height; j++) {
            RGBQUAD color;
            FreeImage_GetPixelColor(bitmap, i, j, &color);
            entrada[i][j][0] = color.rgbBlue;
            entrada[i][j][1] = color.rgbGreen;
            entrada[i][j][2] = color.rgbRed;
        }
    }

    entrada[0][height - 1][0] = 0;
    entrada[0][height - 1][1] = 0;
    entrada[0][height - 1][2] = 0;



    /********* master process *******/
    if (rank == MASTER) 
    {
 
        double start_time, run_time;

        start_time = omp_get_wtime(); //obtiene el tiempo en el que se empieza a ejecutar el algoritmo

        ProcesarMatrices(numworkers);
        
        run_time = omp_get_wtime() - start_time;//se calcula el tiempo de ejecucion
       
        printf("%lf\n", run_time); //Se imprime tiempo de ejecución

   
    }


    /************ worker process *************/
    if (rank != MASTER) {

        WorkerProcesses(rank);
        
    }

    MPI_Finalize();
}
