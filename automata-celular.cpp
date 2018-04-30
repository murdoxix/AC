#include <iostream>
#include <fstream>
#include <string.h>
#include <sys/types.h> // para mkfifo
#include <sys/stat.h> // para mkfifo
#include <unistd.h> // para unlink

#define GPFIFO "./gpio" // any unique name 
#define RLEbufftam  1024 // tamaño del buffer para leer el archivo RLE 
#define ARCHIVO "./celdas.dat" // donde se guardara la info para que GNUPLOT la lea.
#define TIEMPOESPERADEFAULT 500000
using namespace std;

int TIEMPOESPERA = TIEMPOESPERADEFAULT;

// Esta funcion solicita la memoria para representar el tablero y hacer
// que este represente la disposicion inicial que viene  en el archivo RLE. 
// Devuelve 0 si fue exitosa y 1 si hubo algun error.
int armar_tablero(bool** &celdas, bool** &celdasprev, int ancho, int alto, ifstream &rle){
    int lineas = 0; // representa la cantidad de lineas horizontales
    // que ya fueron representadas.
    int cant = 0; // representa la cantidad de celdas que ya
    // fueron representadas en la linea actual.
    int run_count; // se asume 1 si no esta especificado
    char buff[RLEbufftam], *tmp;
    if( !(celdas = (bool**)malloc(sizeof(bool*)*alto)) ){
        puts("Hubo un error pidiendo memoria.");
        return 1;
    }
   
    for(int i=0 ; i<alto ; i++){
        if( !(celdas[i] = (bool*)malloc(sizeof(bool)*ancho)) ){
            puts("Hubo un error pidiendo memoria.");
            return 1;
        }
    }

    if( !(celdasprev = (bool**)malloc(sizeof(bool*)*alto)) ){
        puts("Hubo un error pidiendo memoria.");
        return 1;
    }
   
    for(int i=0 ; i<alto ; i++){
        if( !(celdasprev[i] = (bool*)malloc(sizeof(bool)*ancho)) ){
            puts("Hubo un error pidiendo memoria.");
            return 1;
        }
    }


    for(int i=0 ; i<alto ; i++)
        for(int k=0 ; k<ancho ; k++) celdas[i][k] = false;
    
    if(!rle.is_open()){
        puts("Hubo un error con el archivo RLE");
        return 1;
    }
    rle.getline(buff, RLEbufftam);
    tmp = buff;

    while( lineas<alto ){
        while( tmp[0]==' ' || tmp[0]=='\n' || tmp[0]=='\r' ) tmp++;
        if( tmp[0]=='\0' ){
            rle.getline(buff, RLEbufftam);
            tmp = buff;
        }
        if( isdigit(tmp[0]) ){
            run_count = atoi(tmp);
            while( isdigit(tmp[0]) ) tmp++;

        } else {
            run_count = 1;
        }
        if( tmp[0]=='b' ){
            tmp++;
            cant += run_count; 

        } else if( tmp[0]=='o' ){
            for(int k=0 ; k<run_count ; k++) celdas[lineas][cant+k] = true;
            tmp++;
            cant += run_count;

        } else if( tmp[0]=='$' ){
            tmp++;
            cant = 0;
            lineas += run_count;

        } else if( tmp[0]=='!' ){
            break;
        } else {
            printf("Se encontro algo inesperado en el archivo RLE: '%c'\n", tmp[0]);
            return 1;
        }
    }
    
    rle.close();
    return 0;

}


void actualizar_tablero(bool** celdas, bool** celdasprev, bool* B, bool* S, int ancho, int alto){
    int vecinas;
    for(int i=0 ; i<alto ; i++)
        for(int k=0 ; k<ancho ; k++) celdasprev[i][k] = celdas[i][k];

    for(int i=0 ; i<alto ; i++){
        for(int k=0 ; k<ancho ; k++){
            vecinas = 0;
            if( i>0 && k>0 && celdasprev[i-1][k-1]) vecinas++;
            if( i>0 && celdasprev[i-1][k]) vecinas++;
            if( i>0 && k+1<ancho && celdasprev[i-1][k+1]) vecinas++;
            if( k>0 && celdasprev[i][k-1]) vecinas++;
            if( k+1<ancho && celdasprev[i][k+1]) vecinas++;
            if( i+1<alto && k>0 && celdasprev[i+1][k-1]) vecinas++;
            if( i+1<alto && celdasprev[i+1][k]) vecinas++;
            if( i+1<alto && k+1<ancho && celdasprev[i+1][k+1]) vecinas++;
            // ahora vecinas representa la cant de vecinas vivas de la celda i k
            if( celdasprev[i][k] ){
                if( !S[vecinas] ) celdas[i][k] = false;
            } else {
                if( B[vecinas] ) celdas[i][k] = true;
            }
        }
    }
}

// Borra el contenido de ARCHIVO e imprime los puntos donde estan las celdas
// vivas para que GNUPLOT lo lea.
void escribir_tablero(bool** celdas, int ancho, int alto){
    ofstream tablero(ARCHIVO, ofstream::trunc);

    for(int i=0 ; i<alto ; i++)
        for(int k=0 ; k<ancho ; k++)
            if(celdas[i][k]) tablero << ancho-k << " " << alto-i << endl;

    tablero.close();
}

// Esta funcion libera la memoria solicitada para el tablero.
void liberar_tablero(bool** &celdas, bool** &celdasprev, int alto){
    for(int i=0 ; i<alto ; i++){
        free(celdas[i]);
        free(celdasprev[i]);
    }
    free(celdas);
    free(celdasprev);
}

