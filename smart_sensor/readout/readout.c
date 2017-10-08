#include <stdlib.h>
#include <fcntl.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <math.h>

#if defined(__linux__)
#include <termios.h>
#else
#include <windows.h>
#endif




int setCPUtype(char* cpu);
int parse_hex (char * filename, unsigned char * progmem, unsigned char * config);
size_t getlinex(char **lineptr, size_t *n, FILE *stream) ;
void comErr(char *fmt, ...);
void flsprintf(FILE* f, char *fmt, ...);

char * COM = "";
char * PP_VERSION = "0.95";


int verbose = 1,verify = 1,program = 1,sleep_time = 0;
int devid_expected,devid_mask,baudRate,com,flash_size,page_size,chip_family,config_size;

int get_serial_line (char cmd, char * line);


short FIX_MPY(short a, short b);
int fix_fftr(short f[], int m, int inverse);

#define N_WAVE      128    /* full length of Sinewave[] */
#define LOG2_N_WAVE 7      /* log2(N_WAVE) */



//*********************************************************************************//
//*********************************************************************************//
//*********************************************************************************//
//               serial IO interfaces for Linux and windows
//*********************************************************************************//
//*********************************************************************************//
//*********************************************************************************//

#if defined(__linux__)

void initSerialPort()
    {
    baudRate=B115200;
    if (verbose>2)
        printf("Opening: %s at %d\n",COM,baudRate);
    com =  open(COM, O_RDWR | O_NOCTTY | O_NDELAY);
    if (com <0) comErr("Failed to open serial port");

    struct termios opts;
    memset (&opts,0,sizeof (opts));

    fcntl(com, F_SETFL, 0);
    if (tcgetattr(com, &opts)!=0) printf("Err tcgetattr\n");

    cfsetispeed(&opts, baudRate);
    cfsetospeed(&opts, baudRate);
    opts.c_lflag  &=  ~(ICANON | ECHO | ECHOE | ISIG);

    opts.c_cflag |=  (CLOCAL | CREAD);
    opts.c_cflag &=  ~PARENB;
    opts.c_cflag &= ~CSTOPB;
    opts.c_cflag &=  ~CSIZE;
    opts.c_cflag |=  CS8;
    opts.c_oflag &=  ~OPOST;
    opts.c_iflag &=  ~INPCK;
    opts.c_iflag &=  ~ICRNL;		//do NOT translate CR to NL
    opts.c_iflag &=  ~(IXON | IXOFF | IXANY);
    opts.c_cc[ VMIN ] = 0;
    opts.c_cc[ VTIME ] = 1;//0.1 sec
    if (tcsetattr(com, TCSANOW, &opts) != 0)
        {
        perror(COM);
        printf("set attr error");
        abort();
        }
    tcflush(com,TCIOFLUSH); // just in case some crap is the buffers
    }


void putByte(int byte)
    {
    char buf = byte;
    if (verbose>3) flsprintf(stdout,"TX: 0x%02X\n", byte);
    int n = write(com, &buf, 1);
    if (n != 1) comErr("Serial port failed to send a byte, write returned %d\n", n);
    }


void putBytes (unsigned char * data, int len)
    {

    int i;
    for (i=0; i<len; i++)
        putByte(data[i]);
    /*
    	if (verbose>3)
    		flsprintf(stdout,"TXP: %d B\n", len);
    int n = write(com, data, len);
    	if (n != len)
    		comErr("Serial port failed to send %d bytes, write returned %d\n", len,n);
    */
    }

int getByte()
    {
    char buf;
    int n = read(com, &buf, 1);
    if (verbose>3) flsprintf(stdout,n<1?"RX: fail\n":"RX:  0x%02X\n", buf & 0xFF);
    if (n == 1) return buf & 0xFF;
	else return 0;
    }
#else

HANDLE port_handle;

void initSerialPort()
    {

    char mode[40],portname[20];
    COMMTIMEOUTS timeout_sets;
    DCB port_sets;
    strcpy(portname,"\\\\.\\");
    strcat(portname,COM);
    port_handle = CreateFileA(portname,
                              GENERIC_READ|GENERIC_WRITE,
                              0,                          /* no share  */
                              NULL,                       /* no security */
                              OPEN_EXISTING,
                              0,                          /* no threads */
                              NULL);                      /* no templates */
    if(port_handle==INVALID_HANDLE_VALUE)
        {
        printf("unable to open port %s -> %s\n",COM, portname);
        exit(0);
        }
    strcpy (mode,"baud=115200 data=8 parity=n stop=1");
    memset(&port_sets, 0, sizeof(port_sets));  /* clear the new struct  */
    port_sets.DCBlength = sizeof(port_sets);

    if(!BuildCommDCBA(mode, &port_sets))
        {
        printf("dcb settings failed\n");
        CloseHandle(port_handle);
        exit(0);
        }

    if(!SetCommState(port_handle, &port_sets))
        {
        printf("cfg settings failed\n");
        CloseHandle(port_handle);
        exit(0);
        }

    timeout_sets.ReadIntervalTimeout         = 1;
    timeout_sets.ReadTotalTimeoutMultiplier  = 1000;
    timeout_sets.ReadTotalTimeoutConstant    = 1;
    timeout_sets.WriteTotalTimeoutMultiplier = 1000;
    timeout_sets.WriteTotalTimeoutConstant   = 1;

    if(!SetCommTimeouts(port_handle, &timeout_sets))
        {
        printf("timeout settings failed\n");
        CloseHandle(port_handle);
        exit(0);
        }


    }
