#include <mpi.h>
#include <iostream>
#include "omp.h"
#include <FreeImage.h>
#include <string>     // std::string, std::to_string

using namespace std;
#define BPP 32 //Bits per pixel 


int mapeo(int,int);

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

/**
 * Multiplica dos numeros complejos 
 * @param Z1
 * @param Z2
 * @param result
 */
void multComplex(float *Z1, float *Z2, float *result) {
    result[0] = Z1[0] * Z2[0] - Z1[1] * Z2[1];
    result[1] = Z1[0] * Z2[1] + Z1[1] * Z2[0];
}

/**
 * Divide dos numeros complejos
 * @param Z1
 * @param Z2
 * @param result
 */
void divComplex(float *Z1, float *Z2, float *result) {
    float div = Z2[0] * Z2[0] + Z2[1] * Z2[1];
    float resultMULT[2];
    Z2[1] = Z2[1] * -1;
    multComplex(Z1, Z2, resultMULT);
    result[0] = resultMULT[0] / div;
    result[1] = resultMULT[1] / div;
}

/**
 * Realiza el mapeo de Z a W
 * @param ZX
 * @param result
 */
void mapper(float *ZX, float *result) {

    float Z[2] = {ZX[0], ZX[1]};
    float C[2] = {0.003, 0};
    float resultMULT[2];
    multComplex(C, Z, resultMULT);
    resultMULT[0] += 1;
    resultMULT[1] += 1;
    float A[2] = {2.1, 2.1};
    float resultMULT2[2];
    multComplex(A, Z, resultMULT2);
    divComplex(resultMULT2, resultMULT, result);

}

/**
 * Realiza el mapeo inverso de W a Z
 * @param ZX
 * @param result
 */
void inverseMapper(int *ZX, float *result) {

    float W[2] = {ZX[0], ZX[1]};
    float C[2] = {0.003, 0};
    float resultMULT[2];
    multComplex(C, W, resultMULT);
    resultMULT[0] += -2.1;
    resultMULT[1] += -2.1;
    float D[2] = {-1, -1};
    float resultMULT2[2];
    multComplex(D, W, resultMULT2);
    divComplex(resultMULT2, resultMULT, result);

}

#define MASTER 0       /* id of the first process */ 
#define FROM_MASTER 1  /* setting a message type */ 
#define FROM_WORKER 2  /* setting a message type */ 

MPI_Status status;
MPI_Datatype sub;

int ProcesarMatrices(int numworkers) {


    int IE = 64;

    int averow = IE / numworkers;
    int extra = IE % numworkers;

    int rows = 0;
    int dest = 1;

    int mtype = FROM_MASTER;
    for (dest = 1; dest <= numworkers; dest++) {
        rows = (dest <= extra) ? averow + 1 : averow;
       // printf("rows:%d\n", rows);
        //int MPI_Send(void *buf, int count, MPI_Datatype datatype,int dest, int tag, MPI_Comm comm)
        MPI_Send(&rows, 1, MPI_INT, dest, mtype, MPI_COMM_WORLD); //send the rows to WORKER
    }
    int ready = 0;

    /* wait for results from all worker processes */
    mtype = FROM_WORKER;
    for (dest = 1; dest <= numworkers; dest++) {
        //int MPI_Recv(void *buf, int count, MPI_Datatype datatype,int source, int tag, MPI_Comm comm, MPI_Status *status)
        MPI_Recv(&ready, 1, MPI_INT, dest, mtype, MPI_COMM_WORLD, &status); //receive the offset from WORKER
    }

}

int WorkerProcesses(int rank) {

    
    int source = MASTER;
    int rows = 0;
    int mtype = FROM_MASTER;
    //int MPI_Recv(void *buf, int count, MPI_Datatype datatype,int source, int tag, MPI_Comm comm, MPI_Status *status)
    MPI_Recv(&rows, 1, MPI_INT, source, mtype, MPI_COMM_WORLD, &status); //receive the offset


    for (int t = 0; t < rows; t++) {
	//printf("RANK:%d ROW:%d\n",rank,t);
	mapeo(t,rank);
    }

    int ready = 1;
    mtype = FROM_WORKER;
    //int MPI_Send(void *buf, int count, MPI_Datatype datatype,int dest, int tag, MPI_Comm comm)
    MPI_Send(&ready, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD); //send the offset to MASTER
}

