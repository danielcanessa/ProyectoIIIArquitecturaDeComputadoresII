#include <mpi.h>
#include <iostream>
#include "omp.h"
#include <FreeImage.h>
#include <cmath>
#include <string>
using namespace std;
#define BPP 32 //Bits per pixel 


int width;
int height;
int depth;

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

/*
 * Con esta función se obtiene la componente imaginaria del mapeo inverso
 */
int calcularMapeoInversoY(int i, int j) {
    double A = i - j;
    double B = i + j;
    double D = 2.1 - 0.003 * i;
    double E = 2.1 - 0.003 * j;
    double divisor = pow(D, 2) + pow(E, 2);
    return ((B * D)-(A * E)) / divisor;
}

/*
 * Con esta función se obtiene la componente real del mapeo inverso 
 */
int calcularMapeoInversoX(int i, int j) {
    double A = i - j;
    double B = i + j;
    double D = 2.1 - 0.003 * i;
    double E = 2.1 - 0.003 * j;
    double divisor = pow(D, 2) + pow(E, 2);
    return ((A * D)+(B * E)) / divisor;
}

/*
 * Con esta funcion se obtiene la componente real del mapeo bilineal 
 */
int calcularMapeoX(int i, int j) {
    double A = 2.1 * i - 2.1 * j;
    double B = 2.1 * i + 2.1 * j;
    double D = 0.003 * i + 1;
    double E = 0.003 * j + 1;
    double C = pow((0.003 * i + 1), 2) + pow((0.003 * j + 1), 2);
    return (A * D + B * E) / C;


}

/*
 * Con esta función se obtiene la componente imaginaria del mapeo bilineal
 */
int calcularMapeoY(int i, int j) {
    double A = 2.1 * i - 2.1 * j;
    double B = 2.1 * i + 2.1 * j;
    double D = 0.003 * i + 1;
    double E = 0.003 * j + 1;
    double C = pow((0.003 * i + 1), 2) + pow((0.003 * j + 1), 2);
    return (-A * E + B * D) / C;
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



#define MASTER 0       /* id of the first process */ 
#define FROM_MASTER 1  /* setting a message type */ 
#define FROM_WORKER 2  /* setting a message type */ 


int ***entrada;
int ***salida;
MPI_Status status;


//SOLO LO EJECUTA EL MAESTRO RANK 0
int ProcesarMatrices(int numworkers) {



    int cantidad = 100;
    int dest = 1;
    int finish=0;

    int mtype = FROM_MASTER;
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

//LO REALIZA UN WORKER (CPU NO MAESTRO)
int WorkerProcesses(int rank) {

    int source = MASTER;
    int finish = 0;
    int cantidad = 0;

    int mtype = FROM_MASTER;
	//Recibe la cantidad de mapeos que va a hacer
    MPI_Recv(&cantidad, 1, MPI_INT, source, mtype, MPI_COMM_WORLD, &status); 
    
	for (int times = 0; times < cantidad; times++) {
		cout<<"la cantidad es:" <<cantidad << "\n";
          calcularMapeo(salida,entrada,width,height,depth);
        FIBITMAP* new_bitmap = FreeImage_Allocate(width, height, BPP);
	int i,j;
    for (i = 0; i < width; i++) {
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

    entrada = alloc3d(width, height, depth);
    for (i = 0; i < width; i++) {
        for (j = 0; j < height; j++) {
            for (k = 0; k < depth; k++) {
                entrada[i][j][k] = 0;
            }
        }
    }

    salida = alloc3d(width, height, depth);


    /********* Read Image **********/
    // Bitmap a la matrix llenando la entrada
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
    if (rank == MASTER) {
 
    double start_time, run_time;
    start_time = omp_get_wtime();

    ProcesarMatrices(numworkers);
    
    run_time = omp_get_wtime() - start_time;
    printf("%lf\n", run_time);

   

      

    }


    /************ worker process *************/
    if (rank > MASTER) {

        WorkerProcesses(rank);
        
    }
    MPI_Finalize();
}
