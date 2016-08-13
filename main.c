#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>

typedef struct ExtendedBootRecord{
    char part_status[2];
    char part_fit[3];//Indica tipo de ajuste: first,best,worst
    int part_start;//Indica el byte de inicio de la particion
    int part_size;//Indica el tamano total de la particion en bytes
    int part_next;//Byte en el q se esta el proximo EBR; -1 si no hay
    char part_name[16];//Nombre de la partiion
}structEBR;

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

typedef struct ParticionMontada{
    char idParticionMontada[5];
    char nombreParticion[16];
    char pathDisco[60];
}structParticionMontada;

typedef struct DiscoMontado{
    int noPartActivas;
    char letraAsignada[2];
    char pathDisco[60];
}structDiscoMontado;

//banderas
bool banderaNoHayEspacio=false;
bool banderaParticionesLlenas=false;
bool banderaYaExisteExtendida=false;
bool banderaNoHayEspacioEnExtendida=false;

//ATRIBUTOS TEMPORALES PARA mkdisk CREACION DE DISCO (Archivo Fisico)
FILE *auxArchivo; //FILE aux para crear del disco como un archivo.
char buffer[1];//Char de un caracter, igual a 1byte,
char fecha[16];
int sizeDiscTemp;
char pathDiscTemp[60];
char nameDiscTemp[10];
char unitDiscTemp[2]="m";

//ATRIBUTOS TEMPORALES PARA fdisk CREACION DE UNA PARTICION PRIMARIA O EXTENDIDA
int sizePartTemp;//Debe ser mayor a cero, sino mostrar error
char pathPartTemp[60];//Ruta del disco dodne se creara una particion
char namePartTemp[16];
char fitPartTemp[3]="wf";//bf=best fit, ff,firstfit, wf,worst fit
char unitPartTemp[2]="k";//bytes,kilobytes,megabytes
char typePartTemp[2]="p";//p=primaria, e=extendida, l=logica
int addPartTemp;
char deletePartTemp[5]="";

//ATRIBUTOS TEMPORALES PARA PODER MONTAR CUALQUIER PARTICION
char pathDiskToMountTemp[60];
char namePartToMountTemp[16];
structDiscoMontado listaDiscoMontados[6];//Lista de discos previamente identificados por una letra
int apuntadorListaDiscosMontados=0;
int auxLetraDiskAutoincrementable=97;//97=a va incrementando conforme se van montando discos siempre y cuando no se este ingresado un disco ya montado verificar x path.
structParticionMontada listaParticionesMontadas[12];//LISTA PARTICIONES MONTADAS
int apuntadorListaParticionesMontadas=0;

//ATRIBUTOS TEMPORALES PARA DESMONATAR PARTICIONES
char id1_temp[5];
char id2_temp[5];
char id3_temp[5];
char id4_temp[5];
char id5_temp[5];

//ATRIBUTOS TEMPORALES PARA LA GENERACION DE REPORTES
char nameRepTemp[8];
char pathRepTemp[60];
char idPartRepTemp[5];


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

int existeExtendida(char pathPartTemp[50]){
    int posicionInicioExtendida=-1;

    FILE *f=fopen(pathPartTemp,"rb+");
    if(f!=NULL){
        rewind(f);
        structMBR auxMBR;
        fread(&auxMBR,(int)sizeof(structMBR),1,f);
        if(strcmp(auxMBR.mbr_partition_1.part_type,"e")==0){
            posicionInicioExtendida=auxMBR.mbr_partition_1.part_start;
        }
        if(strcmp(auxMBR.mbr_partition_2.part_type,"e")==0){
            posicionInicioExtendida=auxMBR.mbr_partition_2.part_start;
        }
        if(strcmp(auxMBR.mbr_partition_3.part_type,"e")==0){
            posicionInicioExtendida=auxMBR.mbr_partition_3.part_start;
        }
        if(strcmp(auxMBR.mbr_partition_4.part_type,"e")==0){
            posicionInicioExtendida=auxMBR.mbr_partition_4.part_start;
        }
        fclose(f);
    }
    return posicionInicioExtendida;
}

int tamanoEnBytes(int tamano,char unidad[2]){
    if(strcmp(unidad,"b")==0){
        return tamano;
    }else if(strcmp(unidad,"k")==0){
        return tamano*1024;
    }else if(strcmp(unidad,"m")==0){
        return tamano*1024*1024;
    }else{
        printf("ERROR!!! La unidad del tamano de la particion no es valida.");
    }
}

void verificaSiHayEspacioEnExtendida(char pathPartTemp,int sizeNewEBR){
    FILE *f=fopen(pathPartTemp,"rb+");
    if(f!=NULL){
        rewind(f);
        structMBR auxMBR;
        fread(&auxMBR,(int)sizeof(structMBR),1,f);

        if(strcmp(auxMBR.mbr_partition_1.part_type,"e")==0){
            int sizeParticionExtendida=auxMBR.mbr_partition_1.part_size;
            printf("Size Extended partition: %d \n ",sizeParticionExtendida);

            int posExtendida=existeExtendida(pathPartTemp);

            FILE *f=fopen(pathPartTemp,"r+b");
            fseek(f,posExtendida,SEEK_SET);
            structEBR auxebr;
            fread(&auxebr,sizeof(structEBR),1,f);
            fclose(f);

            while(strcmp(auxebr.part_status,"1")==0){
                sizeParticionExtendida=sizeParticionExtendida-auxebr.part_size;
                printf(" - EBR %d = %d \n ",auxebr.part_size,sizeParticionExtendida);
                if(auxebr.part_next==-1){
                    //No hay siguiente ebr y/o logica
                    strcpy(auxebr.part_status,"0");
                }else{
                    //Si hay posicion de memoria para siguiente ebr o logica
                    FILE *f=fopen(pathPartTemp,"r+b");
                    fseek(f,auxebr.part_next,SEEK_SET);
                    fread(&auxebr,sizeof(structEBR),1,f);
                    fclose(f);
                }
            }

            sizeParticionExtendida=sizeParticionExtendida-sizeNewEBR;
            printf("- New Logical Partition will create %d = %d",sizeNewEBR,sizeParticionExtendida);
            if(sizeParticionExtendida<=0){
                banderaNoHayEspacioEnExtendida=true;
            }
        }

        if(strcmp(auxMBR.mbr_partition_2.part_type,"e")==0){
            int sizeParticionExtendida=auxMBR.mbr_partition_2.part_size;
            printf("Size Extended partition: %d \n ",sizeParticionExtendida);
            int posExtendida=existeExtendida(pathPartTemp);

            FILE *f=fopen(pathPartTemp,"r+b");
            fseek(f,posExtendida,SEEK_SET);
            structEBR auxebr;
            fread(&auxebr,sizeof(structEBR),1,f);
            fclose(f);

            while(strcmp(auxebr.part_status,"1")==0){
                sizeParticionExtendida=sizeParticionExtendida-auxebr.part_size;
                printf(" - EBR %d = %d \n ",auxebr.part_size,sizeParticionExtendida);
                if(auxebr.part_next==-1){
                    //No hay siguiente ebr y/o logica
                    strcpy(auxebr.part_status,"0");
                }else{
                    //Si hay posicion de memoria para siguiente ebr o logica
                    FILE *f=fopen(pathPartTemp,"r+b");
                    fseek(f,auxebr.part_next,SEEK_SET);
                    fread(&auxebr,sizeof(structEBR),1,f);
                    fclose(f);
                }
            }

            sizeParticionExtendida=sizeParticionExtendida-sizeNewEBR;
            printf("- New Logical Partition will create %d = %d",sizeNewEBR,sizeParticionExtendida);
            if(sizeParticionExtendida<=0){
                banderaNoHayEspacioEnExtendida=true;
            }
        }
        if(strcmp(auxMBR.mbr_partition_3.part_type,"e")==0){
            int sizeParticionExtendida=auxMBR.mbr_partition_3.part_size;
            printf("Size Extended partition: %d \n ",sizeParticionExtendida);

            int posExtendida=existeExtendida(pathPartTemp);

            FILE *f=fopen(pathPartTemp,"r+b");
            fseek(f,posExtendida,SEEK_SET);
            structEBR auxebr;
            fread(&auxebr,sizeof(structEBR),1,f);
            fclose(f);

            while(strcmp(auxebr.part_status,"1")==0){
                sizeParticionExtendida=sizeParticionExtendida-auxebr.part_size;
                printf(" - EBR %d = %d \n ",auxebr.part_size,sizeParticionExtendida);
                if(auxebr.part_next==-1){
                    //No hay siguiente ebr y/o logica
                    strcpy(auxebr.part_status,"0");
                }else{
                    //Si hay posicion de memoria para siguiente ebr o logica
                    FILE *f=fopen(pathPartTemp,"r+b");
                    fseek(f,auxebr.part_next,SEEK_SET);
                    fread(&auxebr,sizeof(structEBR),1,f);
                    fclose(f);
                }
            }

            sizeParticionExtendida=sizeParticionExtendida-sizeNewEBR;
            printf("- New Logical Partition will create %d = %d",sizeNewEBR,sizeParticionExtendida);
            if(sizeParticionExtendida<=0){
                banderaNoHayEspacioEnExtendida=true;
            }
        }
        if(strcmp(auxMBR.mbr_partition_4.part_type,"e")==0){
            int sizeParticionExtendida=auxMBR.mbr_partition_4.part_size;
            printf("Size Extended partition: %d \n ",sizeParticionExtendida);

            int posExtendida=existeExtendida(pathPartTemp);

            FILE *f=fopen(pathPartTemp,"r+b");
            fseek(f,posExtendida,SEEK_SET);
            structEBR auxebr;
            fread(&auxebr,sizeof(structEBR),1,f);
            fclose(f);

            while(strcmp(auxebr.part_status,"1")==0){
                sizeParticionExtendida=sizeParticionExtendida-auxebr.part_size;
                printf(" - EBR %d = %d \n ",auxebr.part_size,sizeParticionExtendida);
                if(auxebr.part_next==-1){
                    //No hay siguiente ebr y/o logica
                    strcpy(auxebr.part_status,"0");
                }else{
                    //Si hay posicion de memoria para siguiente ebr o logica
                    FILE *f=fopen(pathPartTemp,"r+b");
                    fseek(f,auxebr.part_next,SEEK_SET);
                    fread(&auxebr,sizeof(structEBR),1,f);
                    fclose(f);
                }
            }

            sizeParticionExtendida=sizeParticionExtendida-sizeNewEBR;
            printf("- New Logical Partition will create %d = %d",sizeNewEBR,sizeParticionExtendida);
            if(sizeParticionExtendida<=0){
                banderaNoHayEspacioEnExtendida=true;
            }
        }
        fclose(f);
    }
}

