
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <endian.h>

#define N		 4096	/* size of ring buffer */
#define F		   18	/* upper limit for match_length */
#define THRESHOLD	2   /* encode string into position and length if match_length is greater than this */
static unsigned char *text_buf;	/* ring buffer of size N, with extra F-1 bytes to facilitate string comparison */
#define LZSS_TYPE	unsigned short
#define NIL			N	/* index for root of binary search trees */
struct lzss_buffer {
	unsigned char	text_buf[N + F - 1];
	LZSS_TYPE	lson[N + 1];
	LZSS_TYPE	rson[N + 257];
	LZSS_TYPE	dad[N + 1];
};
static LZSS_TYPE		match_position, match_length;  /* of longest match.  These are set by the InsertNode() procedure. */
static LZSS_TYPE		*lson, *rson, *dad;  /* left & right children & parents -- These constitute binary search trees. */


typedef struct compress_mib_header {
	unsigned char signature[6];
	unsigned short compRate;
	unsigned int compLen;
} COMPRESS_MIB_HEADER_T;

#define handle_error(msg) \
           do { perror(msg); exit(EXIT_FAILURE); } while (0)

int Decode(unsigned char *ucInput, unsigned int inLen, unsigned char *ucOutput)	/* Just the reverse of Encode(). */
{
	
	int  i, j, k, r, c;
	unsigned int  flags;
	unsigned int ulPos=0;
	unsigned int ulExpLen=0;

	if ((text_buf = (unsigned char *)malloc( N + F - 1 )) == 0) {
		//fprintf(stderr, "fail to get mem %s:%d\n", __FUNCTION__, __LINE__);
		return 0;
	}
	
	for (i = 0; i < N - F; i++)
		text_buf[i] = ' ';

        
	r = N - F;
	flags = 0;
	while(1) {
		if (((flags >>= 1) & 256) == 0) {
			c = ucInput[ulPos++];
			if (ulPos>inLen)
				break;
			flags = c | 0xff00;		/* uses higher byte cleverly */
		}							/* to count eight */
		if (flags & 1) {
			c = ucInput[ulPos++];
			if ( ulPos > inLen )
				break;
			ucOutput[ulExpLen++] = c;
			text_buf[r++] = c;
			r &= (N - 1);
		} else {
			i = ucInput[ulPos++];
			if ( ulPos > inLen ) break;
			j = ucInput[ulPos++];
			if ( ulPos > inLen ) break;
			
			i |= ((j & 0xf0) << 4);
			j = (j & 0x0f) + THRESHOLD;
			for (k = 0; k <= j; k++) {
				c = text_buf[(i + k) & (N - 1)];
				ucOutput[ulExpLen++] = c;
				text_buf[r++] = c;
				r &= (N - 1);
			}
		}
	}

	free(text_buf);
	return ulExpLen;

}


int main(int argc, char**argv) {
           unsigned char *addr;
           int fd;
           struct stat sb;
           off_t offset, pa_offset;
           size_t length;
           ssize_t s;
           char* filename = "config.dat";

           COMPRESS_MIB_HEADER_T * header;

           if (argc>2) {
             printf("Wrong number of parameters!");
             exit(1);
           }
           if (argc==2) {
             filename=argv[1];
           }
           
           fd = open(filename, O_RDONLY);
           if (fd == -1)
               handle_error("open");

           if (fstat(fd, &sb) == -1)           /* To obtain file size */
               handle_error("fstat");


           addr = (unsigned char *)mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

           header = (COMPRESS_MIB_HEADER_T*)addr;

           
           //printf("%u\n", be16toh(header->compRate));
           //printf("%u\n", be32toh(header->compLen));
           //printf("%u\n", sb.st_size);
           unsigned char *expFile=NULL;
           expFile=(unsigned char *)calloc(1,be16toh(header->compRate)*be32toh(header->compLen));


           
           unsigned int expandLen = Decode(addr+sizeof(COMPRESS_MIB_HEADER_T), be32toh(header->compLen), expFile);

           //printf("%u\n", expandLen);
           //printf("%.*s\n",100, expFile);
           //fwrite(expFile, 1, expandLen, stdout);

		   printf("[USERNAME]:%s\r\n", &expFile[0xDB]);
		   printf("[PASSWD]:%s\r\n", &expFile[0x100]);
           //flash_read_raw_mib("config.dat");

		   return 0;
}