void putByte(int byte)
    {
    int n;
    if (verbose>3) flsprintf(stdout,"TX: 0x%02X\n", byte);
    WriteFile(port_handle, &byte, 1, (LPDWORD)((void *)&n), NULL);
    if (n != 1) comErr("Serial port failed to send a byte, write returned %d\n", n);
    }

void putBytes (unsigned char * data, int len)
    {
    /*
    int i;
    for (i=0;i<len;i++)
    	putByte(data[i]);
    */
    int n;
    WriteFile(port_handle, data, len, (LPDWORD)((void *)&n), NULL);
    if (n != len) comErr("Serial port failed to send a byte, write returned %d\n", n);
    }



int getByte()
    {
    unsigned char buf[2];
    int n;
    ReadFile(port_handle, buf, 1, (LPDWORD)((void *)&n), NULL);
    if (verbose>3) flsprintf(stdout,n<1?"RX: fail\n":"RX:  0x%02X\n", buf[0] & 0xFF);
    if (n == 1) return buf[0] & 0xFF;
	if (n == 0) return 0;
    }
#endif


//*********************************************************************************//
//*********************************************************************************//
//*********************************************************************************//
//               generic routines
//*********************************************************************************//
//*********************************************************************************//
//*********************************************************************************//

void comErr(char *fmt, ...)
    {
    char buf[ 500 ];
    va_list va;
    va_start(va, fmt);
    vsnprintf(buf, sizeof(buf), fmt, va);
    fprintf(stderr,"%s", buf);
 //   perror(COM);
 //   va_end(va);
 //   abort();
    }

void flsprintf(FILE* f, char *fmt, ...)
    {
    char buf[ 500 ];
    va_list va;
    va_start(va, fmt);
    vsnprintf(buf, sizeof(buf), fmt, va);
    fprintf(f,"%s", buf);
    fflush(f);
    va_end(va);
    }


//get line replacement
size_t getlinex(char **lineptr, size_t *n, FILE *stream)
    {
    char *bufptr = NULL;
    char *p = bufptr;
    size_t size;
    int c;

    if (lineptr == NULL)return -1;
    if (stream == NULL) return -1;
    if (n == NULL) return -1;
    bufptr = *lineptr;
    size = *n;

    c = fgetc(stream);
    if (c == EOF) return -1;
    if (bufptr == NULL)
        {
        bufptr = malloc(128);
        if (bufptr == NULL)
            {
            return -1;
            }
        size = 128;
        }
    p = bufptr;
    while(c != EOF)
        {
        if ((p - bufptr) > (size - 1))
            {
            size = size + 128;
            bufptr = realloc(bufptr, size);
            if (bufptr == NULL)
                {
                return -1;
                }
            }
        *p++ = c;
        if (c == '\n')
            {
            break;
            }
        c = fgetc(stream);
        }
    *p++ = '\0';
    *lineptr = bufptr;
    *n = size;
    return p - bufptr - 1;
    }

void sleep_ms (int num)
    {
    struct timespec tspec;
    tspec.tv_sec=num/1000;
    tspec.tv_nsec=(num%1000)*1000000;
    nanosleep(&tspec,0);
    }

void printHelp()
    {
    flsprintf(stdout,"readout\n");
    exit(0);
    }


void parseArgs(int argc, char *argv[])
    {
    int c;
    while ((c = getopt (argc, argv, "c:nps:t:v:")) != -1)
        {
        switch (c)
            {
            case 'v' :
                sscanf(optarg,"%d",&verbose);
                break;
            case 'c' :
                COM=optarg;
                break;
            default:
                fprintf (stderr,"Bug, unhandled option '%c'\n",c);
                abort ();
            }
        }
    if (argc<=1)
        printHelp();
    }


