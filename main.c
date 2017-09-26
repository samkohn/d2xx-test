/*
 * Assuming libftd2xx.so is in /usr/local/lib, build with:
 * 
 *     gcc -o bitmode main.c -L. -lftd2xx -Wl,-rpath /usr/local/lib
 * 
 * and run with:
 * 
 *     sudo ./bitmode [port number]
 */
#include <stdio.h>
#include <unistd.h>
#include "ftd2xx.h"

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
    (byte & 0x40 ? '1' : '0'), \
    (byte & 0x20 ? '1' : '0'), \
    (byte & 0x10 ? '1' : '0'), \
    (byte & 0x08 ? '1' : '0'), \
    (byte & 0x04 ? '1' : '0'), \
    (byte & 0x02 ? '1' : '0'), \
    (byte & 0x01 ? '1' : '0') 

UCHAR getWord(UCHAR* inputData, UCHAR channel) {
  /* 
     Helper function to grab data from channel 1
     Only keeps first eight bits from input data
  */
  if ( sizeof(inputData) < 8 )
    return 0;
  
  int returnValue = 0;
  for( int i = 0; i < 8; i++ ) {
    returnValue = returnValue << 1;
    returnValue = returnValue | (((int)inputData[i] & channel) >> (channel-1));
  }

  return returnValue;
}

int main(int argc, char *argv[])
{
	//DWORD       baudRate = 5000000;
	DWORD       divisor = 0;
	UCHAR ASYNC = 0x01;
	UCHAR SYNC  = 0x04;

	DWORD       bytesRead = 0;
	DWORD       bytesWritten = 0;
	FT_STATUS	ftStatus = FT_OK;
	FT_HANDLE	ftHandleA;
	FT_HANDLE	ftHandleB;
	
	int         outputBufferLength = atoi(argv[2]);
	UCHAR       outputData[outputBufferLength];
	int         inputBufferLength = outputBufferLength;
	UCHAR       inputData[inputBufferLength];
	UCHAR       pinStatus;
	int         portNumber;
	DWORD       USBTransferSize = 64; // bytes
	DWORD       timeout = 10; 

	if (argc > 1) 
	{
		sscanf(argv[1], "%d", &portNumber);
	}
	else 
	{
		portNumber = 0;
	}
	
	ftStatus = FT_Open(portNumber, &ftHandleA);
	if (ftStatus != FT_OK) 
	{
		/* FT_Open can fail if the ftdi_sio module is already loaded. */
		printf("FT_Open(%d) failed (error %d).\n", portNumber, (int)ftStatus);
		printf("Use lsmod to check if ftdi_sio (and usbserial) are present.\n");
		printf("If so, unload them using rmmod, as they conflict with ftd2xx.\n");
		return 1;
	}

	/* Enable bit-bang mode, where 8 UART pins (RX, TX, RTS etc.) become
	 * general-purpose I/O pins.
	 */
	printf("Selecting synchronous bit-bang mode.\n");	
	ftStatus = FT_SetBitMode(ftHandleA, 
	                         0x01, /* sets all 7 pins as inputs - pin 0 is output*/
	                         SYNC);
	if (ftStatus != FT_OK) 
	  {
	    printf("FT_SetBitMode failed (error %d).\n", (int)ftStatus);
	    return 0;
	  }

	/* In bit-bang mode, setting the baud rate gives a clock rate
	 * 16 times higher, e.g. baud = 9600 gives 153600 bytes per second.
	 */
	ftStatus = FT_SetDivisor(ftHandleA, divisor);
	//ftStatus = FT_SetBaudRate(ftHandleA, baudRate);
	if (ftStatus != FT_OK) 
	  {
	    printf("FT_SetBaudRate failed (error %d).\n", (int)ftStatus);
	    return 0;
	  }
	
	/* Set timeouts for read cycle
	 */
	ftStatus = FT_SetTimeouts(ftHandleA, timeout, timeout);
	//ftStatus = FT_SetTimeouts(ftHandleB, timeout, timeout);
	if (ftStatus != FT_OK)
          {
            printf("FT_SetTimeouts failed (error %d).\n", (int)ftStatus);
            return 0;
          }

	/* Set usb transfer size 
	 */
	ftStatus = FT_SetUSBParameters(ftHandleA, USBTransferSize, USBTransferSize);
	if (ftStatus != FT_OK)
          {
            printf("FT_SetUSBParameters failed (error %d).\n", (int)ftStatus);
            return 0;
          }

	/* Use FT_Write to set values of output pins.  Here we set
	 * them to alternate low and high (0xAA == 10101010).
	 */
	UCHAR test1 = 0x01;
	UCHAR test2 = 0x00;
	UCHAR readTest = 0x00;
	for ( int i = 0; i < outputBufferLength; i++ ) {
	  if( i%2 == 0) 
	    outputData[i] = test1;
	  else
	    outputData[i] = test2;
	}
	for ( int i = 0; i < inputBufferLength; i++ )
	  inputData[i] = readTest;

	FT_Purge(ftHandleA, FT_PURGE_RX | FT_PURGE_TX);
	FILE *f = fopen("test.txt", "w");

	/* Write to buffer loop
	 */
	int counter = 0;
	//while( ftStatus == FT_OK && counter < 100) {
	  bytesRead = 0;
	  bytesWritten = 0;
	  ftStatus = FT_Write(ftHandleA, outputData, sizeof(outputData), &bytesWritten);
	  ftStatus = FT_Read(ftHandleA, inputData, sizeof(inputData), &bytesRead);
	  FT_Purge(ftHandleA, FT_PURGE_TX | FT_PURGE_RX);

	  //fwrite(inputData, sizeof(UCHAR), sizeof(inputData), f);

	  //readTest = getWord(inputData, 0x2);
	  //printf("Bytes written: %d\n", (int)bytesWritten); 
	  //printf("Bytes read: %d, Word read: "BYTE_TO_BINARY_PATTERN"\n", (int)bytesRead, BYTE_TO_BINARY(readTest)); 

	  //usleep(delay);
	  
	  //for ( int i = 0; i < inputBufferLength; i++ )
	  //inputData[i] = readTest;

	  //counter++;
	//}

	fclose(f);
	
	if (ftStatus != FT_OK) 
	  {
	    printf("FT_Write failed (error %d).\n", (int)ftStatus);
	    return 0;
	  }

exit:
	/* Return chip to default (UART) mode. */
	(void)FT_SetBitMode(ftHandleA, 
	                    0, /* ignored with FT_BITMODE_RESET */
	                    FT_BITMODE_RESET);
	//(void)FT_SetBitMode(ftHandleB, 
	//                  0, /* ignored with FT_BITMODE_RESET */
			      //                  FT_BITMODE_RESET);

	(void)FT_Close(ftHandleA);
	//(void)FT_Close(ftHandleB);
	return 0;
}
