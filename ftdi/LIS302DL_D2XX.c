#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include "libftd2xx/ftd2xx.h"
#include "LIS302DL_D2XX.h"

#define MAX_LOOPS	3000
char LIS302DL_debug;

#define ClearQueue()	memset(Queue, 0, QueueLen);QueueUsed = 0

#define QUEUECHUNK	10
unsigned char *Queue=0;
int QueueUsed=0;
int QueueLen=0;
int AddByteToQueue(unsigned char val)
{
        extern unsigned char *Queue;
        unsigned char *QueuePtr;

        if (Queue == 0 || (QueueLen - QueueUsed) == 0)
        {
                if ((Queue = realloc(Queue, QueueLen + QUEUECHUNK)) == NULL)
                {
                        printf("AddByteToQueue: Error! Cannot allocate %d bytes!\n",\
                                QUEUECHUNK);
                        exit(1);
                }
                QueueLen = QueueLen + QUEUECHUNK;
        }
        QueuePtr = (Queue + QueueUsed);
        QueueUsed++;
        *QueuePtr = val;

        return 0;
}
void printbin(unsigned char dat)
{
	unsigned char i;
	for (i = 0; i < 8; i++) {
		if ((dat & 0x80) > 0)
			printf("1");
		else
			printf("0");
		dat = dat << 1;
	}
	printf("\n");
}