int get_serial_line (char cmd, char * line)
	{
	char tmp;
	char array[20000];
	int array_ptr=0;
	
	putByte(cmd);
	tmp = getByte();
	while (tmp!=0)
		{
		array[array_ptr++] = tmp;
		tmp = getByte();
		}
	array[array_ptr]=0;
	strcpy(line,array);
	if (verbose > 3) printf("rx - %d --- %s \n",array_ptr,array);
	return array_ptr;
	}


int get_serial_data (char cmd, int * data_pts)
{
	
	char array[20000],arr_tmp[20];
	int array_vals[2000];
	int i,j,len,array_vals_ptr=0,data_point;
	len = get_serial_line(cmd,array);
	for (i=0;i<len;i=i+7)
		{
		for (j=0;j<6;j++)
			{
			arr_tmp[j]=array[i+j];
			arr_tmp[6] = 0;	
			}
//		if (verbose > 2) printf("ra %s \n",arr_tmp);
		sscanf(arr_tmp,"%d",&data_point);
		if (verbose > 2) printf("rb %d \n",data_point);
		*data_pts++ = data_point;
		}
	return len/7;
	}




/* fix_fft.c - Fixed-point in-place Fast Fourier Transform  */
/*
  All data are fixed-point short integers, in which -32768
  to +32768 represent -1.0 to +1.0 respectively. Integer
  arithmetic is used for speed, instead of the more natural
  floating-point.

  For the forward FFT (time -> freq), fixed scaling is
  performed to prevent arithmetic overflow, and to map a 0dB
  sine/cosine wave (i.e. amplitude = 32767) to two -6dB freq
  coefficients. The return value is always 0.

  For the inverse FFT (freq -> time), fixed scaling cannot be
  done, as two 0dB coefficients would sum to a peak amplitude
  of 64K, overflowing the 32k range of the fixed-point integers.
  Thus, the fix_fft() routine performs variable scaling, and
  returns a value which is the number of bits LEFT by which
  the output must be shifted to get the actual amplitude
  (i.e. if fix_fft() returns 3, each value of fr[] and fi[]
  must be multiplied by 8 (2**3) for proper scaling.
  Clearly, this cannot be done within fixed-point short
  integers. In practice, if the result is to be used as a
  filter, the scale_shift can usually be ignored, as the
  result will be approximately correctly normalized as is.

  Written by:  Tom Roberts  11/8/89
  Made portable:  Malcolm Slaney 12/15/94 malcolm@interval.com
  Enhanced:  Dimitrios P. Bouras  14 Jun 2006 dbouras@ieee.org
*/

/*
  Henceforth "short" implies 16-bit word. If this is not
  the case in your architecture, please replace "short"
  with a type definition which *is* a 16-bit word.
*/

