#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//ATRIBUTOS TEMPORALES PARA LA CREACION DE DISCO(Archivo Fisico)
int sizeDiscTemp;
char pathDiscTemp[50];
char nameDiscTemp[10];
char unitDiscTemp[2]="m";

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
            }else if(strcmp(tipo,"+unit")==0){
                strcpy(unitDiscTemp,cadena);
            }else if(strcmp(tipo,"-path")==0){
                strcpy(pathDiscTemp,cadena); //strcpy(path,pathDiscTemp);
            }else if(strcmp(tipo,"-name")==0){
                strcpy(nameDiscTemp,cadena);
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
        if((sizeDiscTemp!=NULL) && (strcmp(pathDiscTemp,"")!=0)&& (strcmp(nameDiscTemp,"")!=0)){
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

void leerComando(){
    char lineaLeida[100];   //Variable del metodo, que guarda toda la lienea de comando leida
    printf("-> ");
    scanf("%[^\n]",lineaLeida); //Read the entire line
    getchar();
    int z;
    //FOR QUE CONVIERTE TODA LA CADENA EN MINUSCULAS
    for(z=0;z<100;z++)
    {
        lineaLeida[z]=tolower(lineaLeida[z]);
    }

    if(strcmp (lineaLeida,"exit") == 0){//SI la linea leida es exit, cerramos aplicacion
        printf("Saliendo del sistema...\n");
    }else{
        printf("Identificando Instruccion...");
        char *listaAtributosSeparadosPorEspacio = strtok(lineaLeida," ");//Nos devuelve una lista de palabras separadas por espacio en nuestro apuntador char*
        if(strcmp(listaAtributosSeparadosPorEspacio,"mkdisk")==0){//Si el primer token es MKDISK
            printf("CREACION DE DISCO\n");
            listaAtributosSeparadosPorEspacio=strtok(NULL," ");//Mandamos a llamar el siguiente token de la lista, el cual seria el atrubuto -TIPO=VALOR
            crearDisco(listaAtributosSeparadosPorEspacio);//Y mandamos el resto de la lista separados por espacio, para leer todos los comandos necesarios para crear disco

        }else if(strcmp(listaAtributosSeparadosPorEspacio,"rmdisk")==0){
            printf("ELIMINACION DISCO\n");
            //printf("AVISO!!! Esta seguro que desea eliminar el disco? (y/n)\n");
            //char respuesta[2];
            //scanf("%s",respuesta); //Read the entire line
            //getchar();
            //if(strcmp (respuesta,"y") == 0){
            //    listaAtributosSeparadosPorEspacio=strtok(NULL," ");
            //    eliminarDisco(listaAtributosSeparadosPorEspacio);
            //}
        }else if(strcmp(listaAtributosSeparadosPorEspacio,"fdisk")==0){
            printf("ADMINISTACION DE PARTICIONES.\n");
            //listaAtributosSeparadosPorEspacio=strtok(NULL," ");
            //administrarParticion(listaAtributosSeparadosPorEspacio);

        }else if(strcmp(listaAtributosSeparadosPorEspacio,"mount")==0){
            printf("MONTAR DISCO\n");
            //listaAtributosSeparadosPorEspacio=strtok(NULL," ");
            //montarParticion(listaAtributosSeparadosPorEspacio);

        }else if(strcmp(listaAtributosSeparadosPorEspacio,"unmount")==0){
            printf("desMONTAR DISCO\n");

        }else if(strcmp(listaAtributosSeparadosPorEspacio,"rep")==0){
            printf("GENERANDO REPORTE...\n");
            //listaAtributosSeparadosPorEspacio=strtok(NULL," ");
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