int calcularByteInicio(char pathDisco[50],int sizeParticionEnBytes){
    int byteInicio;

    FILE *f = fopen(pathDisco,"rb+");
    if(f==NULL){
           printf("ERROR! No fue posible encontrar el disco para calcular el inicio de la nueva particion.\n");
           return -1;
    }else{
        rewind(f);//Posicionamos el indicador de posicion del archivo al inicio del mismo
        structMBR mbr;
        fread(&mbr,(int)sizeof(mbr),1,f);

        byteInicio=(int)sizeof(mbr)+1;
        int sizeDisponible=mbr.mbr_tamano-(int)sizeof(mbr);
        printf("SizeDisk %d - sizeMBR %d  =  %d ",mbr.mbr_tamano,(int)sizeof(mbr),sizeDisponible);

        if(strcmp(mbr.mbr_partition_1.part_status,"1")==0){
            sizeDisponible=sizeDisponible-mbr.mbr_partition_1.part_size;
            printf(" - Part1 %d = %d ",mbr.mbr_partition_1.part_size,sizeDisponible);
        }
        if(strcmp(mbr.mbr_partition_2.part_status,"1")==0){
            sizeDisponible=sizeDisponible-mbr.mbr_partition_2.part_size;
            printf(" - Part2 %d = %d",mbr.mbr_partition_2.part_size,sizeDisponible);
        }
        if(strcmp(mbr.mbr_partition_3.part_status,"1")==0){
            sizeDisponible=sizeDisponible-mbr.mbr_partition_3.part_size;
            printf(" - Part3 %d = %d ",mbr.mbr_partition_3.part_size,sizeDisponible);
        }
        if(strcmp(mbr.mbr_partition_4.part_status,"1")==0){
            sizeDisponible=sizeDisponible-mbr.mbr_partition_4.part_size;
            printf(" - Part4 %d = %d",mbr.mbr_partition_4.part_size,sizeDisponible);
        }
        printf("\nEspacio Disponible en Disco: %d \n",sizeDisponible);

        printf("Espacio Disponible Despues de la Particion: %d \n",(sizeDisponible-sizeParticionEnBytes));
        if((sizeDisponible-sizeParticionEnBytes)>=0){
            //printf("EXISTE ESPACIO SUFICIENTE PARA CREAR LA PARTICION");
            if(strcmp(mbr.mbr_partition_1.part_status,"1")==0){
                byteInicio=byteInicio+mbr.mbr_partition_1.part_size;
            }
            if(strcmp(mbr.mbr_partition_2.part_status,"1")==0){
                byteInicio=byteInicio+mbr.mbr_partition_2.part_size;;
            }
            if(strcmp(mbr.mbr_partition_3.part_status,"1")==0){
                byteInicio=byteInicio+mbr.mbr_partition_3.part_size;
            }
            if(strcmp(mbr.mbr_partition_4.part_status,"1")==0){
                byteInicio=byteInicio+mbr.mbr_partition_4.part_size;
            }
        }else{
            banderaNoHayEspacio=true;
            byteInicio=-1;
        }
        fclose(f);
    }
    return byteInicio;
}

void mostrarMBR(char pathDirectClean[70]){
  //PRUEBA DE LEER EL DISCO Y SU MBR
  FILE *f2 = fopen(pathDirectClean,"r+b");
        if(f2==NULL){
               printf("ERROR! No fue posible encontrar el disco para leer mbr.\n");
               return;
        }else{
            rewind(f2);//Posicionamos el indicador de posicion del archivo al inicio del mismo
            structMBR mbr;
            fread(&mbr,(int)sizeof(mbr),1,f2);
            printf("NOMBRE          VALOR \n");
            printf("mbr_tama?o:     %d \n",mbr.mbr_tamano);
            printf("mbr_fecha:      %s \n",mbr.mbr_fecha_creacion);
            printf("mbr_Id_Disk:    %d \n",mbr.mbr_disk_signature);

            printf("part_status1    %s \n",mbr.mbr_partition_1.part_status);
            printf("part_type1    %s \n",mbr.mbr_partition_1.part_type);
            printf("part_fit1    %s \n",mbr.mbr_partition_1.part_fit);
            printf("part_start1    %d \n",mbr.mbr_partition_1.part_start);
            printf("part_size1    %d \n",mbr.mbr_partition_1.part_size);
            printf("part_name1    %s \n",mbr.mbr_partition_1.part_name);

            printf("part_status2    %s \n",mbr.mbr_partition_2.part_status);
            printf("part_type2    %s \n",mbr.mbr_partition_2.part_type);
            printf("part_fit2    %s \n",mbr.mbr_partition_2.part_fit);
            printf("part_start2    %d \n",mbr.mbr_partition_2.part_start);
            printf("part_size2    %d \n",mbr.mbr_partition_2.part_size);
            printf("part_name2    %s \n",mbr.mbr_partition_2.part_name);

            printf("part_status3    %s \n",mbr.mbr_partition_3.part_status);
            printf("part_type3    %s \n",mbr.mbr_partition_3.part_type);
            printf("part_fit3    %s \n",mbr.mbr_partition_3.part_fit);
            printf("part_start3    %d \n",mbr.mbr_partition_3.part_start);
            printf("part_size3    %d \n",mbr.mbr_partition_3.part_size);
            printf("part_name3    %s \n",mbr.mbr_partition_3.part_name);

            printf("part_status4    %s \n",mbr.mbr_partition_4.part_status);
            printf("part_type4    %s \n",mbr.mbr_partition_4.part_type);
            printf("part_fit4    %s \n",mbr.mbr_partition_4.part_fit);
            printf("part_start4    %d \n",mbr.mbr_partition_4.part_start);
            printf("part_size4    %d \n",mbr.mbr_partition_4.part_size);
            printf("part_name4    %s \n",mbr.mbr_partition_4.part_name);
        }
        fclose(f2);
}