/*
  Since we only use 3/4 of N_WAVE, we define only
  this many samples, in order to conserve data space.
*/
/*
const short Sinewave[N_WAVE] = {
      0,    201,    402,    603,    804,   1005,   1206,   1406,
   1607,   1808,   2009,   2209,   2410,   2610,   2811,   3011,
   3211,   3411,   3611,   3811,   4011,   4210,   4409,   4608,
   4807,   5006,   5205,   5403,   5601,   5799,   5997,   6195,
   6392,   6589,   6786,   6982,   7179,   7375,   7571,   7766,
   7961,   8156,   8351,   8545,   8739,   8932,   9126,   9319,
   9511,   9703,   9895,  10087,  10278,  10469,  10659,  10849,
  11038,  11227,  11416,  11604,  11792,  11980,  12166,  12353,
  12539,  12724,  12909,  13094,  13278,  13462,  13645,  13827,
  14009,  14191,  14372,  14552,  14732,  14911,  15090,  15268,
  15446,  15623,  15799,  15975,  16150,  16325,  16499,  16672,
  16845,  17017,  17189,  17360,  17530,  17699,  17868,  18036,
  18204,  18371,  18537,  18702,  18867,  19031,  19194,  19357,
  19519,  19680,  19840,  20000,  20159,  20317,  20474,  20631,
  20787,  20942,  21096,  21249,  21402,  21554,  21705,  21855,
  22004,  22153,  22301,  22448,  22594,  22739,  22883,  23027,
  23169,  23311,  23452,  23592,  23731,  23869,  24006,  24143,
  24278,  24413,  24546,  24679,  24811,  24942,  25072,  25201,
  25329,  25456,  25582,  25707,  25831,  25954,  26077,  26198,
  26318,  26437,  26556,  26673,  26789,  26905,  27019,  27132,
  27244,  27355,  27466,  27575,  27683,  27790,  27896,  28001,
  28105,  28208,  28309,  28410,  28510,  28608,  28706,  28802,
  28897,  28992,  29085,  29177,  29268,  29358,  29446,  29534,
  29621,  29706,  29790,  29873,  29955,  30036,  30116,  30195,
  30272,  30349,  30424,  30498,  30571,  30643,  30713,  30783,
  30851,  30918,  30984,  31049,  31113,  31175,  31236,  31297,
  31356,  31413,  31470,  31525,  31580,  31633,  31684,  31735,
  31785,  31833,  31880,  31926,  31970,  32014,  32056,  32097,
  32137,  32176,  32213,  32249,  32284,  32318,  32350,  32382,
  32412,  32441,  32468,  32495,  32520,  32544,  32567,  32588,
  32609,  32628,  32646,  32662,  32678,  32692,  32705,  32717,
  32727,  32736,  32744,  32751,  32757,  32761,  32764,  32766,
  32767,  32766,  32764,  32761,  32757,  32751,  32744,  32736,
  32727,  32717,  32705,  32692,  32678,  32662,  32646,  32628,
  32609,  32588,  32567,  32544,  32520,  32495,  32468,  32441,
  32412,  32382,  32350,  32318,  32284,  32249,  32213,  32176,
  32137,  32097,  32056,  32014,  31970,  31926,  31880,  31833,
  31785,  31735,  31684,  31633,  31580,  31525,  31470,  31413,
  31356,  31297,  31236,  31175,  31113,  31049,  30984,  30918,
  30851,  30783,  30713,  30643,  30571,  30498,  30424,  30349,
  30272,  30195,  30116,  30036,  29955,  29873,  29790,  29706,
  29621,  29534,  29446,  29358,  29268,  29177,  29085,  28992,
  28897,  28802,  28706,  28608,  28510,  28410,  28309,  28208,
  28105,  28001,  27896,  27790,  27683,  27575,  27466,  27355,
  27244,  27132,  27019,  26905,  26789,  26673,  26556,  26437,
  26318,  26198,  26077,  25954,  25831,  25707,  25582,  25456,
  25329,  25201,  25072,  24942,  24811,  24679,  24546,  24413,
  24278,  24143,  24006,  23869,  23731,  23592,  23452,  23311,
  23169,  23027,  22883,  22739,  22594,  22448,  22301,  22153,
  22004,  21855,  21705,  21554,  21402,  21249,  21096,  20942,
  20787,  20631,  20474,  20317,  20159,  20000,  19840,  19680,
  19519,  19357,  19194,  19031,  18867,  18702,  18537,  18371,
  18204,  18036,  17868,  17699,  17530,  17360,  17189,  17017,
  16845,  16672,  16499,  16325,  16150,  15975,  15799,  15623,
  15446,  15268,  15090,  14911,  14732,  14552,  14372,  14191,
  14009,  13827,  13645,  13462,  13278,  13094,  12909,  12724,
  12539,  12353,  12166,  11980,  11792,  11604,  11416,  11227,
  11038,  10849,  10659,  10469,  10278,  10087,   9895,   9703,
   9511,   9319,   9126,   8932,   8739,   8545,   8351,   8156,
   7961,   7766,   7571,   7375,   7179,   6982,   6786,   6589,
   6392,   6195,   5997,   5799,   5601,   5403,   5205,   5006,
   4807,   4608,   4409,   4210,   4011,   3811,   3611,   3411,
   3211,   3011,   2811,   2610,   2410,   2209,   2009,   1808,
   1607,   1406,   1206,   1005,    804,    603,    402,    201,
      0,   -201,   -402,   -603,   -804,  -1005,  -1206,  -1406,
  -1607,  -1808,  -2009,  -2209,  -2410,  -2610,  -2811,  -3011,
  -3211,  -3411,  -3611,  -3811,  -4011,  -4210,  -4409,  -4608,
  -4807,  -5006,  -5205,  -5403,  -5601,  -5799,  -5997,  -6195,
  -6392,  -6589,  -6786,  -6982,  -7179,  -7375,  -7571,  -7766,
  -7961,  -8156,  -8351,  -8545,  -8739,  -8932,  -9126,  -9319,
  -9511,  -9703,  -9895, -10087, -10278, -10469, -10659, -10849,
 -11038, -11227, -11416, -11604, -11792, -11980, -12166, -12353,
 -12539, -12724, -12909, -13094, -13278, -13462, -13645, -13827,
 -14009, -14191, -14372, -14552, -14732, -14911, -15090, -15268,
 -15446, -15623, -15799, -15975, -16150, -16325, -16499, -16672,
 -16845, -17017, -17189, -17360, -17530, -17699, -17868, -18036,
 -18204, -18371, -18537, -18702, -18867, -19031, -19194, -19357,
 -19519, -19680, -19840, -20000, -20159, -20317, -20474, -20631,
 -20787, -20942, -21096, -21249, -21402, -21554, -21705, -21855,
 -22004, -22153, -22301, -22448, -22594, -22739, -22883, -23027,
 -23169, -23311, -23452, -23592, -23731, -23869, -24006, -24143,
 -24278, -24413, -24546, -24679, -24811, -24942, -25072, -25201,
 -25329, -25456, -25582, -25707, -25831, -25954, -26077, -26198,
 -26318, -26437, -26556, -26673, -26789, -26905, -27019, -27132,
 -27244, -27355, -27466, -27575, -27683, -27790, -27896, -28001,
 -28105, -28208, -28309, -28410, -28510, -28608, -28706, -28802,
 -28897, -28992, -29085, -29177, -29268, -29358, -29446, -29534,
 -29621, -29706, -29790, -29873, -29955, -30036, -30116, -30195,
 -30272, -30349, -30424, -30498, -30571, -30643, -30713, -30783,
 -30851, -30918, -30984, -31049, -31113, -31175, -31236, -31297,
 -31356, -31413, -31470, -31525, -31580, -31633, -31684, -31735,
 -31785, -31833, -31880, -31926, -31970, -32014, -32056, -32097,
 -32137, -32176, -32213, -32249, -32284, -32318, -32350, -32382,
 -32412, -32441, -32468, -32495, -32520, -32544, -32567, -32588,
 -32609, -32628, -32646, -32662, -32678, -32692, -32705, -32717,
 -32727, -32736, -32744, -32751, -32757, -32761, -32764, -32766,
};
*/

