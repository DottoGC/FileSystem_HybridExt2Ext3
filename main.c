#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//ATRIBUTOS TEMPORALES PARA mkdisk CREACION DE DISCO (Archivo Fisico)
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
            printf("MONTAR DISCO\n");
            //listaAtributosSeparadosPorEspacio=strtok(NULL," ");
            //montarParticion(listaAtributosSeparadosPorEspacio);

        }else if(strcmp(listaAtributosSeparadosPorEspacio,"unmount")==0){
            printf("desMONTAR DISCO\n");

        }else if(strcmp(listaAtributosSeparadosPorEspacio,"rep")==0){
            printf("GENERANDO REPORTE...\n");
            listaAtributosSeparadosPorEspacio=strtok(NULL," ");
            //generarReporte(listaAtributosSeparadosPorEspacio);
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
