#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

typedef struct Particion{
    char part_status[2]; //1=Enable | 0=Disable
    char part_type[2]; //P=primaria | E=extendida
    char part_fit[3];  //BF=Best |FF=First |WF=Worst
    int part_start; //start byte
    int part_size;//size partition in bytes
    char part_name[16];//name of partition
}structParticion;

typedef struct MasterBootRecord{
    int mbr_tamano;//size disk in bytes
    char mbr_fecha_creacion[16];//date created disk
    int mbr_disk_signature;//random
    structParticion mbr_partition_1;//primary or extended partition
    structParticion mbr_partition_2;//primary or extended partition
    structParticion mbr_partition_3;//primary or extended partition
    structParticion mbr_partition_4;//primary or extended partition
}structMBR;

FILE *auxArchivo; //FILE aux para crear del disco como un archivo.
char buffer[1];//Char de un caracter, igual a 1byte,
char fecha[16];

void settime(){
    time_t t = time(0);//preparamos una structura de tiempo
    struct tm *local = localtime(&t);//Fecha/Hora del sistema en una estructura time
    strftime(fecha,16,"%d/%m/%y %H:%M",local);
}

int fsizeInBytes(char* file) {
  int size;
  FILE* fh;

  fh = fopen(file, "rb"); //binary mode
  if(fh != NULL){
    if( fseek(fh, 0, SEEK_END) ){
      fclose(fh);
      return -1;
    }

    size = ftell(fh);
    fclose(fh);
    return size;
  }

  return -1; //error
}

void crearDiscoFisico(int sizeDiscTemp,char pathDiscTemp[50],char nameDiscTemp[10] ,char unitDiscTemp[2]){
    //NOS ASEGURAMOS DE QUE EXITAN TODAS LAS CARPETAS INDICADAS ANTES DE ESCRIBIR EL DISCTO.
    char pathClean[50];
    int j=0; //index of new array
    int i;
    for(i=0; i<50; i++){//Recorremos el array del la ruta de directorios y formamos nuevo array sin el caracter de comillas
        if(pathDiscTemp[i]=='"'){
            //Nada q hacer, Ignoramos el caracter
        }else{
            pathClean[j]=pathDiscTemp[i];//Cualquier otro caracter lo compiamos al nuevo array
            j=j+1;//y aumentamos el indice del nuevo array para estar listo en recibir otro caracter
        }
    }
    char comando[75]="mkdir -p ";
    strcat(comando,pathClean);
    //printf("Comand Line to create folders: %s \n",comando);
    system(comando);//CARPETAS CREADAS COMANDO POR CONSOLA


    //AHORA SI CREAMOS LA RUTA DIRECTA HACIA EL ARCHIVO DISCO VIRTUAL
    char pathDirectToFile[50];
    strcpy(pathDirectToFile,pathDiscTemp);
    strcat(pathDirectToFile,nameDiscTemp);
    //printf("Ruta directa al archivo con comillas: %s",pathDirectToFile);
    char pathDirectClean[50];
    int y=0; //index of new array
    int x;
    for(x=0; x<=50; x++){//Recorremos el array del la ruta de directorios y formamos nuevo array sin el caracter de comillas
        if(pathDirectToFile[x]=='"'){
            //Nada q hacer, Ignoramos el caracter
        }else{
            pathDirectClean[y]=pathDirectToFile[x];//Cualquier otro caracter lo compiamos al nuevo array
            y=y+1;//y aumentamos el indice del nuevo array para estar listo en recibir otro caracter
        }
    }
    //printf("Path directo to File Disk: %s \n",pathDirectClean);


    //LISTO! CREAMOS Y ESCRIBIMOS EL DISCO CON EL TAMA;O en bytes INDICADO
  if(strcmp(unitDiscTemp,"k")==0){//Si unidad es en kilobytes "k" Multiplicar el caracter buffer de un byte por la cantidad el tamano ingresado
        auxArchivo = fopen(pathDirectClean, "w+b");
        int i=0;
        for(i;i<sizeDiscTemp;i++){
            fwrite(buffer, sizeof(buffer), 1024, auxArchivo);//Por cada iteracion se escribe un byte por el tamano del buffer char, por lo tanto los 1024 es para que escriba 1024bytes por pasada ya que la unidad es en kilobytes
        }
        fclose(auxArchivo);

    }else if(strcmp(unitDiscTemp,"m")==0){//Si es en Mega Byte, escribirmos el caracter de 1byte el tamano de veces que se pidio por 1024 bytes
        auxArchivo = fopen(pathDirectClean, "w+b");
        int i=0;
        for(i;i<sizeDiscTemp*1024;i++){
            fwrite(buffer, sizeof(buffer), 1024, auxArchivo);//Por cada vuelta que escriba 1024bytes=1kilobyte
        }
        fclose(auxArchivo);

    }else{
        printf("ERROR! El tipo de Unidad de tamano para el disco no es Identificado.");
    }


    //TODO DISCO LLEVA UNA ESTRUCTURA MASTER BOOL RECORD AL INICIO QUE TENDRA TODA LA INFORMACION DEL DISCO, POR LA ESCRIBIMOS EN EL DISCO
  settime();//Obtenemos la hora del sistema en una cadena de caracteres
  structMBR newMBR;
  newMBR.mbr_disk_signature=rand()%(96-2+1)+1; //random
  newMBR.mbr_tamano=fsizeInBytes(pathDirectClean);
  strcpy(newMBR.mbr_fecha_creacion,fecha);
  structParticion partaux;
  strcpy(partaux.part_status,"1");
  newMBR.mbr_partition_1=partaux;
  newMBR.mbr_partition_2=partaux;
  newMBR.mbr_partition_3=partaux;
  newMBR.mbr_partition_4=partaux;
/*
  printf("new MBR to create -> idDisk=%d ",newMBR.mbr_disk_signature);
  printf("of size=%dbytes ",newMBR.mbr_tamano);
  printf("at date=%s ",newMBR.mbr_fecha_creacion);
  printf("on %s \n",pathDirectClean);
*/
  FILE *f = fopen(pathDirectClean,"rw+b");
  if(f==NULL){
         printf("ERROR! El disco no fue encontrado para escribir el MBR.\n");
         return;
  }else{
      rewind(f);//Posicionamos el indicador de posicion del archivo al inicio del mismo
      fwrite(&newMBR,sizeof(newMBR),1,f);
      fclose(f);
      printf("MBR dentro Disco creado exitosamente!!\n\n");
  }


  //PRUEBA DE LEER EL DISCO Y SU MBR
  FILE *f2 = fopen(pathDirectClean,"r+b");
        if(f2==NULL){
               printf("ERROR! No fue posible encontrar el disco para leer mbr.\n");
               return;
        }else{
            rewind(f2);//Posicionamos el indicador de posicion del archivo al inicio del mismo
            structMBR mbr;
            fread(&mbr,(int)sizeof(mbr),1,f2);
            printf("NOMBRE         VALOR \n");
            printf("mbr_tama?o:     %d \n",mbr.mbr_tamano);
            printf("mbr_fecha:      %s \n",mbr.mbr_fecha_creacion);
            printf("mbr_Id_Disk:    %d \n",mbr.mbr_disk_signature);
            printf("part_status1    %s \n",mbr.mbr_partition_1.part_status);
            printf("part_status2    %s \n",mbr.mbr_partition_2.part_status);
            printf("part_status3    %s \n",mbr.mbr_partition_3.part_status);
            printf("part_status4    %s \n",mbr.mbr_partition_4.part_status);
        }
}
