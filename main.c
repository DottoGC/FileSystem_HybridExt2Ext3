#include <stdio.h>
#include <stdlib.h>

void leerComando(){
    char lineaLeida[100];   //Variable del metodo, que guarda toda la lienea de comando leida
    printf("-> ");
    scanf("%[^\n]",lineaLeida); //Read the entire line
    getchar();

    if(strcmp (lineaLeida,"exit") == 0){//SI la linea leida es exit, cerramos aplicacion
        printf("Saliendo del sistema...\n");
    }else{
        printf("Identificando Instruccion...");
        //char *listaAtributosSeparadosPorEspacio = strtok(lineaLeida," ");//Nos devuelve una lista de palabras separadas por espacio en nuestro apuntador char*
        //printf("%s",listaAtributosSeparadosPorEspacio);
        printf("asdf;lkj");

        /*
        if(strcmp(listaAtributosSeparadosPorEspacio,"mkdisk")==0){//Si el primer token es MKDISK
            printf("CREACION DE DISCO\n");
            //listaAtributosSeparadosPorEspacio=strtok(NULL," ");//Mandamos a llamar el siguiente token de la lista, el cual seria el atrubuto -TIPO=VALOR
            //crearDisco(listaAtributosSeparadosPorEspacio);//Y mandamos el resto de la lista separados por espacio, para leer todos los comandos necesarios para crear disco

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
    */
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