/*
const short Sinewave[N_WAVE] = {
0,	804,	1607,	2409,	3210,	4009,	4805,	5599,
6389,	7176,	7958,	8735,	9507,	10273,	11033,	11787,
12533,	13272,	14003,	14725,	15439,	16143,	16838,	17522,
18196,	18859,	19511,	20151,	20778,	21394,	21996,	22585,
23161,	23722,	24269,	24802,	25320,	25822,	26309,	26781,
27236,	27674,	28096,	28501,	28889,	29260,	29613,	29948,
30265,	30564,	30845,	31107,	31350,	31574,	31780,	31966,
32133,	32281,	32409,	32518,	32607,	32676,	32726,	32756,
32767,	32758,	32729,	32680,	32612,	32524,	32417,	32290,
32143,	31977,	31792,	31588,	31365,	31123,	30862,	30583,
30285,	29969,	29635,	29283,	28914,	28527,	28123,	27702,
27265,	26811,	26341,	25855,	25353,	24836,	24304,	23758,
23197,	22623,	22035,	21433,	20819,	20192,	19553,	18902,
18240,	17566,	16883,	16189,	15485,	14772,	14050,	13320,
12582,	11836,	11083,	10323,	9557,	8785,	8008,	7227,
6441,	5651,	4857,	4061,	3262,	2461,	1659,	856,
52,	-752,	-1555,	-2357,	-3158,	-3957,	-4754,	-5548,
-6338,	-7125,	-7907,	-8685,	-9457,	-10224,	-10984,	-11738,
-12485,	-13224,	-13956,	-14679,	-15393,	-16098,	-16793,	-17478,
-18153,	-18816,	-19469,	-20109,	-20738,	-21354,	-21957,	-22547,
-23124,	-23686,	-24234,	-24768,	-25287,	-25790,	-26278,	-26751,
-27207,	-27646,	-28069,	-28476,	-28865,	-29237,	-29591,	-29927,
-30245,	-30545,	-30827,	-31090,	-31335,	-31560,	-31767,	-31954,
-32123,	-32272,	-32401,	-32511,	-32602,	-32672,	-32724,	-32755,
-32767,	-32759,	-32731,	-32684,	-32617,	-32530,	-32424,	-32298,
-32153,	-31989,	-31805,	-31602,	-31380,	-31139,	-30880,	-30602,
-30305,	-29990,	-29658,	-29307,	-28939,	-28553,	-28150,	-27730,
-27294,	-26841,	-26372,	-25887,	-25386,	-24870,	-24339,	-23794,
-23234,	-22661,	-22073,	-21473,	-20859,	-20233,	-19595,	-18944,
-18283,	-17610,	-16927,	-16234,	-15531,	-14819,	-14097,	-13368,
-12630,	-11884,	-11132,	-10372,	-9607,	-8836,	-8059,	-7278,
-6492,	-5702,	-4909,	-4113,	-3314,	-2513,	-1711,	-908
};
*/