void crearDiscoFisico(int sizeDiscTemp,char pathDiscTemp[50],char nameDiscTemp[10] ,char unitDiscTemp[2]){
   //NOS ASEGURAMOS DE QUE EXITAN TODAS LAS CARPETAS INDICADAS ANTES DE ESCRIBIR EL DISCTO.
    char pathClean[60];
    int j=0; //index of new array
    int i;
    for(i=0; i<60; i++){//Recorremos el array del la ruta de directorios y formamos nuevo array sin el caracter de comillas
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
    char pathDirectToFile[70];
    strcpy(pathDirectToFile,pathDiscTemp);
    strcat(pathDirectToFile,nameDiscTemp);
    //printf("Ruta directa al archivo con comillas: %s",pathDirectToFile);
    char pathDirectClean[70];
    int y=0; //index of new array
    int x;
    for(x=0; x<=70; x++){//Recorremos el array del la ruta de directorios y formamos nuevo array sin el caracter de comillas
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
  strcpy(partaux.part_status,"0");
  strcpy(partaux.part_type,"");
  strcpy(partaux.part_fit,"");
  strcpy(partaux.part_name,"");
  partaux.part_size=-1;
  partaux.part_start=-1;

  newMBR.mbr_partition_1=partaux;
  newMBR.mbr_partition_2=partaux;
  newMBR.mbr_partition_3=partaux;
  newMBR.mbr_partition_4=partaux;

  FILE *f = fopen(pathDirectClean,"rw+b");
  if(f==NULL){
         printf("ERROR! El disco no fue encontrado para escribir el MBR.\n");
         return;
  }else{
      rewind(f);//Posicionamos el indicador de posicion del archivo al inicio del mismo
      fwrite(&newMBR,sizeof(newMBR),1,f);
      fclose(f);
      printf("MBR y Disco Creados Exitosamente!!\n\n");
  }


    mostrarMBR(pathDirectClean);
}

void crearParticion(int sizePartTemp, char pathPartTemp[50], char namePartTemp[16], char unitPartTemp[2], char fitPartTemp[2], char typePartTemp[2])
{
    if(strcmp(typePartTemp,"p")==0 || strcmp(typePartTemp,"e")==0){

        structParticion newParticion;
        strcpy(newParticion.part_status,"1");
        strcpy(newParticion.part_type,typePartTemp);
        strcpy(newParticion.part_fit,fitPartTemp);
        strcpy(newParticion.part_name,namePartTemp);
        newParticion.part_size=tamanoEnBytes(sizePartTemp,unitPartTemp);
        //METODO QUE CALCULA EL BYTE DE INICIO DE LA PARTICION
        newParticion.part_start=calcularByteInicio(pathPartTemp,newParticion.part_size);

        if(banderaNoHayEspacio==false){
                FILE *f = fopen(pathPartTemp,"rb+");
                if(f==NULL){
                       printf("ERROR! No fue posible encontrar el disco para crear particion.\n");
                       return;
                }else{
                    rewind(f);//Posicionamos el indicador de posicion del archivo al inicio del mismo
                    structMBR mbr;
                    fread(&mbr,(int)sizeof(mbr),1,f);

                    //EXISTE ESPACIO SUFICIENTE PARA CREAR LA NUEVA PARTICION
                    //AHORA VERIFICAMOS SI NO SE HAN EXCEDIDO DE LAS 4 PARTICIONES PERMITIDAS
                    if(strcmp(mbr.mbr_partition_1.part_status,"1")==0 && strcmp(mbr.mbr_partition_2.part_status,"1")==0 && strcmp(mbr.mbr_partition_3.part_status,"1")==0 && strcmp(mbr.mbr_partition_4.part_status,"1")==0){
                        banderaParticionesLlenas=true;
                    }
                    //TAMBIEN VERIFICAMOS SI YA EXISTE UNA EXTENDIDA EN CASO QUE SE QUIERA INGRESAR OTRA NO PERMITIRLA
                    if(strcmp(mbr.mbr_partition_1.part_status,"1")==0 || strcmp(mbr.mbr_partition_2.part_status,"1")==0 || strcmp(mbr.mbr_partition_3.part_status,"1")==0 || strcmp(mbr.mbr_partition_4.part_status,"1")==0){
                        if(strcmp(mbr.mbr_partition_1.part_type,"e")==0 || strcmp(mbr.mbr_partition_2.part_type,"e")==0 || strcmp(mbr.mbr_partition_3.part_type,"e")==0 || strcmp(mbr.mbr_partition_4.part_type,"e")==0){
                           if(strcmp(newParticion.part_type,"e")==0){
                               banderaYaExisteExtendida=true;
                           }
                        }
                    }

                    if(banderaParticionesLlenas==false){
                        if(banderaYaExisteExtendida==false){
                            printf("NewPartition to create: %s ",newParticion.part_name);
                            printf("size: %d bytes ",newParticion.part_size);
                            printf("start: %d ",newParticion.part_start);
                            printf("type: %s ",newParticion.part_type);
                            printf("fit: %s ",newParticion.part_fit);
                            printf("status: %s \n",newParticion.part_status);

                            if(strcmp(mbr.mbr_partition_1.part_status,"0")==0){
                                mbr.mbr_partition_1=newParticion;
                                printf("particion creado en particion 1 ");
                            }else if(strcmp(mbr.mbr_partition_2.part_status,"0")==0){
                                mbr.mbr_partition_2=newParticion;
                                printf("particion creado en particion 2 ");
                            }else if(strcmp(mbr.mbr_partition_3.part_status,"0")==0){
                                mbr.mbr_partition_3=newParticion;
                                printf("particion creado en particion 3 ");
                            }else if(strcmp(mbr.mbr_partition_4.part_status,"0")==0){
                                mbr.mbr_partition_4=newParticion;
                                printf("particion creado en particion 4 ");
                            }
                            fclose(f);

                            FILE *f2 = fopen(pathPartTemp,"rb+");
                            rewind(f2);//Posicionamos el indicador de posicion del archivo al inicio del mismo
                            fwrite(&mbr,sizeof(mbr),1,f2);
                            fclose(f2);

                            if(strcmp(newParticion.part_type,"e")==0){
                                printf("Particion Extendida creada exitosamente con un EBR inactivo.!!\n");
                                //Despues de tener la particion extendida, escribimos un EBR al inicio con estado D=Disable q estara lista para alojar una particion logica al momento q se desee crear una
                                FILE *f3 = fopen(pathPartTemp,"rb+");
                                rewind(f3);//Posicionamos el indicador de posicion del archivo al inicio del mismo
                                structEBR auxEBR;
                                strcpy(auxEBR.part_status,"0");
                                fseek(f3,newParticion.part_start,SEEK_SET);
                                fwrite(&auxEBR,sizeof(structEBR),1,f3);
                                fclose(f3);
                            }else{
                                printf("Particion primaria creada exitosamente!\n\n");
                            }

                        }else{
                            printf("ERROR!!! YA EXISTE UNA PARTICION EXTENDIDA.\n");
                            banderaYaExisteExtendida=false;
                        }
                    }else{
                        printf("ERROR!!! SE HAN ALCANZADO EL NUMERO DE PARTICIONES PERMITIDAS.\n");
                        banderaParticionesLlenas=false;
                    }
                }
        }else{
            //NO FUE POSIBLE CREAR LA PARTICION POR FALTA DE ESPACIO
            printf("ERROR!!! ESPACIO INSUFICIENTE EN DISCO PARA CREAR LA PARTICION.\n");
            banderaNoHayEspacio=false;
        }
    }

    if(strcmp(typePartTemp,"l")==0){
        //1. VERIFICAR SI EXISTE UNA PARTICION EXTENDIDA, SI EXISTE OBTENER POSICION DE INICIO PARA ESCRIBIR LOGICA, SI NO MOSTRAR MENSAJE DE ERROR
        if(existeExtendida(pathPartTemp)!=-1){
            verificaSiHayEspacioEnExtendida(pathPartTemp,tamanoEnBytes(sizePartTemp,unitPartTemp));
            if (banderaNoHayEspacioEnExtendida==false){//SI HAY ESPACIO EMPEZAMOS A LEER EL PRIMER EBR
                //2. LEER EL PRIMER EXTENDED BOOT RECORD PARA VER SI YA ESTA ACTIVA Y BUSCAR EN QUE POSICION IRA EL NUEVO EBR
                FILE *f1=fopen(pathPartTemp,"rb+");
                int posExtendida=existeExtendida(pathPartTemp);
                fseek(f1,posExtendida,SEEK_SET);
                structEBR ebrAux;
                fread(&ebrAux,sizeof(structEBR),1,f1);
                //Leeremos nuestro primer ebr para ver si esta activa y buscar nueva posicion de la nueva particion logica.
                if(strcmp(ebrAux.part_status,"0")==0){//Si no esta activa, alojamos la primera particion logica ahi mismo
                    structEBR newEBR;
                    strcpy(newEBR.part_fit,fitPartTemp);
                    strcpy(newEBR.part_name,namePartTemp);
                    strcpy(newEBR.part_status,"1");
                    newEBR.part_size=tamanoEnBytes(sizePartTemp,unitPartTemp);
                    newEBR.part_next=-1;
                    newEBR.part_start=posExtendida+(int)sizeof(newEBR);

                    fseek(f1,posExtendida,SEEK_SET);
                    fwrite(&newEBR,sizeof(structEBR),1,f1);
                    fclose(f1);
                    printf("Particion Logica creada exitosamente en particion extendida principal.!!\n");
                }else{//Si ya se encuentra activa, leer la posicion del siguiente extended boot record y repetIr verifiacion si ya esta acriva, en caso es -1 esta existe siguiente ebr alojarla como la siguiente
                    if(ebrAux.part_next==-1){//si no existe siguiente ebr, entonces agregamos le nueva particion logica a la lista
                        structEBR newEBR;
                        strcpy(newEBR.part_fit,fitPartTemp);
                        strcpy(newEBR.part_name,namePartTemp);
                        strcpy(newEBR.part_status,"1");
                        newEBR.part_size=tamanoEnBytes(sizePartTemp,unitPartTemp);
                        newEBR.part_next=-1;
                        newEBR.part_start=posExtendida+(int)sizeof(newEBR)+ebrAux.part_size+(int)sizeof(newEBR)+1;

                        int siguiete=posExtendida+(int)sizeof(newEBR)+ebrAux.part_size+1;
                        //Debido ah que alteraremos el atributos part_next del ebrAux, lo editamos y lo escribios el cambio en el disco
                        ebrAux.part_next=siguiete;
                        fseek(f1,posExtendida,SEEK_SET);
                        fwrite(&ebrAux,sizeof(structEBR),1,f1);
                        fclose(f1);
                        //Escribimos la segunda particion en su posicion de inicio
                        FILE *f2=fopen(pathPartTemp,"rw+b");
                        fseek(f2,siguiete,SEEK_SET);
                        fwrite(&newEBR,sizeof(structEBR),1,f2);
                        fclose(f2);


                        printf("Particion Logica creada exitosamente en particion extendida.!!\n");
                    }else{//Si no es -1 entonces existe una segunda particion logica activa,.... ent este caso seguir leyendo el tercero y asi sucesivamente...
                        printf("Se prentende crear una tercera o cuarta o quinta particion extendida....\n");
                    }
                }
            }else{//NO HAY ESPACIO, MOSTRAMOS MENSAJE DE ERROR
                printf("Espacio Insuficiente en la particion Extendida para alojar nuevo particion Logica.");
            }
        }else{
            printf("ERROR! NO EXISTE PARTICION EXTENDIDA PARA ALOJAR PARTICION LOGICA\n");
        }

    }
}

void evaluarAtributosCrearDisco(char * tokenASeparar){
    int cont=0;//cont auxiliar para determinar si estamos leyendo el primer o el segundo valor -Val1=Val2
    char *tipo; //tipo auxiliar usado en lectura segundo valor para determinar que tipo de valor viene segun val1 ledio en primera pasada

    char *cadena;
    while(cadena=strtok_r(tokenASeparar,"::",&tokenASeparar)){

        cont=cont+1;//Cuando entramos a leer el primer valor del tokenASeparar, dejamos evidencia q estamos leyendo el primer valor
        if(cont==1){//Si es primera vez q pasa por aqui el token es el q nos indica el tipo de valor q obtendremos
            //printf("TIPO: %s\n",token);
            tipo=cadena;
        }else if(cont==2){//Ya sabremos que tipo de valor otendremos aqui...xq en el la primera pasada lo determinamos
            //printf("VALOR: %s\n",token);
            if(strcmp(tipo,"-size")==0){
                sizeDiscTemp=atoi(cadena);//Convert char* to int
                //printf("Size: %d",sizeDiscTemp);
            }else if(strcmp(tipo,"+unit")==0){
                strcpy(unitDiscTemp,cadena);
            }else if(strcmp(tipo,"-path")==0){
                strcpy(pathDiscTemp,cadena); //strcpy(path,pathDiscTemp);
                //printf("PathDisk: %s",pathDiscTemp);
            }else if(strcmp(tipo,"-name")==0){
                strcpy(nameDiscTemp,cadena);
                //printf("NameDisk: %s",nameDiscTemp);
            }else{
                printf("ERROR: ATRIBUTO DE MKDISK NO IDENTIFICADO.\n");
            }//FIN IF si atributo es identificado o no Identificado
        }//FIN IF si es primera o segunda lectura de token

    }//FIN WHILE estamos leyendo los dos valores de la cadena a separar por ":"
}

void crearDisco(char * listaAtributosPorEspacio){
        //En la lista tenemos los tokenes de tipo -TIPO=VALOR
        char *token;
        for (token = listaAtributosPorEspacio; token != NULL; token = strtok(NULL, " "))
        {
          //printf("Lista Atributos MKDISK: %s\n",token);
          evaluarAtributosCrearDisco(token);//Mandamos el token, para que sea separado por ":" y guardar valor segun su tipo
        }

        /*DESPUES DE LEER TODOS LOS ATRIBUTOS Y TENERLOS EN VARIABLES TERMPORALES
        *VERIFICAMOS SI HAY SUFICIENTES DATOS COMO PARA CREAR UN DISCO NUEVO. DE NO TENER DATOS MINIMOS
        * NO CREAMOS EL DISCO Y MOSTRAMOS MENSAJE DE ERROR QUE NO SE ENCONTRARON SUFIICIENTES DATOS
        */
        //printf("Size: %d",sizeDiscTemp);printf("PathDisk: %s ",pathDiscTemp);printf("NameDisk: %s",nameDiscTemp);
        if((sizeDiscTemp>0) && (strcmp(pathDiscTemp,"")!=0) && (strcmp(nameDiscTemp,"")!=0)){
            //VIENEN DATOS MINIMOS -> CREAMOS DISC
            crearDiscoFisico(sizeDiscTemp,pathDiscTemp,nameDiscTemp,unitDiscTemp);
        }else{
            //NO HAY DATOS SUFICIENTES PARA CREAR DISCO
            printf("Error: Datos insuficientes para crear nuevo disco.\n");
        }
        //SE HAYA O NO CRADO EL DISCO, REGRESAMOS LAS VARIABLES TEMPORALES A SUS VALORES INICIALES PARA NO REPETIR VALORES
        sizeDiscTemp=NULL;
        strcpy(pathDiscTemp,"");
        strcpy(nameDiscTemp,"");
        strcpy(unitDiscTemp,"m");
}

void evaluarAtributosEliminarDisco(char * tokenASeparar){
    int cont=0;//cont auxiliar para determinar si estamos leyendo el primer o el segundo valor -Val1=Val2
    char *tipo; //tipo auxiliar usado en lectura segundo valor para determinar que tipo de valor viene segun val1 ledio en primera pasada
    char *cadena;
    while(cadena=strtok_r(tokenASeparar,"::",&tokenASeparar)){
        cont=cont+1;//Cuando entramos a leer el primer valor del tokenASeparar, dejamos evidencia q estamos leyendo el primer valor
        if(cont==1){//Si es primera vez q pasa por aqui el token es el q nos indica el tipo de valor q obtendremos
            //printf("TIPO: %s\n",token);
            tipo=cadena;
        }else if(cont==2){//Ya sabremos que tipo de valor otendremos aqui...xq en el la primera pasada lo determinamos
            //printf("VALOR: %s\n",token);
            if(strcmp(tipo,"-path")==0){
                char pathClean[60];
                int j=0; //index of new array
                int i;
                for(i=0; i<50; i++){//Recorremos el array del la ruta de directorios y formamos nuevo array sin el caracter de comillas
                    if(cadena[i]=='"'){
                        //Nada q hacer, Ignoramos el caracter
                    }else{
                        pathClean[j]=cadena[i];//Cualquier otro caracter lo compiamos al nuevo array
                        j=j+1;//y aumentamos el indice del nuevo array para estar listo en recibir otro caracter
                    }
                }
                //Despues de haber limpiado el path de las comillas, procedemos con la ELIMINACION
                if(remove(pathClean)==0){
                    printf("Disco \"%s\" Eliminado Exitosamente! \n\n",pathClean);
                }else{
                    printf("No se pudo Eliminar el Disco, Revice bien la ruta al archivo y vuelta a intentar.\n\n");
                }

            }else{
                printf("ERROR: ATRIBUTO DE RMDISK NO IDENTIFICADO\n");
            }//FIN IF si atributo es identificado o no Identificado
        }//FIN IF si es primera o segunda lectura de token
    }//FIN WHILE estamos leyendo los dos valores de la cadena a separar por ":"
}

void eliminarDisco(char * listaAtributosPorEspacio){
    //En la lista tenemos los tokenes de tipo -TIPO=VALOR(-path::/RUTA/DISCO.DSK)
    char * token;
    for (token = listaAtributosPorEspacio; token != NULL; token = strtok(NULL, " "))
    {
      //printf("Lista Atributos rMdisk: %s\n",token);
      evaluarAtributosEliminarDisco(token);//Mandamos el token, para que sea separado por ":" y guardar valor segun su tipo
    }
}

void verificarAtributosManejoParticiones(char * tokenASeparar){
    int cont=0;//cont auxiliar para determinar si estamos leyendo el primer o el segundo valor -Val1=Val2
    char *tipo; //tipo auxiliar usado en lectura segundo valor para determinar que tipo de valor viene segun val1 ledio en primera pasada
    char *cadena;
    while(cadena=strtok_r(tokenASeparar,"::",&tokenASeparar)){
        cont=cont+1;//Cuando entramos a leer el primer valor del tokenASeparar, dejamos evidencia q estamos leyendo el primer valor
        if(cont==1){//Si es primera vez q pasa por aqui el token es el q nos indica el tipo de valor q obtendremos
            //printf("TIPO: %s\n",token);
            tipo=cadena;
        }else if(cont==2){//Ya sabremos que tipo de valor obtendremos aqui...xq en el la primera pasada lo determinamos
            //printf("VALOR: %s\n",token);
            if(strcmp(tipo,"-size")==0){
                sizePartTemp=atoi(cadena);//Convert char* to int
            }else if(strcmp(tipo,"-path")==0){
                strcpy(pathPartTemp,cadena);
            }else if(strcmp(tipo,"-name")==0){
                strcpy(namePartTemp,cadena);
            }else if(strcmp(tipo,"+unit")==0){
                strcpy(unitPartTemp,cadena);
            }else if(strcmp(tipo,"+fit")==0){
                strcpy(fitPartTemp,cadena);
            }else if(strcmp(tipo,"+type")==0){
                strcpy(typePartTemp,cadena);
            }else if(strcmp(tipo,"+delete")==0){
                strcpy(deletePartTemp,cadena);
            }else if(strcmp(tipo,"+add")==0){
                addPartTemp=atoi(cadena);
            }else{
                printf("ERROR!!! ATRIBUTO DE FDISK NO IDENTIFICADO.\n");
            }//FIN IF si atributo es identificado o no Identificado
        }//FIN IF si es primera o segunda lectura de token
    }//FIN WHILE estamos leyendo los dos valores de la cadena a separar por ":"
}

void administrarParticion(char * listaAtributosPorEspacio){
    //En la lista tenemos los tokenes de tipo -TIPO=VALOR
    char *token;
    for (token = listaAtributosPorEspacio; token != NULL; token = strtok(NULL, " "))
    {
      verificarAtributosManejoParticiones(token);//Mandamos el token, para que sea separado por ":" y guardar valor segun su tipo
    }

    /*DESPUES DE LEER TODOS LOS ATRIBUTOS Y TENERLOS EN VARIABLES TERMPORALES VERIFICAMOS SI HAY SUFICIENTES
    *DATOS COMO PARA CREAR UNA PARTICION, ELIMINAR UNA PARTICION O RESIZE DE LA PARTICION.*/
    if(strcmp(deletePartTemp,"")!=0){//SI SE HACE LLAMAR AL COMANDO DELETE::TYPE SE DESEA ELIMINAR UNA PARTICION

    }else if(addPartTemp!=NULL){//sI SE HACE LLAMAR AL COMANDO ADD::SIZE SE DESEA CAMBIAR DE TAMANO DE LA PARTICION

    }else{//SI NO SE LLAMA EL COMANDO DELETE NI ADD ENTONCES SE DESEA CREAR UNA NUEVA PARTICION
        /*DE NO TENER DATOS MINIMOS NO CREAMOS LA PARTICION Y MOSTRAMOS LOS MENSAJES DE ERRORES    */
        if((sizePartTemp>0) && (strcmp(pathPartTemp,"")!=0) && (strcmp(namePartTemp,"")!=0)){
            //VIENEN DATOS MINIMOS -> CREAMOS PARTICION

            //Limpiamos el PathDelDisco de comillas antes de mandarlo
            char pathClean[60];
            int j=0; //index of new array
            int i;
            for(i=0; i<60; i++){//Recorremos el array del la ruta de directorios y formamos nuevo array sin el caracter de comillas
                if(pathPartTemp[i]=='"'){
                    //Nada q hacer, Ignoramos el caracter
                }else{
                    pathClean[j]=pathPartTemp[i];//Cualquier otro caracter lo compiamos al nuevo array
                    j=j+1;//y aumentamos el indice del nuevo array para estar listo en recibir otro caracter
                }
            }
            crearParticion(sizePartTemp,pathClean,namePartTemp,unitPartTemp,fitPartTemp,typePartTemp);
            //mostrarMBR(pathClean);
        }else{
            //NO HAY DATOS SUFICIENTES PARA CREAR DISCO
        printf("Error: Datos insuficientes para crear particion.\n");

        }
    }

    //SE HAYA O NO CRADO LA PARTICION, REGRESAMOS LAS VARIABLES TEMPORALES A SUS VALORES INICIALES PARA NO REPETIR VALORES
    sizePartTemp=NULL;
    strcpy(pathPartTemp,"");
    strcpy(namePartTemp,"");
    strcpy(fitPartTemp,"wf");
    strcpy(unitPartTemp,"k");
    strcpy(typePartTemp,"p");
}

char obtenerLetraDisco(char path[50]){
    char letra;
    int i;
        for(i=0; i<6; i++){
            if(strcmp(listaDiscoMontados[i].pathDisco,path)==0){
                letra=listaDiscoMontados[i].letraAsignada[0];
            }
        }
    return letra;
}

char obtenerNumeroDeParticionActivaDeDisco(char path[50]){
    char leter[2];
    int i;
    for(i=0; i<6; i++){
        if(strcmp(listaDiscoMontados[i].pathDisco,path)==0){
            int idSiguientePartActivaDeDisco=listaDiscoMontados[i].noPartActivas+1;
            listaDiscoMontados[i].noPartActivas=listaDiscoMontados[i].noPartActivas+1;
            leter[0]=idSiguientePartActivaDeDisco+ '0';
        }
    }
    return leter[0];
}

bool estaDiscoPreviamenteIdentificado(char path[50]){
    bool estado=false;
    int i;
    for(i=0; i<6; i++){
        if(strcmp(listaDiscoMontados[i].pathDisco,path)==0){
            estado=true;
        }
    }
    return estado;
}

void mostrarListaParticionesMontadas(){
    int i;
    printf("\nLISTA PARTICIONES MONTADAS AL SISTEMA:\n");
    for(i=0;i<12;i++){
        if(strcmp(listaParticionesMontadas[i].idParticionMontada,"")!=0){
            printf("ID: %s  NAME: %s  PATH:%s \n",listaParticionesMontadas[i].idParticionMontada,listaParticionesMontadas[i].nombreParticion,listaParticionesMontadas[i].pathDisco);
        }
    }
    printf("\n");
}

void generarIdentificadorParticion(char path[50],char name[16]){
    //Antes de montar particion, Primero identificamos el disco con una letra, empezando desde: A hasta: Z asi susecivamente... y la registramos en el sistema.

    if(auxLetraDiskAutoincrementable==97){//Si no se ha utilizado el caracter '97' "a" se regsitra entonces el primer disco como "a"
        structDiscoMontado auxDiskMontado;
        auxDiskMontado.letraAsignada[0]=(char)auxLetraDiskAutoincrementable;
        auxLetraDiskAutoincrementable=auxLetraDiskAutoincrementable+1;
        strcpy(auxDiskMontado.pathDisco,path);

        listaDiscoMontados[apuntadorListaDiscosMontados]=auxDiskMontado;
        apuntadorListaDiscosMontados=apuntadorListaDiscosMontados+1;
        printf("Desde el registro del nuevo disco: %s letra Asignada: %c \n",auxDiskMontado.pathDisco,auxDiskMontado.letraAsignada[0]);

        //Despues de tener registrado ya un disco con su respetiva LETRA, procedemos a MONTAR la primera particion
        structParticionMontada auxParticionAMontar;
        strcpy(auxParticionAMontar.pathDisco,path);
        strcpy(auxParticionAMontar.nombreParticion,name);
        auxParticionAMontar.idParticionMontada[0]='v';
        auxParticionAMontar.idParticionMontada[1]='d';
        auxParticionAMontar.idParticionMontada[2]=obtenerLetraDisco(path);
        auxParticionAMontar.idParticionMontada[3]=obtenerNumeroDeParticionActivaDeDisco(path);
        listaParticionesMontadas[apuntadorListaParticionesMontadas]=auxParticionAMontar;
        apuntadorListaParticionesMontadas=apuntadorListaParticionesMontadas+1;

        printf("Particion: %s  nombre: %s  path:%s \n",auxParticionAMontar.idParticionMontada,auxParticionAMontar.nombreParticion,auxParticionAMontar.pathDisco);

    }else {//A LA SEGUNDA, POSIBLEMENTE EL DISCO YA SE ENCUNTRE IDENTIFICADO CON UNA LETRA, BUSCARLA EN LA LISTA SI NO REGITRARLA Y MONTAR PARTICION
        if(estaDiscoPreviamenteIdentificado(path)==true){//VERIFICAR SI SE ENCUENTRA YA IDENTIFICADO EL DISCO CON SU RESPECTIVA LETRA
            //sI SE ENCUENTRA, MONTAR PARTICION
            structParticionMontada auxParticionAMontar;
            strcpy(auxParticionAMontar.pathDisco,path);
            strcpy(auxParticionAMontar.nombreParticion,name);
            auxParticionAMontar.idParticionMontada[0]='v';
            auxParticionAMontar.idParticionMontada[1]='d';
            auxParticionAMontar.idParticionMontada[2]=obtenerLetraDisco(path);
            auxParticionAMontar.idParticionMontada[3]=obtenerNumeroDeParticionActivaDeDisco(path);
            listaParticionesMontadas[apuntadorListaParticionesMontadas]=auxParticionAMontar;
            apuntadorListaParticionesMontadas=apuntadorListaParticionesMontadas+1;

            printf("Particion: %s  nombre: %s  path:%s \n",auxParticionAMontar.idParticionMontada,auxParticionAMontar.nombreParticion,auxParticionAMontar.pathDisco);

        }else{//SI NO, IDENTIFICARLA E MONTAR PARTICION

            structDiscoMontado auxDiskMontado;
            auxDiskMontado.letraAsignada[0]=(char)auxLetraDiskAutoincrementable;
            auxLetraDiskAutoincrementable=auxLetraDiskAutoincrementable+1;
            strcpy(auxDiskMontado.pathDisco,path);

            listaDiscoMontados[apuntadorListaDiscosMontados]=auxDiskMontado;
            apuntadorListaDiscosMontados=apuntadorListaDiscosMontados+1;
            printf("Desde el registro del nuevo disco: %s letra Asignada: %c \n",auxDiskMontado.pathDisco,auxDiskMontado.letraAsignada[0]);

            //Despues de tener registrado ya un disco con su respetiva LETRA, procedemos a MONTAR siguiente particion del nuevo disco
            structParticionMontada auxParticionAMontar;
            strcpy(auxParticionAMontar.pathDisco,path);
            strcpy(auxParticionAMontar.nombreParticion,name);
            auxParticionAMontar.idParticionMontada[0]='v';
            auxParticionAMontar.idParticionMontada[1]='d';
            auxParticionAMontar.idParticionMontada[2]=obtenerLetraDisco(path);
            auxParticionAMontar.idParticionMontada[3]=obtenerNumeroDeParticionActivaDeDisco(path);
            listaParticionesMontadas[apuntadorListaParticionesMontadas]=auxParticionAMontar;
            apuntadorListaParticionesMontadas=apuntadorListaParticionesMontadas+1;

            printf("Particion: %s  nombre: %s  path:%s \n",auxParticionAMontar.idParticionMontada,auxParticionAMontar.nombreParticion,auxParticionAMontar.pathDisco);

        }

    }

    mostrarListaParticionesMontadas();
}

void verificarAtributosMontajeParticion(char * tokenASeparar){
    int cont=0;//cont auxiliar para determinar si estamos leyendo el primer o el segundo valor -Val1=Val2
    char *tipo; //tipo auxiliar usado en lectura segundo valor para determinar que tipo de valor viene segun val1 ledio en primera pasada
    char *cadena;
    while(cadena=strtok_r(tokenASeparar,"::",&tokenASeparar)){
        cont=cont+1;//Cuando entramos a leer el primer valor del tokenASeparar, dejamos evidencia q estamos leyendo el primer valor
        if(cont==1){//Si es primera vez q pasa por aqui el token es el q nos indica el tipo de valor q obtendremos
            //printf("TIPO: %s\n",token);
            tipo=cadena;
        }else if(cont==2){//Ya sabremos que tipo de valor obtendremos aqui...xq en el la primera pasada lo determinamos
            //printf("VALOR: %s\n",token);
            if(strcmp(tipo,"-path")==0){
                strcpy(pathDiskToMountTemp,cadena);
            }else if(strcmp(tipo,"-name")==0){
                strcpy(namePartToMountTemp,cadena);
            }else{
                printf("ERROR!!! ATRIBUTO DE MOUNT NO IDENTIFICADO.\n");
            }//FIN IF si atributo es identificado o no Identificado
        }//FIN IF si es primera o segunda lectura de token
    }//FIN WHILE estamos leyendo los dos valores de la cadena a separar por ":"
}

void montarParticion(char * listaAtributosPorEspacio){
    //En la lista tenemos los tokenes de tipo -TIPO=VALOR
    char * token;
    for (token = listaAtributosPorEspacio; token != NULL; token = strtok(NULL, " "))
    {
      verificarAtributosMontajeParticion(token);//Mandamos el token, para que sea separado por ":" y guardar valor segun su tipo
    }

    /*DESPUES DE LEER TODOS LOS ATRIBUTOS Y TENERLOS EN VARIABLES TERMPORALES
    *VERIFICAMOS SI HAY SUFICIENTES DATOS COMO PARA CREAR UNA PARTICION. DE NO TENER DATOS MINIMOS
    * NO MONTAMOS
    */
    if((strcmp(pathDiskToMountTemp,"") && (namePartToMountTemp!=NULL) !=0)){
        //VIENEN DATOS MINIMOS -> MONTAMOS PARTICION
        generarIdentificadorParticion(pathDiskToMountTemp,namePartToMountTemp);
    }else{
        //NO HAY DATOS SUFICIENTES PARA MONTAR PARTICION
        printf("Error: Datos insuficientes para montar particion.\n");

    }
    //SE HAYA O NO MONTADO LA PARTICION, REGRESAMOS LAS VARIABLES TEMPORALES A SUS VALORES INICIALES PARA NO REPETIR VALORES
    strcpy(pathDiskToMountTemp,"");
    strcpy(namePartToMountTemp,"");
}

void verificarAtributosDesmontajeParticion(char * tokenASeparar){
 int cont=0;//cont auxiliar para determinar si estamos leyendo el primer o el segundo valor -Val1=Val2
    char *tipo; //tipo auxiliar usado en lectura segundo valor para determinar que tipo de valor viene segun val1 ledio en primera pasada
    char *cadena;
    while(cadena=strtok_r(tokenASeparar,"::",&tokenASeparar)){
        cont=cont+1;//Cuando entramos a leer el primer valor del tokenASeparar, dejamos evidencia q estamos leyendo el primer valor
        if(cont==1){//Si es primera vez q pasa por aqui el token es el q nos indica el tipo de valor q obtendremos
            //printf("TIPO: %s\n",token);
            tipo=cadena;
        }else if(cont==2){//Ya sabremos que tipo de valor obtendremos aqui...xq en el la primera pasada lo determinamos
            //printf("VALOR: %s\n",token);
            if(strcmp(tipo,"-id1")==0){
                strcpy(id1_temp,cadena);
            }else if(strcmp(tipo,"-id2")==0){
                strcpy(id2_temp,cadena);
            }else if(strcmp(tipo,"-id3")==0){
                strcpy(id3_temp,cadena);
            }else if(strcmp(tipo,"-id4")==0){
                strcpy(id2_temp,cadena);
            }else if(strcmp(tipo,"-id5")==0){
                strcpy(id5_temp,cadena);
            }else{
                printf("ERROR!!! ATRIBUTO DE UMOUNT NO IDENTIFICADO.\n");
            }//FIN IF si atributo es identificado o no Identificado
        }//FIN IF si es primera o segunda lectura de token
    }//FIN WHILE estamos leyendo los dos valores de la cadena a separar por ":"
}

int verificarSiIDestaMontado(char id[5]){
    int indiceParticion=-1;
    int i;
    for(i=0; i<12; i++){//Recorremos toda la lista de particiones montadas para verificar si existe la particion indicada y obtener su indice
            if(strcmp(listaParticionesMontadas[i].idParticionMontada,id)==0){
                //Se encontro el id, osea que si esta montado, obtenemos su indice en la lista
                indiceParticion=i;
            }
    }//fin for que recorre toda la lista de particiones montadas que es estatico y tiene el valor de 12
    return indiceParticion;
}

void desmontarParticionesSolicitadas(char id1[5],char id2[5],char id3[5],char id4[5],char id5[5]){
    if(strcmp(id1,"")!=0){
        int indiceID1=verificarSiIDestaMontado(id1);
        if(indiceID1!=-1){
            strcpy(listaParticionesMontadas[indiceID1].idParticionMontada,"");
            strcpy(listaParticionesMontadas[indiceID1].nombreParticion,"");
            strcpy(listaParticionesMontadas[indiceID1].pathDisco,"");
            printf("La particion %s ha sido desmontada con exito!!\n",id1);
        }else{
            printf("El ID::%s no existe o no ha sido montado previamente!!\n",id1);
        }
    }

    if(strcmp(id2,"")!=0){
        int indiceID2=verificarSiIDestaMontado(id2);
        if(indiceID2!=-1){
            strcpy(listaParticionesMontadas[indiceID2].idParticionMontada,"");
            strcpy(listaParticionesMontadas[indiceID2].nombreParticion,"");
            strcpy(listaParticionesMontadas[indiceID2].pathDisco,"");
            printf("La particion %s ha sido desmontada con exito!!\n",id2);
        }else{
            printf("El ID::%s no existe o no ha sido montado previamente!!\n",id2);
        }
    }

    if(strcmp(id3,"")!=0){
        int indiceID3=verificarSiIDestaMontado(id3);
        if(indiceID3!=-1){
            strcpy(listaParticionesMontadas[indiceID3].idParticionMontada,"");
            strcpy(listaParticionesMontadas[indiceID3].nombreParticion,"");
            strcpy(listaParticionesMontadas[indiceID3].pathDisco,"");
            printf("La particion %s ha sido desmontada con exito!!\n",id3);
        }else{
            printf("El ID::%s no existe o no ha sido montado previamente!!\n",id3);
        }
    }

    if(strcmp(id4,"")!=0){
        int indiceID4=verificarSiIDestaMontado(id4);
        if(indiceID4!=-1){
            strcpy(listaParticionesMontadas[indiceID4].idParticionMontada,"");
            strcpy(listaParticionesMontadas[indiceID4].nombreParticion,"");
            strcpy(listaParticionesMontadas[indiceID4].pathDisco,"");
            printf("La particion %s ha sido desmontada con exito!!\n",id4);
        }else{
            printf("El ID::%s no existe o no ha sido montado previamente!!\n",id4);
        }
    }

    if(strcmp(id5,"")!=0){
        int indiceID5=verificarSiIDestaMontado(id5);
        if(indiceID5!=-1){
            strcpy(listaParticionesMontadas[indiceID5].idParticionMontada,"");
            strcpy(listaParticionesMontadas[indiceID5].nombreParticion,"");
            strcpy(listaParticionesMontadas[indiceID5].pathDisco,"");
            printf("La particion %s ha sido desmontada con exito!!\n",id5);
        }else{
            printf("El ID::%s no existe o no ha sido montado previamente!!\n",id5);
        }
    }

    mostrarListaParticionesMontadas();
}

void desMontarParticion(char * listaAtributosPorEspacio){
    //En la lista tenemos los tokens de tipo -TIPO=VALOR
    char * token;
    for (token = listaAtributosPorEspacio; token != NULL; token = strtok(NULL, " "))
    {
      verificarAtributosDesmontajeParticion(token);//Mandamos el token, para que sea separado por ":" y guardar valor segun su tipo
    }

    /*DESPUES DE LEER TODOS LOS ATRIBUTOS Y TENERLOS EN VARIABLES TERMPORALES
    *VERIFICAMOS SI HAY SUFICIENTES DATOS COMO PARA CREAR UNA PARTICION. DE NO TENER DATOS MINIMOS
    * NO MONTAMOS
    */
    if(strcmp(id1_temp,"") !=0 || strcmp(id2_temp,"") !=0 || strcmp(id3_temp,"") !=0 || strcmp(id4_temp,"") !=0 || strcmp(id5_temp,"") !=0){
        //VIENEN DATOS MINIMOS -> DESMONTAMOS PARTICION
        desmontarParticionesSolicitadas(id1_temp,id2_temp,id3_temp,id4_temp,id5_temp);
    }else{
        //NO HAY DATOS SUFICIENTES PARA MONTAR PARTICION
        printf("Error: Datos insuficientes para desmontar particion.\n");
    }
    //SE HAYA O NO MONTADO LA PARTICION, REGRESAMOS LAS VARIABLES TEMPORALES A SUS VALORES INICIALES PARA NO REPETIR VALORES
    strcpy(id1_temp,"");
    strcpy(id2_temp,"");
    strcpy(id3_temp,"");
    strcpy(id4_temp,"");
    strcpy(id5_temp,"");
}

char * obtenerPATHxID(char id[5]){
    char texto[50]="";
    int i;
    for(i=0; i<6; i++){
        if(strcmp(listaParticionesMontadas[i].idParticionMontada,id)==0){
            strcpy(texto,listaParticionesMontadas[i].pathDisco);
        }
    }
    return texto;
}

char* obtenerNAMExID(char id[5]){
    char texto[16]="";
    int i;
    for(i=0; i<6; i++){
        if(strcmp(listaParticionesMontadas[i].idParticionMontada,id)==0){
            strcpy(texto,listaParticionesMontadas[i].nombreParticion);
        }
    }
    return texto;
}

void escribirMBR(FILE *grafo,char idPartition[5], char pathDisk[60])
{
        FILE *f = fopen(pathDisk,"r+b");
        if(f==NULL){
               printf("ERROR! No fue posible encontrar el disco para crear reporte MBR.\n");
               return;
        }else{
            rewind(f);//Posicionamos el indicador de posicion del archivo al inicio del mismo
            structMBR mbr;
            fread(&mbr,(int)sizeof(mbr),1,f);

            fprintf(grafo,"mbr[\nlabel=<\n<TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"0\">\n");
            fprintf(grafo,"<TR>\n<TD COLSPAN=\"48\" BGCOLOR=\"darkkhaki\">MASTER BOOT RECORD</TD></TR>\n\n");

            fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">ATRIBUTO:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">VALOR:</TD></TR>\n\n");
            fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">mbr_tama?o:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%d</TD></TR>\n\n",mbr.mbr_tamano);
            fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">mbr_fecha:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%s</TD></TR>\n\n",mbr.mbr_fecha_creacion);
            fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">mbr_Id_Disk:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%d</TD></TR>\n\n",mbr.mbr_disk_signature);

            if(strcmp(mbr.mbr_partition_1.part_status,"1")==0){
                fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_status_1:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%s</TD></TR>\n\n",mbr.mbr_partition_1.part_status);
                fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_type_1:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%s</TD></TR>\n\n",mbr.mbr_partition_1.part_type);
                fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_fit_1:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%s</TD></TR>\n\n",mbr.mbr_partition_1.part_fit);
                fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_start_1:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%d</TD></TR>\n\n",mbr.mbr_partition_1.part_start);
                fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_size_1:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%d</TD></TR>\n\n",mbr.mbr_partition_1.part_size);
                fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_name_1:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%s</TD></TR>\n\n",mbr.mbr_partition_1.part_name);
                if(strcmp(mbr.mbr_partition_1.part_type,"e")==0){
                    //Si es EXTENDIDA leemos el EBR pordefault para ver si esta activo o no esta activo
                    int positionFirstEBR;
                    structEBR auxEBR;
                    positionFirstEBR = mbr.mbr_partition_1.part_start;
                    FILE * ff=fopen(pathDisk,"r+b");
                    fseek(ff,positionFirstEBR,SEEK_SET);
                    fread(&auxEBR,sizeof(structEBR),1,ff);
                    fclose(ff);
                    //Si esta activo el primer imprimimos toda la informacion de la particion logica
                    if(strcmp(auxEBR.part_status,"1")==0){
                        fprintf(grafo,"<TR>\n<TD COLSPAN=\"48\" BGCOLOR=\"khaki\">EBR</TD>\n</TR>\n\n");
                        fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_status:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%s</TD></TR>\n\n",auxEBR.part_status);
                        fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_fit:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%s</TD></TR>\n\n",auxEBR.part_fit);
                        fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_start:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%d</TD></TR>\n\n",auxEBR.part_start);
                        fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_size:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%d</TD></TR>\n\n",auxEBR.part_size);
                        fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_next:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%d</TD></TR>\n\n",auxEBR.part_next);
                        fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_name:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%s</TD></TR>\n\n",auxEBR.part_name);

                        //Verificamos si nuestro auxEBR apunta a siguiente EBR en dado caso que no su valor es -1
                        if(auxEBR.part_next!=-1){
                            //Imprimos los demas EBR en caso que existan mientras auxEBR.partNext !=-1
                            while(auxEBR.part_next!=-1){
                                //leeos el siguiente EBR
                                FILE *auxF=fopen(pathDisk,"rw+b");
                                fseek(auxF,auxEBR.part_next,SEEK_SET);
                                structEBR nextEBR;
                                fread(&nextEBR,sizeof(structEBR),1,auxF);
                                fclose(auxF);

                                fprintf(grafo,"<TR>\n<TD COLSPAN=\"48\" BGCOLOR=\"khaki\">EBR</TD>\n</TR>\n\n");
                                fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_status:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%s</TD></TR>\n\n",nextEBR.part_status);
                                fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_fit:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%s</TD></TR>\n\n",nextEBR.part_fit);
                                fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_start:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%d</TD></TR>\n\n",nextEBR.part_start);
                                fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_size:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%d</TD></TR>\n\n",nextEBR.part_size);
                                fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_next:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%d</TD></TR>\n\n",nextEBR.part_next);
                                fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_name:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%s</TD></TR>\n\n",nextEBR.part_name);

                                auxEBR=nextEBR;
                            }//Fin WHILE minetras exista siguiente EBR mandarlas a pintar
                        }//Fin if si existe siguiente EBR

                    }
                }
            }

            if(strcmp(mbr.mbr_partition_2.part_status,"1")==0){
                fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_status_2:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%s</TD></TR>\n\n",mbr.mbr_partition_2.part_status);
                fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_type_2:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%s</TD></TR>\n\n",mbr.mbr_partition_2.part_type);
                fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_fit_2:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%s</TD></TR>\n\n",mbr.mbr_partition_2.part_fit);
                fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_start_2:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%d</TD></TR>\n\n",mbr.mbr_partition_2.part_start);
                fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_size_2:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%d</TD></TR>\n\n",mbr.mbr_partition_2.part_size);
                fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_name_2:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%s</TD></TR>\n\n",mbr.mbr_partition_2.part_name);
                if(strcmp(mbr.mbr_partition_2.part_type,"e")==0){
                    //Si es EXTENDIDA leemos el EBR pordefault para ver si esta activo o no esta activo
                    int positionFirstEBR;
                    structEBR auxEBR;
                    positionFirstEBR = mbr.mbr_partition_2.part_start;
                    FILE * ff=fopen(pathDisk,"r+b");
                    fseek(ff,positionFirstEBR,SEEK_SET);
                    fread(&auxEBR,sizeof(structEBR),1,ff);
                    fclose(ff);
                    //Si esta activo el primer imprimimos toda la informacion de la particion logica
                    if(strcmp(auxEBR.part_status,"1")==0){
                        fprintf(grafo,"<TR>\n<TD COLSPAN=\"48\" BGCOLOR=\"khaki\">EBR</TD>\n</TR>\n\n");
                        fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_status:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%s</TD></TR>\n\n",auxEBR.part_status);
                        fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_fit:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%s</TD></TR>\n\n",auxEBR.part_fit);
                        fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_start:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%d</TD></TR>\n\n",auxEBR.part_start);
                        fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_size:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%d</TD></TR>\n\n",auxEBR.part_size);
                        fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_next:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%d</TD></TR>\n\n",auxEBR.part_next);
                        fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_name:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%s</TD></TR>\n\n",auxEBR.part_name);

                        //Verificamos si nuestro auxEBR apunta a siguiente EBR en dado caso que no su valor es -1
                        if(auxEBR.part_next!=-1){
                            //Imprimos los demas EBR en caso que existan mientras auxEBR.partNext !=-1
                            while(auxEBR.part_next!=-1){
                                //leeos el siguiente EBR
                                FILE *auxF=fopen(pathDisk,"r+b");
                                fseek(auxF,auxEBR.part_next,SEEK_SET);
                                structEBR nextEBR;
                                fread(&nextEBR,sizeof(structEBR),1,auxF);
                                fclose(auxF);

                                fprintf(grafo,"<TR>\n<TD COLSPAN=\"48\" BGCOLOR=\"khaki\">EBR</TD>\n</TR>\n\n");
                                fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_status:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%s</TD></TR>\n\n",nextEBR.part_status);
                                fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_fit:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%s</TD></TR>\n\n",nextEBR.part_fit);
                                fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_start:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%d</TD></TR>\n\n",nextEBR.part_start);
                                fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_size:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%d</TD></TR>\n\n",nextEBR.part_size);
                                fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_next:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%d</TD></TR>\n\n",nextEBR.part_next);
                                fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_name:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%s</TD></TR>\n\n",nextEBR.part_name);

                                auxEBR=nextEBR;
                            }//Fin WHILE minetras exista siguiente EBR mandarlas a pintar
                        }//Fin if si existe siguiente EBR

                    }
                }
            }

            if(strcmp(mbr.mbr_partition_3.part_status,"1")==0){
                fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_status_3:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%s</TD></TR>\n\n",mbr.mbr_partition_3.part_status);
                fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_type_3:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%s</TD></TR>\n\n",mbr.mbr_partition_3.part_type);
                fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_fit_3:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%s</TD></TR>\n\n",mbr.mbr_partition_3.part_fit);
                fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_start_3:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%d</TD></TR>\n\n",mbr.mbr_partition_3.part_start);
                fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_size_3:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%d</TD></TR>\n\n",mbr.mbr_partition_3.part_size);
                fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_name_3:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%s</TD></TR>\n\n",mbr.mbr_partition_3.part_name);
                if(strcmp(mbr.mbr_partition_3.part_type,"e")==0){

                    //Si es EXTENDIDA leemos el EBR pordefault para ver si esta activo o no esta activo
                    int positionFirstEBR;
                    structEBR auxEBR;
                    positionFirstEBR = mbr.mbr_partition_3.part_start;
                    FILE * ff=fopen(pathDisk,"r+b");
                    fseek(ff,positionFirstEBR,SEEK_SET);
                    fread(&auxEBR,sizeof(structEBR),1,ff);
                    fclose(ff);
                    //Si esta activo el primer imprimimos toda la informacion de la particion logica
                    if(strcmp(auxEBR.part_status,"1")==0){
                        fprintf(grafo,"<TR>\n<TD COLSPAN=\"48\" BGCOLOR=\"khaki\">EBR</TD>\n</TR>\n\n");
                        fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_status:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%s</TD></TR>\n\n",auxEBR.part_status);
                        fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_fit:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%s</TD></TR>\n\n",auxEBR.part_fit);
                        fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_start:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%d</TD></TR>\n\n",auxEBR.part_start);
                        fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_size:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%d</TD></TR>\n\n",auxEBR.part_size);
                        fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_next:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%d</TD></TR>\n\n",auxEBR.part_next);
                        fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_name:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%s</TD></TR>\n\n",auxEBR.part_name);

                        //Verificamos si nuestro auxEBR apunta a siguiente EBR en dado caso que no su valor es -1
                        if(auxEBR.part_next!=-1){
                            //Imprimos los demas EBR en caso que existan mientras auxEBR.partNext !=-1
                            while(auxEBR.part_next!=-1){
                                //leeos el siguiente EBR
                                FILE *auxF=fopen(pathDisk,"r+b");
                                fseek(auxF,auxEBR.part_next,SEEK_SET);
                                structEBR nextEBR;
                                fread(&nextEBR,sizeof(structEBR),1,auxF);
                                fclose(auxF);

                                fprintf(grafo,"<TR>\n<TD COLSPAN=\"48\" BGCOLOR=\"khaki\">EBR</TD>\n</TR>\n\n");
                                fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_status:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%s</TD></TR>\n\n",nextEBR.part_status);
                                fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_fit:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%s</TD></TR>\n\n",nextEBR.part_fit);
                                fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_start:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%d</TD></TR>\n\n",nextEBR.part_start);
                                fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_size:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%d</TD></TR>\n\n",nextEBR.part_size);
                                fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_next:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%d</TD></TR>\n\n",nextEBR.part_next);
                                fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_name:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%s</TD></TR>\n\n",nextEBR.part_name);

                                auxEBR=nextEBR;
                            }//Fin WHILE minetras exista siguiente EBR mandarlas a pintar
                        }//Fin if si existe siguiente EBR

                    }
                }
            }

            if(strcmp(mbr.mbr_partition_4.part_status,"1")==0){
                fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_status_4:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%s</TD></TR>\n\n",mbr.mbr_partition_4.part_status);
                fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_type_4:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%s</TD></TR>\n\n",mbr.mbr_partition_4.part_type);
                fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_fit_4:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%s</TD></TR>\n\n",mbr.mbr_partition_4.part_fit);
                fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_start_4:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%d</TD></TR>\n\n",mbr.mbr_partition_4.part_start);
                fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_size_4:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%d</TD></TR>\n\n",mbr.mbr_partition_4.part_size);
                fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_name_4:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%s</TD></TR>\n\n",mbr.mbr_partition_4.part_name);
                if(strcmp(mbr.mbr_partition_4.part_type,"e")==0){

                    //Si es EXTENDIDA leemos el EBR pordefault para ver si esta activo o no esta activo
                    int positionFirstEBR;
                    structEBR auxEBR;
                    positionFirstEBR = mbr.mbr_partition_4.part_start;
                    FILE * ff=fopen(pathDisk,"r+b");
                    fseek(ff,positionFirstEBR,SEEK_SET);
                    fread(&auxEBR,sizeof(structEBR),1,ff);
                    fclose(ff);
                    //Si esta activo el primer imprimimos toda la informacion de la particion logica
                    if(strcmp(auxEBR.part_status,"1")==0){
                        fprintf(grafo,"<TR>\n<TD COLSPAN=\"48\" BGCOLOR=\"khaki\">EBR</TD>\n</TR>\n\n");
                        fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_status:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%s</TD></TR>\n\n",auxEBR.part_status);
                        fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_fit:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%s</TD></TR>\n\n",auxEBR.part_fit);
                        fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_start:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%d</TD></TR>\n\n",auxEBR.part_start);
                        fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_size:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%d</TD></TR>\n\n",auxEBR.part_size);
                        fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_next:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%d</TD></TR>\n\n",auxEBR.part_next);
                        fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_name:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%s</TD></TR>\n\n",auxEBR.part_name);

                        //Verificamos si nuestro auxEBR apunta a siguiente EBR en dado caso que no su valor es -1
                        if(auxEBR.part_next!=-1){
                            //Imprimos los demas EBR en caso que existan mientras auxEBR.partNext !=-1
                            while(auxEBR.part_next!=-1){
                                //leeos el siguiente EBR
                                FILE *auxF=fopen(pathDisk,"r+b");
                                fseek(auxF,auxEBR.part_next,SEEK_SET);
                                structEBR nextEBR;
                                fread(&nextEBR,sizeof(structEBR),1,auxF);
                                fclose(auxF);

                                fprintf(grafo,"<TR>\n<TD COLSPAN=\"48\" BGCOLOR=\"khaki\">EBR</TD>\n</TR>\n\n");
                                fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_status:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%s</TD></TR>\n\n",nextEBR.part_status);
                                fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_fit:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%s</TD></TR>\n\n",nextEBR.part_fit);
                                fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_start:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%d</TD></TR>\n\n",nextEBR.part_start);
                                fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_size:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%d</TD></TR>\n\n",nextEBR.part_size);
                                fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_next:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%d</TD></TR>\n\n",nextEBR.part_next);
                                fprintf(grafo,"<TR>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">part_name:</TD>\n<TD COLSPAN=\"24\" BGCOLOR=\"khaki\">%s</TD></TR>\n\n",nextEBR.part_name);

                                auxEBR=nextEBR;
                            }//Fin WHILE minetras exista siguiente EBR mandarlas a pintar
                        }//Fin if si existe siguiente EBR

                    }
                }
            }

            fprintf(grafo,"</TABLE>\n>];\n\n");
        }
    fclose(f);
}

