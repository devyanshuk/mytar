#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <err.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>


typedef int bool;
enum { false, true };

long tarSize = 0;
int out = 0;

#define noFileNameErr "Option -f requires an argument"
#define badTarFileName "mytar: Error opening archive: Failed to open"

typedef struct ustar_header_block {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char s_m_c[32];
    char typeflag;
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
} tar_header;


void print_tarFiles(char *buffer, char** searchNames, int index, int argc){
    bool fileToSearch = (argc != index);
	const int block_size=512;
	int file_offset = 0;
	int position = 0; //current
	do{
    	char *size = malloc(12);
    	char *mtime = malloc(12);
        char *chksum = malloc(8);
		position += file_offset;

        tar_header* header;
        header = (tar_header*)(buffer+position);

        strncpy(size, header->s_m_c, 12);
        strncpy(mtime, header->s_m_c+12, 12);
        strncpy(chksum, header->s_m_c+24, 8);
        if (header->typeflag != '0' && header->typeflag!='\x00'){
            errx(2, "%s%c", "Unsupported header type: ", header->typeflag);
        }
		// Since size is in octal, we need to convert to decimal
		char *ptr;
		long file_size;
		file_size = strtoul(size, &ptr, 8);

		// Offset position of the file based on size of file
		file_offset = (1 + file_size/block_size) * block_size;

        if (!fileToSearch)
    		printf("%s\n", header->name);
        else{
            int c = index;
            while (c < argc){
                if (strcmp(header->name, searchNames[c]) == 0){
                    printf("%s\n", header->name);
                    fflush(stdout);
                    searchNames[c] = " ";
                    break;
                }
                c++;
            }

        }
        if (position + file_offset + 2 * block_size >= tarSize)
            break;
	} while(position + file_offset + block_size + 2 * block_size <= tarSize);
	int zero1_position = position + file_offset;
	int zero2_position = zero1_position + block_size;
// First Zero block check
	int zero1_chksum = 0;
	for (int i = 0; i<=block_size; i++){
		if (buffer[zero1_position + i] == '\x00'){
			zero1_chksum+=1;
		}
	}

	int zero2_chksum = 0;
	for (int i = 0; i <= block_size; i++){
		if (buffer[zero2_position + i] == '\x00'){
			zero2_chksum+=1;
		}
	}

    //printf("01 = %d 02 = %d tarsize = %ld\n", zero1_position, zero2_position, tarSize);

    if (zero1_chksum == block_size || tarSize - zero1_position == block_size){
        printf("mytar: A lone zero block at %d\n", 1 + zero1_position/block_size);
    }

    if (tarSize-zero1_position < 0 && zero2_chksum != block_size && zero1_chksum != block_size) {
        fflush(stdout);
        fprintf(stderr, "%s", "mytar: Unexpected EOF in archive\n");
        errx(2, "Error is not recoverable: exiting now");
    }

    bool found = false;
    for (int i = index; i < argc; i++){
        if (strcmp(searchNames[i], " ") != 0){
            fprintf(stderr, "%s: %s: %s\n", "mytar", searchNames[i], "Not found in archive");
            fflush(stdout);
            found = true;
        }
    }
    if (found){
        errx(2, "Exiting with failure status due to previous errors");
    }
}



char* openTarFile(char *file){
	FILE *tarFile;
	tarFile = fopen(file , "rb" );
	if (!tarFile)
		errx(2, "no file");

	long tarRead;

	fseek(tarFile, 0, SEEK_END);
	tarSize = ftell(tarFile);
    //printf("Tarsize: %ld", tarSize);
	fseek(tarFile, 0, SEEK_SET);


	char *buffer;
	buffer = (char*) malloc(tarSize);
	tarRead = fread(buffer,sizeof(char),tarSize,tarFile);
	return buffer;
}

int main(int argc, char *argv[])
{
	bool fOptionFound = false;
    bool fileNameFound = false;
    bool tBeforeF = false;
	bool tOptionFound = false;
	bool optionFound = false;
	char *buffer;

	if (argc == 1)
		errx(2, "need at least one option");

	for (int i = 1; i < argc+1; i++){
		if (!fileNameFound && fOptionFound){
			buffer = openTarFile(argv[i]);
            fileNameFound = true;
		}

        if (tOptionFound && fileNameFound){
            int ind = (tBeforeF)? i+1 : i;
            print_tarFiles(buffer, argv, ind, argc);
            break;
        }
        if (tOptionFound && !fOptionFound){
            tBeforeF = true;
        }
        if (i < argc){
    		optionFound = (argv[i][0] == '-');
    		if (optionFound){
    			char currChar = argv[i][1];
    			if (!fOptionFound) fOptionFound = (currChar == 'f');
    			if (!tOptionFound) tOptionFound = (currChar == 't');
    			if (fOptionFound && (argc == i+1 || argv[i+1][0] == '-')){
                    printf("%d", i);
                    printf("%c", argv[i][0]);
    				errx(2, noFileNameErr);
                }

    			if (currChar != 't' && currChar != 'f')
    				errx(2, "%s: %s", "mytar: Unknown option", argv[i]);
    		}
        }
        if (!fOptionFound && !tOptionFound){
            out = 2;
            break;
        }
}

	return out;
}
