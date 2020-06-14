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

typedef struct ustarHeaderBlock {
	char name[100];
	char mode[8];
	char uid[8];
	char gid[8];
	//char size[12];
	//char mtime[12];
	//char chksum[8];
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
} tarHeader;

int calculateCheckSum(int blockSize, int pos, char *buffer){
	int chksum = 0;
	for (int i = 0; i<=blockSize; i++){
		if (buffer[pos + i] == '\x00'){
			chksum+=1;
		}
	}
	return chksum;
}

void printTarFiles(char *buffer, char** searchNames, int index, int argc){
	bool fileToSearch = (argc != index);
	const int blockSize=512;
	int fileOffset = 0;
	int position = 0; //current
	char *size = malloc(12);
	char *mtime = malloc(12);
	char *chksum = malloc(8);
	do{
		position += fileOffset;

		tarHeader* header;
		header = (tarHeader*)(buffer+position);

		strncpy(size, header->s_m_c, 12);
		strncpy(mtime, header->s_m_c+12, 12);
		strncpy(chksum, header->s_m_c+24, 8);
		if (header->typeflag != '0' && header->typeflag!='\x00'){
			errx(2, "%s%d", "Unsupported header type: ", header->typeflag);
		}
		// Since size is in octal, we need to convert to decimal
		char *ptr;
		long file_size;
		file_size = strtoul(size, &ptr, 8);

		// Offset position of the file based on size of file
		fileOffset = (1 + file_size/blockSize) * blockSize;

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

		if (position + fileOffset + 2 * blockSize >= tarSize)
			break;

	} while(position + fileOffset + blockSize + 2 * blockSize <= tarSize);

	int zero1Position = position + fileOffset;
	int zero2Position = zero1Position + blockSize;

	// First Zero block check
	int zero1Chksum = calculateCheckSum(blockSize, zero1Position, buffer);

	int zero2Chksum = calculateCheckSum(blockSize, zero2Position, buffer);

	if (zero1Chksum == blockSize || tarSize - zero1Position == blockSize){
		printf("mytar: A lone zero block at %d\n", 1 + zero1Position/blockSize);
	}

		bool found = false;

		for (int i = index; i < argc; i++){
			if (strcmp(searchNames[i], " ") != 0){
				fprintf(stderr, "%s: %s: %s\n", "mytar", searchNames[i], "Not found in archive");
				fflush(stdout);
				found = true;
			}
		}

		if (tarSize-zero1Position < 0 && zero2Chksum != blockSize && zero1Chksum != blockSize) {
			fflush(stdout);
			fprintf(stderr, "%s", "mytar: Unexpected EOF in archive\n");
			errx(2, "Error is not recoverable: exiting now");
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
	fseek(tarFile, 0, SEEK_SET);

	char *buffer;
	buffer = (char*) malloc(tarSize);
	tarRead = fread(buffer,sizeof(char),tarSize,tarFile);
	return buffer;
}

int main(int argc, char *argv[])
{
	bool fOptionFound = false, fileNameFound = false, tBeforeF = false;
	bool tOptionFound = false, optionFound = false;
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
			printTarFiles(buffer, argv, ind, argc);
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
				if (fOptionFound && !fileNameFound && (argc == i+1 || argv[i+1][0] == '-')){
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
