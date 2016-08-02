
void crearDiscoFisico(int sizeDiscTemp,char pathDiscTemp[50],char nameDiscTemp[10] ,char unitDiscTemp[2]){
    printf("Size %d%s \n",sizeDiscTemp,unitDiscTemp);
    strcat(pathDiscTemp,nameDiscTemp);
    printf("Path %s \n",pathDiscTemp);
    printf("char: %c \n",pathDiscTemp[0]);
}