int LIS302DL(unsigned char reg, unsigned char *dat, unsigned char dir)
{
// -----------------------------------------------------------
// Static Variables
// -----------------------------------------------------------
	static FT_HANDLE ftHandle = 0;	// Handle of the FTDI device
// -----------------------------------------------------------
// Volatile Stack Variables
// -----------------------------------------------------------
//
	FT_DEVICE_LIST_INFO_NODE *devInfo;
	FT_STATUS ftStatus;	// Result of each D2XX call
	DWORD dwNumDevs;	// The number of devices
	DWORD dwDevNum;		// Our target device
	BYTE byOutputBuffer[8];	// Buffer to hold MPSSE commands and data
	// to be sent to the FT2232H
	BYTE byInputBuffer[8];	// Buffer to hold data read from the FT2232H
	DWORD dwNumBytesSent = 0;	// Count of actual bytes sent - used with FT_Write
	DWORD dwNumBytesToRead = 0;	// Number of bytes available to read
	// in the driver's input buffer
	DWORD dwNumBytesRead = 0;	// Count of actual bytes read - used with FT_Read
	int i;


	// We hold the ftHandle as a static variable, and only
	// perform the device initialization once
	if (ftHandle == 0) {
		// -----------------------------------------------------------
		// Does an FTDI device exist?
		// -----------------------------------------------------------
		printf("Checking for FTDI devices...\n");
		// Get the number of FTDI devices
		ftStatus = FT_CreateDeviceInfoList(&dwNumDevs);
		if (ftStatus != FT_OK)	// Did the command execute OK?
		{
			printf("Error in getting the number of devices\n");
			return 1;	// Exit with error
		}
		if (dwNumDevs < 1)	// Exit if we don't see any
		{
			printf("There are no FTDI devices installed\n");
			return 1;	// Exit with error
		}
		// Malloc some space for the devic information
		devInfo =
		    (FT_DEVICE_LIST_INFO_NODE *)
		    malloc(sizeof(FT_DEVICE_LIST_INFO_NODE)
			   * dwNumDevs);
		//
		// get the device information
		//
		ftStatus = FT_GetDeviceInfoList(devInfo, &dwNumDevs);

		dwDevNum = -1;
		if (ftStatus == FT_OK) {
			for (i = 0; i < dwNumDevs; i++) {
				/*
				   printf("Dev %d:\n", i);
				   printf(" Flags=0x%x\n", devInfo[i].Flags);
				   printf(" Type=0x%x\n", devInfo[i].Type);
				   printf(" ID=0x%x\n", devInfo[i].ID);
				   printf(" LocId=0x%x\n", devInfo[i].LocId);
				   printf(" SerialNumber=%s\n", devInfo[i].SerialNumber);
				   printf(" Description=%s\n", devInfo[i].Description);
				   printf(" ftHandle=0x%x\n", devInfo[i].ftHandle);
				 */
				if (strncmp
				    ("Accelerometer", devInfo[i].Description,
				     strlen("Accelerometer")) == 0) {
					dwDevNum = i;
					break;
				}
			}
		}
		if (dwDevNum == -1) {
			printf
			    ("Cannot locate accelerometer device. Exiting.\n");
			return 1;
		}

		printf
		    ("FTDI device number %ld identified as accelerometer device.\n",
		     dwDevNum);

		ftStatus = FT_Open(dwDevNum, &ftHandle);
		if (ftStatus != FT_OK) {
			printf("Open Failed. Error: %ld\n", ftStatus);
			return 1;	// Exit with error
		}
		// Configure MPSSE
		printf("\nConfiguring MPSSE...\n");
		ftStatus |= FT_ResetDevice(ftHandle);

		//Purge any buffered I/O
		ftStatus |= FT_Purge(ftHandle, FT_PURGE_RX | FT_PURGE_TX);

		// How are we doing so far?
		if (ftStatus != FT_OK)
		{
			printf("Error: FTD initialize failed! (PART1)!\n");
			return 1;
		}

		// Get the number of bytes in the FT2232H receive buffer
		ftStatus = FT_GetQueueStatus(ftHandle, &dwNumBytesToRead);
		if ((ftStatus == FT_OK) && (dwNumBytesToRead > 0)) {
			printf("Error: Failed to clear internal buffers\n");
			return 1;
		}

		//Set USB request transfer sizes to 64K
		ftStatus = FT_SetUSBParameters(ftHandle, 65536, 65535);

		//Disable event and error characters
		ftStatus |= FT_SetChars(ftHandle, false, 0, false, 0);

		//Sets the read and write timeouts in milliseconds
		ftStatus |= FT_SetTimeouts(ftHandle, 0, 5000);

		//Set the latency timer to 1mS (default is 16mS)
		ftStatus |= FT_SetLatencyTimer(ftHandle, 1);

		//Turn on flow control to synchronize IN requests
		ftStatus |=
		    FT_SetFlowControl(ftHandle, FT_FLOW_RTS_CTS, 0x00, 0x00);

		//Reset controller 
		ftStatus |= FT_SetBitMode(ftHandle, 0x0, 0x00);

		// How are we doing so far?
		if (ftStatus != FT_OK)
		{
			printf("Error: FTD initialize failed! (PART1)!\n");
			return 1;
		}

		//Enable MPSSE mode
		ftStatus = FT_SetBitMode(ftHandle, 0x0b, 0x02);
		// 0x0b = 0b00001011 = Pins 1, 2, and 4 as outputs
		// 0x02 = MPSSE mode

		if (ftStatus != FT_OK) {
			printf("Error in initializing the MPSSE %ld\n",
			       ftStatus);
			FT_Close(ftHandle);
			return 1;	// Exit with error
		}

		// Disable internal loop-back
		ClearQueue();
		AddByteToQueue(0x85);
		// Send off the loopback command
		ftStatus = FT_Write(ftHandle, Queue,
				    QueueUsed, &dwNumBytesSent);

		// Check the receive buffer - it should be empty
		ftStatus = FT_GetQueueStatus(ftHandle, &dwNumBytesToRead);
		if (dwNumBytesToRead != 0) {
			printf
			    ("INIT 1: Error %ld - MPSSE receive buffer should be empty (%ld)\n",
			     ftStatus, dwNumBytesToRead);
			// Dump out the errant bytes
			FT_Read(ftHandle, &byInputBuffer, dwNumBytesToRead,
				&dwNumBytesRead);
			for (i = 0; i < dwNumBytesToRead; i++)
				printf("%02d: 0x%02x\n", i, byInputBuffer[i]);

			// Reset the port to disable MPSSE
			FT_SetBitMode(ftHandle, 0x0, 0x00);
			// Close up
			FT_Close(ftHandle);	// Close the USB port
			// and exit
			return 1;	// Exit with error
		}
		// -----------------------------------------------------------
		// Synchronize the MPSSE by sending a bogus opcode (0xAB),
		// The MPSSE will respond with "Bad Command" (0xFA) followed by
		// the bogus opcode itself.
		// -----------------------------------------------------------
		ClearQueue();	// Reset output buffer pointer
		AddByteToQueue(0xAB);
		ftStatus = FT_Write(ftHandle, Queue, QueueUsed,
				    &dwNumBytesSent);

		// Loop until we see some data ready to read (or die if looping too much)
		i = 0;
		do {
			ftStatus =
			    FT_GetQueueStatus(ftHandle, &dwNumBytesToRead);
			i++;
			usleep(10);
		} while ((dwNumBytesToRead == 0) && (ftStatus == FT_OK)
			 && (i < MAX_LOOPS));
		printf("(INFO) Read delay measured as (%d)\n", i);

		if (i == MAX_LOOPS) {
			printf("Device not responding. Giving up.\n");
			return 1;
		}
		// Read out the data from input buffer
		ftStatus =
		    FT_Read(ftHandle, &byInputBuffer, dwNumBytesToRead,
			    &dwNumBytesRead);

		// Check if Bad command and echo command are received
		if ((byInputBuffer[0] == 0xFA) && (byInputBuffer[1] == 0xAB)) {
			printf("(INFO) MPSSE in Sync...\n");
		} else {
			printf("Error in synchronizing the MPSSE\n");
			FT_Close(ftHandle);
			return 1;	// Exit with error
		}
	}

	// Queue should be empty
	ftStatus = FT_GetQueueStatus(ftHandle, &dwNumBytesToRead);
	if (ftStatus != 0)
	{
		printf("ERR: Status=%ld\n", ftStatus);
		return 1;
	}


	// Device is initialized and (hopefully!) ready.
	//

	ClearQueue();	// Reset output buffer pointer
	// Assert CS
	AddByteToQueue(0x80);
	AddByteToQueue(0 & ~0x08);
	AddByteToQueue(0x0b);

	// Are we reading, or writing the Accelerometer
	// 'READ'
	if (dir == 'r') {
		//if (LIS302DL_debug)
			//printf("Reading..\n");
		// Start by writing the register address we
		// are interested in
		AddByteToQueue(0x11);
		AddByteToQueue(0x00);
		AddByteToQueue(0x00);
		AddByteToQueue(reg | 0x80);
		
		// Then we can read its value
		AddByteToQueue(0x20);
		AddByteToQueue(0x00);
		AddByteToQueue(0x00);

		// Deassert CS
		AddByteToQueue(0x80);
		AddByteToQueue(0x08);
		AddByteToQueue(0x0b);

		// Send the commands out to the FTDI chip
		ftStatus =
		    FT_Write(ftHandle, Queue, QueueUsed,
			     &dwNumBytesSent);
		if (ftStatus != 0)
			printf("ERR: Status=%ld\n", ftStatus);

		// Wait for a response
		i = 0;
		do {
			ftStatus =
			    FT_GetQueueStatus(ftHandle, &dwNumBytesToRead);
			if (ftStatus != 0)
				printf("ERR: Status=%ld\n", ftStatus);
			i++;
			usleep(10);
		} while ((dwNumBytesToRead == 0) && (ftStatus == FT_OK)
			 && (i < MAX_LOOPS));
		//if (LIS302DL_debug)
			//printf("Read delay measured as (%d)\n", i);
		if (i == MAX_LOOPS) {
			printf("Device not responding. Giving up.\n");
			return 1;
		}

/*
		if (LIS302DL_debug)
			printf("%ld bytes waiting for read\n",
			       dwNumBytesToRead);
*/

		// Pick up the value returned by the accelerometer
		ftStatus =
		    FT_Read(ftHandle, &byInputBuffer, dwNumBytesToRead,
			    &dwNumBytesRead);
		if (ftStatus != 0)
		{
			printf("ERR: Status=%ld\n", ftStatus);
			return 1;
		}
/*
		if (LIS302DL_debug)
			for (i = 0; i < dwNumBytesToRead; i++)
				printf("%02d: 0x%02x\n", i, byInputBuffer[i]);
*/
		// Push the data into the indicated location
		*dat = byInputBuffer[0];
	}
	
	// 'WRITE'
	if (dir == 'w') {
		if (LIS302DL_debug)
			printf("Writing..\n");

		// For writing, we send the register and
		// the data together
		AddByteToQueue(0x11);
		AddByteToQueue(0x01);
		AddByteToQueue(0x00);
		AddByteToQueue(reg);
		AddByteToQueue(*dat);
		// Deassert CS
		AddByteToQueue(0x80);
		AddByteToQueue(0x08);
		AddByteToQueue(0x0b);

		// Send the commands out to the FTDI chip
		ftStatus = FT_Write(ftHandle, Queue, QueueUsed,
				    &dwNumBytesSent);
		if (ftStatus != 0)
		{
			printf("ERR: Status=%ld\n", ftStatus);
			return 1;
		}

		ftStatus = FT_GetQueueStatus(ftHandle, &dwNumBytesToRead);
		if (ftStatus != 0)
		{
			printf("ERR: Status=%ld\n", ftStatus);
			return 1;
		}
		if (dwNumBytesToRead != 0) {
			printf ("ERROR_AFTER_WRITE: %ld bytes in queue\n",
			     dwNumBytesToRead);
			for (i = 0; i < dwNumBytesToRead; i++)
				printf("%02d: 0x%02x\n", i, byInputBuffer[i]);
			return 1;
		}
	}

	return 0;
}