const short Sinewave[N_WAVE] = {
		0,1608,3212,4808,6393,7962,9512,11039,
		12539,14010,15446,16846,18204,19519,20787,22005,
		23170,24279,25329,26319,27245,28105,28898,29621,
		30273,30852,31356,31785,32137,32412,32609,32728,
		32767,32728,32609,32412,32137,31785,31356,30852,
		30273,29621,28898,28105,27245,26319,25329,24279,
		23170,22005,20787,19519,18204,16846,15446,14010,
		12539,11039,9512,7962,6393,4808,3212,1608,
		0,-1608,-3212,-4808,-6393,-7962,-9512,-11039,
		-12539,-14010,-15446,-16846,-18204,-19519,-20787,-22005,
		-23170,-24279,-25329,-26319,-27245,-28105,-28898,-29621,
		-30273,-30852,-31356,-31785,-32137,-32412,-32609,-32728,
		-32767,-32728,-32609,-32412,-32137,-31785,-31356,-30852,
		-30273,-29621,-28898,-28105,-27245,-26319,-25329,-24279,
		-23170,-22005,-20787,-19519,-18204,-16846,-15446,-14010,
		-12539,-11039,-9512,-7962,-6393,-4808,-3212,-1608


};


/*
  FIX_MPY() - fixed-point multiplication & scaling.
  Substitute inline assembly for hardware-specific
  optimization suited to a particluar DSP processor.
  Scaling ensures that result remains 16-bit.
*/
short FIX_MPY(short a, short b)
{
	/* shift right one less bit (i.e. 15-1) */
	int c = ((int)a * (int)b) >> 14;
	/* last bit shifted out = rounding-bit */
	b = c & 0x01;
	/* last shift + rounding bit */
	a = (c >> 1) + b;
	return a;
}

/*
  fix_fft() - perform forward/inverse fast Fourier transform.
  fr[n],fi[n] are real and imaginary arrays, both INPUT AND
  RESULT (in-place FFT), with 0 <= n < 2**m; set inverse to
  0 for forward transform (FFT), or 1 for iFFT.
*/
int fix_fft(short fr[], short fi[], short m, short inverse)
{
	int mr, nn, i, j, l, k, istep, n, scale, shift;
	short qr, qi, tr, ti, wr, wi;

	n = 1 << m;

	/* max FFT size = N_WAVE */
	if (n > N_WAVE)
		return -1;

	mr = 0;
	nn = n - 1;
	scale = 0;

	/* decimation in time - re-order data */
	for (m=1; m<=nn; ++m) {
		l = n;
		do {
			l >>= 1;
		} while (mr+l > nn);
		mr = (mr & (l-1)) + l;

		if (mr <= m)
			continue;
		tr = fr[m];
		fr[m] = fr[mr];
		fr[mr] = tr;
		ti = fi[m];
		fi[m] = fi[mr];
		fi[mr] = ti;
	}

	l = 1;
	k = LOG2_N_WAVE-1;
	while (l < n) {
		if (inverse) {
			/* variable scaling, depending upon data */
			shift = 0;
			for (i=0; i<n; ++i) {
				j = fr[i];
				if (j < 0)
					j = -j;
				m = fi[i];
				if (m < 0)
					m = -m;
				if (j > 16383 || m > 16383) {
					shift = 1;
					break;
				}
			}
			if (shift)
				++scale;
		} else {
			/*
			  fixed scaling, for proper normalization --
			  there will be log2(n) passes, so this results
			  in an overall factor of 1/n, distributed to
			  maximize arithmetic accuracy.
			*/
			shift = 1;
		}
		/*
		  it may not be obvious, but the shift will be
		  performed on each data point exactly once,
		  during this pass.
		*/
		istep = l << 1;
		for (m=0; m<l; ++m) {
			j = m << k;
			/* 0 <= j < N_WAVE/2 */
			wr =  Sinewave[j+N_WAVE/4];
			wi = -Sinewave[j];
			if (inverse)
				wi = -wi;
			if (shift) {
				wr >>= 1;
				wi >>= 1;
			}
			for (i=m; i<n; i+=istep) {
				j = i + l;
				tr = FIX_MPY(wr,fr[j]) - FIX_MPY(wi,fi[j]);
				ti = FIX_MPY(wr,fi[j]) + FIX_MPY(wi,fr[j]);
				qr = fr[i];
				qi = fi[i];
				if (shift) {
					qr >>= 1;
					qi >>= 1;
				}
				fr[j] = qr - tr;
				fi[j] = qi - ti;
				fr[i] = qr + tr;
				fi[i] = qi + ti;
			}
		}
		--k;
		l = istep;
	}
	return scale;
}