int Delete3D_Array(int ***pResult, int x, int y, int z)
{
    int ***p = pResult;
    for(int i = 0; i < x; i++)
        for(int j = 0; j < y; j++)
            delete p[i][j];
 
    for(int i = 0; i < x; i++)
        delete p[i];
    delete p;
    return 0;
}
 


#define iter 1

int mapeo(int map,int processor){

    int i,j,k;

    FreeImage_Initialise();
    FREE_IMAGE_FORMAT formato = FreeImage_GetFileType("foto7.jpg", 0);
    FIBITMAP* bitmap = FreeImage_Load(formato, "foto7.jpg");
    FIBITMAP* temp = FreeImage_ConvertTo32Bits(bitmap);
    int width = FreeImage_GetWidth(temp);
    int height = FreeImage_GetHeight(temp);
    FreeImage_Unload(bitmap);
    bitmap = temp;

    int IE = width;
    int JE = height; 
    int KE = 3;

    int ***entrada = alloc3d(IE, JE, KE);
    int ***salida = alloc3d(IE, JE, KE);


    /********* Read Image **********/
    // Bitmap a la matrix llenando la entrada
    for (i = 0; i < IE; i++) {
        for (j = 0; j < JE; j++) {
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


    for (i = 0; i < IE; i++) {
            for (j = 0; j < JE; j++) {

                int Z[2] = {i, JE - j};
                float resultMap[2];
                inverseMapper(Z, resultMap);

                int newi = resultMap[0];
                int newj = resultMap[1];

                newj = newj >= 0 ? newj : 0;
                newi = newi >= 0 ? newi : 0;

                newj = newj < JE ? newj : 0;
                newi = newi < IE ? newi : 0;

                newj = newi == 0 ? 0 : newj;
                newi = newj == 0 ? 0 : newi;

                for (k = 0; k < KE; k++) {

                    int x = entrada[newi][JE - 1 - newj][k];
                    salida[i][j][k] = x;

                }
            }
    }

FIBITMAP* new_bitmap = FreeImage_Allocate(width, height, BPP);

 //convertir matrix a bitmap
        for (int i = 0; i < width; i++) {
            for (int j = 0; j < height; j++) {
                RGBQUAD color;
                color.rgbBlue = salida[i][j][0];
                color.rgbGreen = salida[i][j][1];
                color.rgbRed = salida[i][j][2];
                FreeImage_SetPixelColor(new_bitmap, i, j, &color);
            }
        }
    
    
    
        float Zwidth[2] = {(float) width, 0};
        float Zheight[2] = {0, (float) height};
        float resultwidth[2];
        float resultheight[2];
    
    
        mapper(Zwidth, resultwidth);
        mapper(Zheight, resultheight);
    
    
        //Recortar la imagen
        int newwidth = resultwidth[0];
        int newheight = resultheight[1];
        FIBITMAP * salidaFinal = FreeImage_Allocate(newwidth, newheight, BPP);
        for (int i = 0; i < newwidth; i++) {
            for (int j = 0; j < newheight; j++) {
                RGBQUAD color;
                FreeImage_GetPixelColor(new_bitmap, i, height - j, &color);
                FreeImage_SetPixelColor(salidaFinal, i, newheight - j, &color);
            }
        }
    


 
 
  std::string s = std::to_string(processor) +" S:"+ std::to_string(map) + ".bmp";
char const *pchar = s.c_str();  //use char const* as target type


	string a = "P:";
	a += pchar;
	const char *C = a.c_str();

        FreeImage_Save(FIF_BMP, salidaFinal,C);
        FreeImage_Unload(bitmap);


}

int main(int argc, char ** argv) {

  

    int size, rank;
    int numworkers, extra, averow, mtype;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);


    numworkers = size - 1;

  


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

        // for (int a = 0; a < iter; a++) {
        WorkerProcesses(rank);
        // }
    }
    MPI_Finalize();
}
