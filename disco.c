#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>

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

//VARIABLES TEMPORALES PARA LA CREACION DE DISCO
FILE *auxArchivo; //FILE aux para crear del disco como un archivo.
char buffer[1];//Char de un caracter, igual a 1byte,
char fecha[16];

//VARIABLES TEMPORALES PARA LA CREAACION DE PARTICIONES



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
        //1. VERIFICAR SI EXISTE UNA PARTICION EXTENDIDA, SI EXISTE OBTENER POSICION DE INICIO, SI NO MOSTRAR MENSAJE DE ERROR
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
                }else{//Si ya se encuentra activa, leer la posicion del siguiente extended boot record y repeter verifiacion si ya esta acriva, en caso es -1 esta existe siguiente ebr alojarla como la siguiente
                    if(ebrAux.part_next==-1){//si no existe siguiente ebr, entonces creamos el segundo de la lista
                        structEBR newEBR;
                        strcpy(newEBR.part_fit,fitPartTemp);
                        strcpy(newEBR.part_name,namePartTemp);
                        strcpy(newEBR.part_status,"E");
                        newEBR.part_size=tamanoEnBytes(sizePartTemp,unitPartTemp);
                        newEBR.part_next=-1;
                        newEBR.part_start=posExtendida+(int)sizeof(newEBR)+ebrAux.part_size+(int)sizeof(newEBR);
                        int siguiete=posExtendida+(int)sizeof(newEBR)+ebrAux.part_size;
                        fseek(f1,siguiete,SEEK_SET);
                        fwrite(&newEBR,sizeof(structEBR),1,f1);
                        fclose(f1);

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