/*
  fix_fftr() - forward/inverse FFT on array of real numbers.
  Real FFT/iFFT using half-size complex FFT by distributing
  even/odd samples into real/imaginary arrays respectively.
  In order to save data space (i.e. to avoid two arrays, one
  for real, one for imaginary samples), we proceed in the
  following two steps: a) samples are rearranged in the real
  array so that all even samples are in places 0-(N/2-1) and
  all imaginary samples in places (N/2)-(N-1), and b) fix_fft
  is called with fr and fi pointing to index 0 and index N/2
  respectively in the original array. The above guarantees
  that fix_fft "sees" consecutive real samples as alternating
  real and imaginary samples in the complex array.
*/
int fix_fftr(short f[], int m, int inverse)
{
	int i, N = 1<<(m-1), scale = 0;
	short tt, *fr=f, *fi=&f[N];

	if (inverse)
		scale = fix_fft(fi, fr, m-1, inverse);
	for (i=1; i<N; i+=2) {
		tt = f[N+i-1];
		f[N+i-1] = f[i];
		f[i] = tt;
	}
	if (! inverse)
		scale = fix_fft(fi, fr, m-1, inverse);
	return scale;
}

#define	PI			3.14159

void WindowBlackmanHarrisInit(int *pas32,int nLen)
 {
 int i;
 for (i=0;i<nLen;i++)
 {
  *pas32++=(65536*(0.35875 - 0.48829*cos(2*PI*i/(nLen-1)) + 0.14128*cos(4*PI*i/(nLen-1)) - 0.01168*cos(6*PI*i/(nLen-1))));
 } 
 }



#define	N_BINS		6
#define	N_BIN_1		(N_WAVE * 50 * 1) / 1000
#define	N_BIN_2		(N_WAVE * 50 * 2) / 1000
#define	N_BIN_3		(N_WAVE * 50 * 3) / 1000
#define	N_BIN_4		(N_WAVE * 50 * 4) / 1000
#define	N_BIN_5		(N_WAVE * 50 * 5) / 1000
#define	N_BIN_6		(N_WAVE * 50 * 6) / 1000



int n_bins[N_BINS] = {N_BIN_1,N_BIN_2,N_BIN_3,N_BIN_4,N_BIN_5,N_BIN_6};