int main(int argc, char *argv[]){ 
    FILE *gp, *gpin;
    ifstream rle;
    char RLEbuff[RLEbufftam], *tmp;
    int ancho, alto;
    bool  B[9], S[9], **celdas, **celdasprev; 
// B y S son arreglos que representan las vecinas necesarias para que una celda
// nazca y sobreviva respectivamente. Si B[1] es true y todos los demas falso,
// significa que una celda nace si tiene una vecina viva. Si S[3] y S[5] son true
// y los demas son falso significa que una celda sobrevive si tiene 3 o 5 vecinas.
    for(int i=0 ; i<9 ; i++ ){
        B[i] = false;
        S[i] = false;
    }

    if(argc != 2 && argc != 3){
        cout << "Use: " << argv[0] << " <archivo rle> [TIEMPO]\nTiempo: Intervalo de tiempo entre actualizacion del tablero en segundos (0.5 por defecto)." << endl;
        exit(1);
    }

    if(argc == 3){
        if((double) atof(argv[2])<=0) {cout << "El tiempo debe ser positivo." << endl; exit(1);}
        TIEMPOESPERA = (int)(atof(argv[2])*1000000);
        printf("El intervalo de tiempo entre actualizacion del tablero se establecio en %f segundos.\n",TIEMPOESPERA/1000000.0);
    }
    rle.open(argv[1], std::ifstream::in);
    if(!rle.is_open()){
        cout << "Error abriendo " << argv[1] << ". Debe ser un archivo RLE." << endl;
        return 1;
    }
    puts("Archivo RLE abierto exitosamente.");
    
// Crea un archivo FIFO que va ser para conectar gnuplot => nuestro programa.
    if (mkfifo(GPFIFO, 0600)) {
        if (errno != EEXIST) {
        perror(GPFIFO);
        unlink(GPFIFO);
        return 1;
        }
    }

    if (NULL == (gp = popen("gnuplot","w"))) {
        perror("gnuplot");
        pclose(gp);
        return 1;
    }
    puts("Conectado con gnuplot.\n");

// Inicializar mouse y redireccionar gnuplot print al FIFO.
    fprintf(gp, "set mouse; set print \"%s\"\n", GPFIFO);
    fflush(gp);

// Abrir FIFO, donde esta escribiendo gnuplot, para escribir.
    if (NULL == (gpin = fopen(GPFIFO,"r"))) {
        perror(GPFIFO);
        pclose(gp);
        return 1;
    }
    puts("FIFO abierto para lectura de gnuplot.\n");

    rle.getline(RLEbuff,RLEbufftam);
    while(RLEbuff[0] == '#') rle.getline(RLEbuff,RLEbufftam); // Eliminar las lineas de comentarios

    if( strstr(RLEbuff,"x = ") != RLEbuff ){
        puts("Error en el archivo RLE (1)");
        return 1;
    }

    if( !(ancho = atoi(RLEbuff+4)) ){
        puts("Error en el archivo RLE (2)");
        return 1;
    }

    if( ( !(tmp = strchr(RLEbuff+4, 'y')) ) && strcmp(tmp,"y = ") ){
        puts("Error en el archivo RLE (3)");
        return 1;
    }

    if( !(alto = atoi(tmp+4)) ){
        puts("Error en el archivo RLE (4)");
        return 1;
    }
    
    if( !(tmp = strstr(tmp+4, "rule = ")) ){
        // En este caso no se especifico rule, asi que tomamos por defecto B3/S23 (Juego De La Vida rule)    
        B[3] = true;
        S[2] = true;
        S[3] = true;
    } else { // Interpretamos la rule especificada en el RLE.
        tmp += 7;
        if( tmp[0] == 'B' || tmp[0] == 'b' ) tmp ++;
        while( tmp[0] != '/' ){
            if( !isdigit(tmp[0]) ){
                puts("Error en el archivo RLE (5)");
                return 1;
            } 
            B[tmp[0]-'0'] = true;
            tmp++;
        }
        tmp++;
        if( tmp[0] == 'S' || tmp[0] == 's' ) tmp ++;
        while( isdigit(tmp[0]) ){
            S[tmp[0]-'0'] = true;
            tmp++;
        }


    }

    if( armar_tablero(celdas, celdasprev, ancho, alto, rle) ){
        puts("Error armando el tablero.");
        return 1;
    }

    escribir_tablero(celdas, ancho, alto);

// Preparamos gnuplot
    fprintf(gp, "set xrange [-2:%d]\n", ancho+2);
    fprintf(gp, "set yrange [-2:%d]\n", alto+2);
    fprintf(gp, "set term x11 1 noraise\n"); // para que no haga foco sobre el grafico
    fprintf(gp, "set size ratio -1\n"); // radio del cuadrado
    fprintf(gp, "set style fill solid 0.5\n"); // cuadrados rellenos
    fprintf(gp, "plot \"%s\" u 1:2:(.45):(.45) w boxxyerrorbars notitle\n", ARCHIVO);
    fflush(gp);

    for(int n=0; n<1000; n++) {
        actualizar_tablero(celdas, celdasprev, B, S, ancho, alto);
        escribir_tablero(celdas, ancho, alto);
        fprintf(gp, "replot\n");
        fflush(gp);
        usleep(TIEMPOESPERA);
    }

    liberar_tablero(celdas, celdasprev, alto);
    fclose(gpin);
    pclose(gp);
    unlink (GPFIFO);
    return 0;
}