void graficarMBR(char idPartition[5], char pathDisk[60]){
    FILE* file = fopen("MBR.dot","w+");

    if(file != NULL)
    {
        fprintf(file,"digraph structs\n{\n");
        fprintf(file,"node [shape=plaintext]\n");
        //fprintf(file,"avd0 [label=<<TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"0\"><TR><TD COLSPAN=\"24\" BGCOLOR=\"khaki\" PORT=\"title\">avd0</TD></TR></TABLE>>];");
        escribirMBR(file,idPartition,pathDisk);

        fprintf(file,"}");
        fclose(file);
    }

    system("/usr/bin/dot -Tjpg MBR.dot -o MBR.jpg");
    system("gnome-open MBR.jpg");
}

void evaluarAtributosParaReportes(char * tokenASeparar){
    int cont=0;//cont auxiliar para determinar si estamos leyendo el primer o el segundo valor -Val1=Val2
    char *tipo; //tipo auxiliar usado en lectura segundo valor para determinar que tipo de valor viene segun val1 ledio en primera pasada

    char *cadena;
    while(cadena=strtok_r(tokenASeparar,"::",&tokenASeparar)){
        cont=cont+1;//Cuando entramos a leer el primer valor del tokenASeparar, dejamos evidencia q estamos leyendo el primer valor
        if(cont==1){//Si es primera vez q pasa por aqui el token es el q nos indica el tipo de valor q obtendremos
            //printf("TIPO: %s\n",token);
            tipo=cadena;
        }else if(cont==2){//Ya sabremos que tipo de valor otendremos aqui...xq en el la primera pasada lo determinamos
            //printf("VALOR: %s\n",token);
            if(strcmp(tipo,"-name")==0){
                strcpy(nameRepTemp,cadena);
            }else if(strcmp(tipo,"-path")==0){
                strcpy(pathRepTemp,cadena);
            }else if(strcmp(tipo,"-id")==0){
                strcpy(idPartRepTemp,cadena);
            }else{
                printf("ERROR: ATRIBUTO DE REP NO IDENTIFICADO\n");
            }//FIN IF si atributo es identificado o no Identificado
        }//FIN IF si es primera o segunda lectura de token
    }//FIN WHILE estamos leyendo los dos valores de la cadena a separar por ":"
}