int main(int argc, char *argv[])
    {
    int i,j,pages_performed,config,econfig;
    unsigned char * pm_point, * cm_point;
    unsigned char tdat[200];
    int curr_c_min, curr_c_max, curr_c_avg, volt_c_avg,volt_c_min,volt_c_max,tran_num;
    int curr_t_min, curr_t_max, curr_t_avg, volt_t_avg,volt_t_min,volt_t_max;
    int curr_c_int=0, curr_t_int=0,volt_c_int=0, volt_t_int=0,pow_c,pow_t,curr_t_tran, pow_t_tran;
    int data_points[2000];
    int volt_c[1000], curr_c[1000],curr_c_raw[1000],data_c_points;
    int volt_t[1000], curr_t[1000],data_t_points;
    short fft_in_arr_r[N_WAVE],fft_in_arr_i[N_WAVE];
	short fft_out_arr[N_WAVE/2];
	int fft_window[N_WAVE],fft_out_perc[N_WAVE/2];
	int fft_bins_abs[N_BINS],fft_bins_rel[N_BINS];
    int tt1,tt2;
    
    WindowBlackmanHarrisInit(fft_window,N_WAVE);
    
    parseArgs(argc,argv);
  //  if (verbose>0) printf ("readout %s\n",PP_VERSION);
    if (verbose>1) printf ("Opening serial port\n");
    initSerialPort();
    
    j = get_serial_data('c',data_points);
	data_c_points = data_points[0];
	for (i=0;i<data_c_points;i++)
			{
			volt_c[i] = (data_points[i+1])/65;
			curr_c[i] = (data_points[i+data_c_points+1]) / 10;	
			curr_c_raw[i] = (data_points[i+data_c_points+1]) / 1;	
			}
//    for (i=0;i<data_c_points;i++) printf("ra3 - V: %d I:%d \n",volt_c[i],curr_c[i]);
		
	curr_c_min = 65536;
	curr_c_max = -65536;	
	volt_c_min = 65536;
	volt_c_max = -65536;	
	curr_c_avg = 0;
	volt_c_avg = 0;
    for (i=0;i<data_c_points;i++)
			{
			if (curr_c[i] > curr_c_max) curr_c_max = curr_c[i];
			if (curr_c[i] < curr_c_min) curr_c_min = curr_c[i];
			if (volt_c[i] < volt_c_min) volt_c_min = volt_c[i];
			if (volt_c[i] > volt_c_max) volt_c_max = volt_c[i];
			curr_c_avg = curr_c_avg + curr_c[i];	
			volt_c_avg = volt_c_avg + volt_c[i];
			}
	curr_c_avg = curr_c_avg / data_c_points;
	volt_c_avg = volt_c_avg / data_c_points;
	if (verbose>1) printf(" P: %d\n",data_c_points);
	if (verbose>1) printf(" CA %d CN %d CX %d\n",curr_c_avg,curr_c_min,curr_c_max);
	if (verbose>1) printf(" VA %d VN %d VX %d\n",volt_c_avg,volt_c_min,volt_c_max);
	
	
	
	
    j = get_serial_data('t',data_points);
    tran_num = data_points[1];
    data_t_points = data_points[0];
	for (i=0;i<data_t_points;i++)
			{
			volt_t[i] = (data_points[i+2])/65;
			curr_t[i] = (data_points[i+data_t_points+2])/10;	
			}

	if (verbose>1) printf(" t: %d %d\n",data_t_points,tran_num);
	curr_t_min = 65536;
	curr_t_max = -65536;	
	volt_t_min = 65536;
	volt_t_max = -65536;	
	curr_t_tran = 0;
	curr_t_avg = 0;
	volt_t_avg = 0;
    for (i=0;i<data_t_points;i++)
			{
			if (curr_t[i] > curr_t_max) curr_t_max = curr_t[i];
			if (curr_t[i] < curr_t_min) curr_t_min = curr_t[i];
			if (volt_t[i] < volt_t_min) volt_t_min = volt_t[i];
			if (volt_t[i] > volt_t_max) volt_t_max = volt_t[i];
			curr_t_avg = curr_t_avg + curr_t[i];	
			volt_t_avg = volt_t_avg + volt_t[i];
			}
	curr_t_avg = curr_t_avg / data_t_points;
	volt_t_avg = volt_t_avg / data_t_points;
	if (abs(curr_t_max-curr_t_avg) > abs(curr_t_max-curr_t_avg))
		curr_t_tran = abs(curr_t_max-curr_t_avg);
	else
		curr_t_tran = abs(curr_t_max-curr_t_avg);
	if (verbose>1) printf(" CA %d CN %d CX %d\n",curr_t_avg,curr_t_min,curr_t_max);
	if (verbose>1) printf(" VA %d VN %d VX %d\n",volt_t_avg,volt_t_min,volt_t_max);
 
 
 
    for (i=0;i<data_c_points;i++)
		{
		curr_c_int = curr_c_int + abs((curr_c[i] - curr_c_avg));
		volt_c_int = volt_c_int + abs((volt_c[i] - volt_c_avg));
		}
	curr_c_int = curr_c_int / data_c_points;
	volt_c_int = volt_c_int / data_c_points;
   for (i=0;i<data_t_points;i++)
		{
		curr_t_int = curr_t_int + abs((curr_t[i] - curr_t_avg));
		volt_t_int = volt_t_int + abs((volt_t[i] - volt_t_avg));
		}
	curr_t_int = curr_t_int / data_t_points;
	volt_t_int = volt_t_int / data_t_points;
	pow_c = curr_c_int * volt_c_int;
	pow_t = curr_t_int * volt_t_int;
	pow_t_tran = curr_t_tran * volt_t_int;

 
	
 
 	for (i=0;i<N_WAVE;i++)
		{
		tt1 = curr_c_raw[i];
		tt1 = tt1 * fft_window[i];
		tt1 = tt1 / 65536;
//		printf("%d ",curr_c_raw[i]);
		fft_in_arr_r[i] = tt1;
		fft_in_arr_i[i] = 0;
		}
	fix_fft(fft_in_arr_r,fft_in_arr_i,LOG2_N_WAVE,0);
	for (i=0;i<N_WAVE/2;i++)
		{
		tt1 = fft_in_arr_r[i];
		tt1 = tt1*tt1;
		tt2 = fft_in_arr_i[i];
		tt2 = tt2*tt2;
		tt1 = tt1 + tt2;
		tt1 = sqrt(tt1);
		fft_out_arr[i] = tt1;
		}

	int fourier_total=0;

	for (i=0;i<N_BINS;i++)
		{
		fft_bins_abs[i] = fft_out_arr[n_bins[i]] + fft_out_arr[n_bins[i]-1] + fft_out_arr[n_bins[i]+1];	
		fourier_total = fourier_total + fft_bins_abs[i];
		}


//	printf(" CC %d CV %d CP %d TC %d TV %d TP %d TCT %d TPT %d\n",curr_c_int,volt_c_int,pow_c, curr_t_int,volt_t_int, pow_t,curr_t_tran,pow_t_tran);
	printf("NT %d PC %d PT %d PP %d ",tran_num, pow_c/1000,pow_t/1000,pow_t_tran/1000);
	for (i=0;i<N_BINS;i++)
		{
		printf("H%d %d ",i+1,(fft_bins_abs[i] * 100) / fourier_total);
		}
	printf("\n");
    return 0;
    }








