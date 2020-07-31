#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <err.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>


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
	char size[12];
	char mtime[12];
	char chksum[8];
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

void writeToFile(long file_size, char *buffer, int position, int blockSize, char *name){
	FILE * fp;
	fp = fopen (name, "w");
	for (int i = 0; i < file_size; i++){

		if (position + blockSize + i >= tarSize){
			fflush(stdout);
			fprintf(stderr, "mytar: Unexpected EOF in archive\n");
			break;
		}

		fputc(buffer[position + blockSize + i], fp);
	}
	fclose(fp);
}

void searchForBlocks(int position, int fileOffset, int blockSize, char *buffer, int index, int argc, char **searchNames){
	int zero1Position = position + fileOffset;
	int zero2Position = zero1Position + blockSize;

	// First Zero block check
	int zero1Chksum = calculateCheckSum(blockSize, zero1Position, buffer);
	int zero2Chksum = calculateCheckSum(blockSize, zero2Position, buffer);

	if (zero1Chksum == blockSize || tarSize - zero1Position == blockSize)
		printf("mytar: A lone zero block at %d\n", 1 + zero1Position/blockSize);

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
	if (found)
		errx(2, "Exiting with failure status due to previous errors");
}

void printTarFiles(char *buffer, char** searchNames, int index, int argc, bool verbose, bool extract, bool tOptionFound){
	bool fileToSearch = (argc != index);
	const int blockSize=512;
	int fileOffset = 0;
	int position = 0;
	if (extract || tOptionFound){
		do{
			position += fileOffset;
			tarHeader* header;
			header = (tarHeader*)(buffer+position);
			// Since size is in octal, we need to convert to decimal
			char *ptr;
			long file_size;
			file_size = strtoul(header->size, &ptr, 8);

			fileOffset = (1 + file_size/blockSize + (file_size % blockSize != 0)) * blockSize;

			if (strcmp(header->chksum, "") == 0)
				break;
			if (header->typeflag != '0' && header->typeflag!='\x00')
				errx(2, "%s%d", "Unsupported header type: ", header->typeflag);

			if (!fileToSearch){
				if (verbose || tOptionFound)
					printf("%s\n", header->name);
				if (extract){
					writeToFile(file_size, buffer, position, blockSize, header->name);
				}
			}
			else{
				int c = index;
				while (c < argc){
					if (strcmp(header->name, searchNames[c]) == 0){
						if (verbose || tOptionFound){
							printf("%s\n", header->name);
							fflush(stdout);
						}
						if (extract)
							writeToFile(file_size, buffer, position, blockSize, header->name);
						searchNames[c] = " ";
						break;
					}
					c++;
				}
			}
			if (position + fileOffset + 2 * blockSize >= tarSize)
				break;
		} while(position + fileOffset + blockSize + 2 * blockSize <= tarSize);
	}
	searchForBlocks(position, fileOffset, blockSize, buffer, index, argc, searchNames);
}

char* openTarFile(char *file){
	FILE *tarFile;
	tarFile = fopen(file , "rb" );
	if (tarFile == NULL)
		errx(2, "%s '%s'","Error opening archive: Failed to open", file);
	fseek(tarFile, 0, SEEK_END);
	tarSize = ftell(tarFile);
	fseek(tarFile, 0, SEEK_SET);
	char *buffer;
	buffer = (char*) malloc(tarSize);
	//printf("tarsize = %ld and size_t tarsize = %ld\n", tarSize, (size_t)tarSize);
	if (fread(buffer,sizeof(char),tarSize,tarFile) != (size_t)tarSize)
		errx(2, "Unexpected EOF in archive\nmytar: Error is not recoverable: exiting now");
	tarHeader* header;
	header = (tarHeader*)(buffer);
	if (strcmp(header->magic, "ustar  ") != 0)
		errx(2, "This does not look like a tar archive\nmytar: Exiting with failure status due to previous errors");
	return buffer;
}

int main(int argc, char *argv[])
{
	bool fOptionFound = false, fileNameFound = false, tBeforeF = false;
	bool xBeforeF = false;
	bool tOptionFound = false, optionFound = false;
	bool verbose = false, extract = false, vAfterX = false;
	char *buffer;
	if (argc == 1)
		errx(2, "need at least one option");

	for (int i = 1; i < argc+1; i++){
		if (!fileNameFound && fOptionFound){
			buffer = openTarFile(argv[i]);
			fileNameFound = true;
		}

		if ((tOptionFound || extract) && fileNameFound){
			int ind = (tBeforeF || xBeforeF || vAfterX)? i+1 : i;
			printTarFiles(buffer, argv, ind, argc, verbose, extract, tOptionFound);
			break;
		}
		if (tOptionFound && !fOptionFound)
		    tBeforeF = true;

		if (extract && !fOptionFound)
			xBeforeF = true;

		if (i < argc){
			optionFound = (argv[i][0] == '-');
			if (optionFound){
				char currChar = argv[i][1];
				if (!fOptionFound) fOptionFound = (currChar == 'f');
				if (!tOptionFound) tOptionFound = (currChar == 't');
				if (!verbose) verbose = (currChar == 'v');
				if (!extract && currChar == 'x'){
					extract = true;
					if (i + 2 <= argc && strcmp(argv[i + 1],"-v") == 0){
						verbose = true;
						vAfterX = true;
					}
				} 

				if (fOptionFound && !fileNameFound && (argc == i+1 || argv[i+1][0] == '-')){
					errx(2, noFileNameErr);
				}

				if (currChar != 't' && currChar != 'f' && currChar != 'x' && currChar != 'v')
					errx(2, "%s: %s", "mytar: Unknown option", argv[i]);
				}
			}
		if (!fOptionFound && !tOptionFound && !verbose && !extract){
			out = 2;
			break;
		}
	}
	return out;
}