void escribirDISK(FILE *grafo,char idPartition[5], char pathDisk[60])
{
        FILE *f = fopen(pathDisk,"r+b");
        if(f==NULL){
               printf("ERROR! No fue posible encontrar el disco para crear reporte DISK.\n");
               return;
        }else{
            rewind(f);//Posicionamos el indicador de posicion del archivo al inicio del mismo
            structMBR mbr;
            fread(&mbr,(int)sizeof(mbr),1,f);

            fprintf(grafo,"disco[\nlabel=<\n<TABLE WIDTH=\"80%\" BORDER=\"1\" CELLBORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"8\">\n<TR>\n");
            fprintf(grafo,"<TD ROWSPAN=\"2\" COLSPAN=\"10\" BGCOLOR=\"WHIE\">MBR</TD>\n");
            if(strcmp(mbr.mbr_partition_1.part_status,"1")==0){
                    if(strcmp(mbr.mbr_partition_1.part_type,"p")==0){//SI ES PRIMARIA
                        fprintf(grafo,"<TD COLSPAN=\"20\" BGCOLOR=\"WHITE\"><B>PRIMARIA</B><BR/>%s<BR/>Size=%dkb</TD>");
                    }else{//SI ES EXTENDIDA
                        char cadena[25]="EBR - ";
                        //Leemos primer EBR
                        FILE *f=fopen(pathDisk,"rw+b");
                        structEBR ebraux;
                        fseek(f,mbr.mbr_partition_1.part_start,SEEK_SET);
                        fread(&ebraux,sizeof(structEBR),1,f);
                        fclose(f);
                        //Vefificamos si esta activa para concatenar logica
                        if(strcmp(ebraux.part_status,"1")==0){
                            strcat(cadena,"LOGICA - ");
                        }
                        if(ebraux.part_next!=-1){
                            strcat(cadena,"EBR - LOGICA");
                        }

                        fprintf(grafo,"<TD COLSPAN=\"20\" BGCOLOR=\"WHITE\"><B>EXTENDIDA</B><BR/>%s </TD>",cadena);
                    }
            }else{//Esta Libre
                fprintf(grafo,"<TD COLSPAN=\"20\" BGCOLOR=\"WHITE\"><B>LIBRE</B></TD>");
            }

            fprintf(grafo,"</TR>\n</TABLE>\n>];\n\n");
        }
    fclose(f);
}




