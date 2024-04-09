#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>

unsigned char FAT_TABLES = 0; //Кол-во табл fat
short BYTE_IN_SECTOR = 0; //Байт в секторе
unsigned char CLASTER_SIZE = 0;// размер кластера в секторах
short RESERVED_SECTORS = 0; // Секторов зарезервированно
short NUMB_OF_FILE_IN_CORN_DIR = 0; // Файлов в корневой дир
unsigned char FAT_SECTORS = 0; // Сколько занимает fat table в секторах

int offset_final(short klaster)
{
	int offset = ((RESERVED_SECTORS*512) + (FAT_SECTORS*FAT_TABLES*512) + (32*NUMB_OF_FILE_IN_CORN_DIR)) + ((klaster-2)*CLASTER_SIZE*BYTE_IN_SECTOR);
	return offset;
}

int main(int argc, char* argv[])
{
	if(argc != 4){
		printf("Arguments error!!!\n1. Имя образа 2. Имя искомого файла 3. Куда записать результат!!!\n");
		return 1;
	}

	char* NEED_TO_FIND = argv[2];
	for(int i = 0; i < strlen(NEED_TO_FIND); i++){
		NEED_TO_FIND[i] = toupper(NEED_TO_FIND[i]);
	}

	printf("PROGRAMM START\n\n");
	FILE* file = fopen(argv[1], "r");

	// Вынимаем из смещений значения //
	fseek(file, 0xb, SEEK_SET);
	fread(&BYTE_IN_SECTOR, 1, 2, file);
	fread(&CLASTER_SIZE, 1, 1, file);
	fread(&RESERVED_SECTORS, 1, 2, file);
	fread(&FAT_TABLES, 1, 1, file);
	fread(&NUMB_OF_FILE_IN_CORN_DIR, 1, 2, file);
	
	fseek(file, 0x16, SEEK_SET);
	fread(&FAT_SECTORS, 1, 2, file);
	fseek(file, 0, SEEK_SET);

	short tmp = RESERVED_SECTORS * BYTE_IN_SECTOR;
	short FIRST_FAT_ADRESS = tmp;
	printf("Всего FAT таблиц: %d\n", FAT_TABLES);
	for(int i = 0; i < FAT_TABLES; i++){
		printf(" #%d FAT по адр: 0x%04x\n", i, tmp);
		tmp = tmp + (FAT_SECTORS * BYTE_IN_SECTOR);
	}
	int NUMB_OF_FILE_IN_CORN_DIR_IN_BYTES = NUMB_OF_FILE_IN_CORN_DIR * 32; // Размер рута в байтах
//	printf("В руте %d байта\n", NUMB_OF_FILE_IN_CORN_DIR_IN_BYTES);
	short root_adr = tmp;
//	printf(" root po adr: 0x%04x\n", root_adr);
	short data_adr = root_adr + NUMB_OF_FILE_IN_CORN_DIR_IN_BYTES;
//	printf("Данные начинаются по адресу: 0x%04x\n", data_adr);

	char data_from_root[32];
	char file_found[32];
	int pointer;
	fseek(file, root_adr, SEEK_SET);
	for(int i = 0; i < NUMB_OF_FILE_IN_CORN_DIR_IN_BYTES; i+=32){
		fread(&data_from_root, 1, 32, file);
		if (strstr(data_from_root, NEED_TO_FIND) != NULL){
			strncpy(file_found, data_from_root, 32);
//			printf("Данные о файле из FAT:\n%s\n", file_found);
			pointer = i;
		}
	}
	short need_file_adr = root_adr + pointer;
	printf("\nFILE %s adress:0x%04x\n", argv[2], need_file_adr);

	fseek(file, (need_file_adr + 0x1a), SEEK_SET);
	short claster_in_fat = 0;
	fread(&claster_in_fat, 1, 2, file);
	unsigned int need_file_size = 0;
	fread(&need_file_size, 1, 4, file);
//	printf("Размер %s:|%d| bytes\n", argv[2], need_file_size);

	char need_string[BYTE_IN_SECTOR * CLASTER_SIZE];
	FILE* file2 = fopen(argv[3], "w");

	fseek(file, offset_final(claster_in_fat), SEEK_SET);
	fread(&need_string, 1, (BYTE_IN_SECTOR * CLASTER_SIZE), file);
	fputs(need_string, file2);
	while(claster_in_fat != 0x00ff){
		short offset_fat = ((claster_in_fat * 1.5) + FIRST_FAT_ADRESS);
		fseek(file, offset_fat, SEEK_SET);
		fread(&claster_in_fat, 1, 1.5, file);

		fseek(file, offset_final(claster_in_fat), SEEK_SET);
		fread(&need_string, 1, (BYTE_IN_SECTOR * CLASTER_SIZE), file);
		fputs(need_string, file2);
		if(claster_in_fat == 0x00ff){
			break;
		}
		fseek(file, offset_fat + 1, SEEK_SET);
		fread(&claster_in_fat, 1, 2, file);
		claster_in_fat = (int)claster_in_fat / 16;
		fseek(file, offset_final(claster_in_fat), SEEK_SET);
		fread(&need_string, 1, (BYTE_IN_SECTOR * CLASTER_SIZE), file);
		fputs(need_string, file2);

	}
	fclose(file2);

	FILE* file3 = fopen(argv[3], "r");
	char rezult_string[need_file_size];
	fgets(rezult_string, need_file_size + 1, file2);
	fclose(file3);
	FILE* file4 = fopen(argv[3], "w");
	fputs(rezult_string, file4);
	printf("\nDONE!\nРезультат помещен в файл %s\n", argv[3]);

	fclose(file);
	fclose(file4);
}