void graficarDISK(char idPartition[5], char pathDisk[60]){
    FILE* file = fopen("DISK.dot","w+");

    if(file != NULL)
    {
        fprintf(file,"digraph structs\n{\n");
        fprintf(file,"labelloc=\"t\";\n{\n");
        fprintf(file,"label=\"REPOTE DISCO\"\n{\n");
        fprintf(file,"node [shape=plaintext]\n");
        //fprintf(file,"avd0 [label=<<TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"0\"><TR><TD COLSPAN=\"24\" BGCOLOR=\"khaki\" PORT=\"title\">avd0</TD></TR></TABLE>>];");
        escribirDISK(file,idPartition,pathDisk);

        fprintf(file,"}");
        fclose(file);
    }

    system("/usr/bin/dot -Tjpg DISK.dot -o DISK.jpg");
    system("gnome-open DISK.jpg");
}

void generarReporte(char * listaAtributosPorEspacio){
    //En la lista tenemos los tokenes de tipo -TIPO=VALOR
    char *token;
    for (token = listaAtributosPorEspacio; token != NULL; token = strtok(NULL, " "))
    {
      evaluarAtributosParaReportes(token);//Mandamos el token, para que sea separado por ":" y guardar valor segun su tipo
    }

    /*DESPUES DE LEER TODOS LOS ATRIBUTOS Y TENERLOS EN VARIABLES TERMPORALES
    *VERIFICAMOS SI HAY SUFICIENTES DATOS COMO PARA CREAR UN DISCO NUEVO. DE NO TENER DATOS MINIMOS
    * NO CREAMOS EL DISCO Y MOSTRAMOS MENSAJE DE ERROR QUE NO SE ENCONTRARON SUFIICIENTES DATOS
    */
    if((strcmp(idPartRepTemp,"")!=0) && (strcmp(nameRepTemp,"\"mbr\"")==0)){
        //VIENEN DATOS MINIMOS -> CREAMOS REPORTE
        char pathDiskOfID[60];
        strcpy(pathDiskOfID,obtenerPATHxID(idPartRepTemp));

        char pathDiskClean[60];
        int j=0; //index of new array
        int i;
        for(i=0; i<60; i++){//Recorremos el array del la ruta de directorios y formamos nuevo array sin el caracter de comillas
            if(pathDiskOfID[i]=='"'){
            //Nada q hacer, Ignoramos el caracter
            }else{
                pathDiskClean[j]=pathDiskOfID[i];//Cualquier otro caracter lo compiamos al nuevo array
                j=j+1;//y aumentamos el indice del nuevo array para estar listo en recibir otro caracter
            }
        }
        graficarMBR(idPartRepTemp,pathDiskClean);
    }else if((strcmp(idPartRepTemp,"")!=0) && (strcmp(nameRepTemp,"\"disk\"")==0)){
        //VIENEN DATOS MINIMOS -> CREAMOS REPORTE
        char pathDiskOfID[60];
        strcpy(pathDiskOfID,obtenerPATHxID(idPartRepTemp));

        char pathDiskClean[60];
        int j=0; //index of new array
        int i;
        for(i=0; i<60; i++){//Recorremos el array del la ruta de directorios y formamos nuevo array sin el caracter de comillas
            if(pathDiskOfID[i]=='"'){
            //Nada q hacer, Ignoramos el caracter
            }else{
                pathDiskClean[j]=pathDiskOfID[i];//Cualquier otro caracter lo compiamos al nuevo array
                j=j+1;//y aumentamos el indice del nuevo array para estar listo en recibir otro caracter
            }
        }
        graficarDISK(idPartRepTemp,pathDiskClean);
    }else{
        //NO HAY DATOS SUFICIENTES PARA CREAR REPORTE
        printf("Error: Datos insuficientes para crear reporte.\n");

    }
    //SE HAYA O NO CRADO EL REPORTE, REGRESAMOS LAS VARIABLES TEMPORALES A SUS VALORES INICIALES PARA NO REPETIR VALORES
    char nameRepTemp[8];
    char pathRepTemp[50];
}

void leerComando(){
    char lineaLeida[115];   //Variable del metodo, que guarda toda la lienea de comando leida
    printf("-> ");
    scanf("%[^\n]",lineaLeida); //Read the entire line
    getchar();
    int z;
    //FOR QUE CONVIERTE TODA LA CADENA EN MINUSCULAS
    for(z=0;z<115;z++)
    {
        lineaLeida[z]=tolower(lineaLeida[z]);
    }

    if(strcmp (lineaLeida,"exit") == 0){//SI la linea leida es exit, cerramos aplicacion
        printf("Saliendo del sistema...\n");
    }else{
        printf("Identificando Instruccion...");
        char *listaAtributosSeparadosPorEspacio = strtok(lineaLeida," ");//Nos devuelve una lista de palabras separadas por espacio en nuestro apuntador char*
        if(strcmp(listaAtributosSeparadosPorEspacio,"mkdisk")==0){//Si el primer token es MKDISK
            printf("CREACION DE DISCO.\n");
            listaAtributosSeparadosPorEspacio=strtok(NULL," ");//Mandamos a llamar el siguiente token de la lista, el cual seria el atrubuto -TIPO=VALOR
            crearDisco(listaAtributosSeparadosPorEspacio);//Y mandamos el resto de la lista separados por espacio, para leer todos los comandos necesarios para crear disco

        }else if(strcmp(listaAtributosSeparadosPorEspacio,"rmdisk")==0){
            printf("ELIMINACION DE DISCO.\n");
            char respuesta[2];
            printf("AVISO!!! Esta seguro que desea eliminar el disco? (y/n)\n");
            scanf("%s",respuesta); //Read the entire line
            getchar();

            if(strcmp(respuesta,"y")==0){
                listaAtributosSeparadosPorEspacio=strtok(NULL," ");//llamamos la siguiente cadena que seria el path del disco a eliminar
                eliminarDisco(listaAtributosSeparadosPorEspacio);
            }
        }else if(strcmp(listaAtributosSeparadosPorEspacio,"fdisk")==0){
            printf("ADMINISTACION DE PARTICIONES.\n");
            listaAtributosSeparadosPorEspacio=strtok(NULL," ");
            administrarParticion(listaAtributosSeparadosPorEspacio);

        }else if(strcmp(listaAtributosSeparadosPorEspacio,"mount")==0){
            printf("MONTAR DISCO Y PARTICION\n");
            listaAtributosSeparadosPorEspacio=strtok(NULL," ");
            montarParticion(listaAtributosSeparadosPorEspacio);

        }else if(strcmp(listaAtributosSeparadosPorEspacio,"umount")==0){
            printf("DESMONTAR PARTICION\n");
            listaAtributosSeparadosPorEspacio=strtok(NULL," ");
            desMontarParticion(listaAtributosSeparadosPorEspacio);

        }else if(strcmp(listaAtributosSeparadosPorEspacio,"rep")==0){
            printf("GENERANDO REPORTE...\n");
            listaAtributosSeparadosPorEspacio=strtok(NULL," ");
            generarReporte(listaAtributosSeparadosPorEspacio);
        }else{
            printf("ERROR: INSTRUCCION %s NO IDENTIFICADA\n",listaAtributosSeparadosPorEspacio);
        }//Fin if identificacion de comando
    leerComando();//AFTER EXECUTE THE INSTRUCTION, READ CONSOLE ONE MORE TIME
    }//Fin de IF si no es EXIT
}

int main()
{
    printf("*****************************************************************************\n");
    printf("******************************* PROYECTO ************************************\n");
    printf("********************  SISTEMA DE ARCHIVOS EXT2/EXT3  ************************\n");
    printf("*****************************************************************************\n\n");

    leerComando();

    return 0;
}
