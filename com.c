#ifdef __MINGW32__
#include <windows.h>
#include <winbase.h>
#endif
#ifdef __linux
#include <sys/types.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include "log.h"

#define  __linux
#ifdef __MINGW32__
static HANDLE hSerial;
#endif
#ifdef __linux
static int fd;
#endif


static int         _fd = -1;

char *array_to_str(char * str, uint8_t *array, unsigned int n) {
  int r;
  if (n == 0) return 0;
  if (n == 1) r = sprintf(str, "%X", array[0]);
  else        r = sprintf(str, "%X, ", array[0]);
  array_to_str(str + r, array + 1, n - 1);
  return str;
}

static uint32_t COM_Baudrate = 115200;

/** \brief Open COM port with settings
 *
 * \param [in] port Port name as string
 * \param [in] baudrate Port baudrate
 * \param [in] have_parity true if parity should be switched on
 * \param [in] two_stopbits true if 2 stop bits used
 * \return true if succeed
 *
 */
bool COM_Open(char *port, uint32_t baudrate, bool have_parity, bool two_stopbits)
{
  //printf("Opening %s at %u baud\n", port, baudrate);
  COM_Baudrate = baudrate;

  #ifdef __MINGW32__
  char str[64];
  uint8_t multiplier;

  sprintf(str, "\\\\.\\%s", port);
  hSerial = CreateFile(str, GENERIC_READ | GENERIC_WRITE, 0,
                              NULL, OPEN_EXISTING, 0, NULL);
  if (hSerial == INVALID_HANDLE_VALUE)
    return false;
  DCB dcbSerialParams = { 0 }; // Initializing DCB structure
  dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
  GetCommState(hSerial, &dcbSerialParams);
  dcbSerialParams.BaudRate = baudrate;  // Setting BaudRate
  dcbSerialParams.ByteSize = 8;         // Setting ByteSize = 8
  if (two_stopbits == true)
    dcbSerialParams.StopBits = TWOSTOPBITS;
  else
    dcbSerialParams.StopBits = ONESTOPBIT;
  if (have_parity == true)
    dcbSerialParams.Parity   = EVENPARITY;  // Setting Parity = None
  else
    dcbSerialParams.Parity   = NOPARITY;
  dcbSerialParams.fDtrControl = DTR_CONTROL_DISABLE;
  SetCommState(hSerial, &dcbSerialParams);
  COMMTIMEOUTS timeouts;
  multiplier = (uint8_t)ceil((float)100000 / baudrate);
  timeouts.ReadIntervalTimeout = 20 * multiplier;
  timeouts.ReadTotalTimeoutMultiplier = 1 * multiplier;
  timeouts.ReadTotalTimeoutConstant = 100 * multiplier;
  timeouts.WriteTotalTimeoutMultiplier = 1;
  timeouts.WriteTotalTimeoutConstant = 1;
  SetCommTimeouts(hSerial, &timeouts);
  //COM_Bytes = 0;
  #endif
  #ifdef __linux
  fd = open(port, O_RDWR | O_NOCTTY );
  if (fd <0)
    return false;
  struct termios SerialPortSettings;
  tcgetattr(fd, &SerialPortSettings);	/* Get the current attributes of the Serial port */
  /* Setting the Baud rate */
  switch (baudrate)
  {
    case 300:
      cfsetispeed(&SerialPortSettings, B300);
      cfsetospeed(&SerialPortSettings, B300);
      break;

    case 4800:
      cfsetispeed(&SerialPortSettings, B4800);
      cfsetospeed(&SerialPortSettings, B4800);
      break;

    case 9600:
      cfsetispeed(&SerialPortSettings, B9600);
      cfsetospeed(&SerialPortSettings, B9600);
      break;
    case 38400:
      cfsetispeed(&SerialPortSettings, B38400);
      cfsetospeed(&SerialPortSettings, B38400);
      break;
    case 57600:
      cfsetispeed(&SerialPortSettings, B57600);
      cfsetospeed(&SerialPortSettings, B57600);
      break;
    default:
    case 115200:
      cfsetispeed(&SerialPortSettings, B115200);
      cfsetospeed(&SerialPortSettings, B115200);
      break;
  }
  cfmakeraw(&SerialPortSettings);           /* Set raw mode (special processing disabled) */
  if (have_parity == true)
    SerialPortSettings.c_cflag |= PARENB;   /* Enables the Parity Enable bit(PARENB) */
  else
    SerialPortSettings.c_cflag &= ~PARENB;  /* Disables the Parity Enable bit(PARENB), so No Parity */
  if (two_stopbits == true)
    SerialPortSettings.c_cflag |= CSTOPB;   /* CSTOPB = 2 Stop bits */
  else
    SerialPortSettings.c_cflag &= ~CSTOPB;  /* CSTOPB = 2 Stop bits,here it is cleared so 1 Stop bit */
  SerialPortSettings.c_cflag |= (CREAD | CLOCAL); /* Enable receiver,Ignore Modem Control lines       */
  SerialPortSettings.c_cc[VMIN]  = 0;            // read doesn't block
  SerialPortSettings.c_cc[VTIME] = 5;            // 0.1 seconds read timeout
  tcsetattr(fd, TCSANOW, &SerialPortSettings);  /* Set the attributes to the termios structure*/
  tcflush(fd, TCIFLUSH);
  #endif

  return true;
}

/** \brief Write data to COM port
 *
 * \param [in] data Data buffer for writing
 * \param [in] len Length of data buffer
 * \return 0 if everything Ok
 *
 */
int COM_Write(uint8_t *data, uint16_t len)
{
  usleep(4000);
  char buf[512];
  LOG_Print(LOG_LEVEL_INFO, "COM_Write(%s)", array_to_str(buf, data, len));
  #ifdef __MINGW32__
  DWORD dwBytesWritten = 0;
  //DWORD signal;
  //OVERLAPPED ov = { 0 };
  //int res;
  //ov.hEvent = CreateEvent(NULL, true, true, NULL);

  if (!WriteFile(hSerial, data, len, &dwBytesWritten, NULL))
    return -1;
  //COM_Bytes += dwBytesWritten;
//  WriteFile(hSerial, data, len, &dwBytesWritten, &ov);
//  signal = WaitForSingleObject(ov.hEvent, INFINITE);
//  if ((signal == WAIT_OBJECT_0) && (GetOverlappedResult(hSerial, &ov, &dwBytesWritten, true)))
//    res = 0;
//  else
//    res = -1;
//  CloseHandle(ov.hEvent);
//  return res;
  #endif
  #ifdef __linux
  tcflush(fd, TCIOFLUSH);
  int iOut = write(fd, data, len);
  if (iOut < 0)
    return -1;
  #endif

  return 0;
}

/** \brief Read data from COM port
 *
 * \param [out] data Data buffer to read data in
 * \param [in] len Length of data to read
 * \return number of received bytes as int
 *
 */



int COM_Read(uint8_t *data, uint16_t len)
{
  usleep(5000);
  char buf[512];



  #ifdef __MINGW32__
  //OVERLAPPED ov = { 0 };
  //COMSTAT status;
  //DWORD errors;
  //DWORD mask, btr, temp, signal;
  DWORD dwBytesRead = 0;

//  ClearCommError(hSerial, &errors, &status);
//  if (!ReadFile(hSerial, data, len, &dwBytesRead, &ov))
//    return -1;

//  btr = 0;
//  while (btr < len)
//  {
//    SetCommMask(hSerial, EV_RXCHAR);
//    WaitCommEvent(hSerial, &mask, NULL);
//    if (mask & EV_ERR)
//      break;
//    ClearCommError(hSerial, &temp, &status);
//    btr = status.cbInQue;
//    if (btr >= len)
//    {
//      ReadFile(hSerial, data, len, &dwBytesRead, NULL);
//    }
//  }
  ReadFile(hSerial, data, len, &dwBytesRead, NULL);
  #endif
  #ifdef __linux

  // Variables
  uint16_t bytes_remaining;
  int byte_cnt=0;
  int ret;

  // give system some time to settle before starting a read
  //
  LOG_Print(LOG_LEVEL_INFO, "Trying to read %u bytes", len);

  // Copy len to remainting bytes to read var
  bytes_remaining = len;

  while (bytes_remaining) {

      // Try and read a single byte on the current position in the recieve buffer
      ret = read(fd, &data[byte_cnt], 1);

      // Check number of bytes that have been read(ret)
      if (ret <= 0) {
        LOG_Print(LOG_LEVEL_WARNING,"Read fail byte %u, return code: %u, read bytes sofar(%s)", byte_cnt, ret, array_to_str(buf, data, byte_cnt));
        return false;
      }
      if (ret>1){
        LOG_Print(LOG_LEVEL_WARNING,"Read more than one byte!?(%u)", ret);
      }

      bytes_remaining -=ret;
      byte_cnt += ret;
  }

  LOG_Print(LOG_LEVEL_INFO, "COM_Read(%s)", array_to_str(buf, data, byte_cnt));

  return byte_cnt;
  #endif
}

/** \brief Calculate time for transmission with current baudrate
 *
 * \param [in] len Length of transmitted data
 * \return time in milliseconds as uint16_t
 *
 */
uint16_t COM_GetTransTime(uint16_t len)
{
  return (uint16_t)(len * 1000 * 11 / COM_Baudrate + 1);
}

#ifdef __MINGW32__
void COM_WaitForTransmit(void)
{
  COMSTAT rStat;
  DWORD nErr;
  do {
    ClearCommError(hSerial, &nErr, &rStat);
  } while (rStat.cbOutQue > 0);
}
#endif // __MINGW32__

/** \brief Close current COM port
 *
 * \return Nothing
 *
 */
void COM_Close(void)
{
  //printf("Closing COM port\n");
  #ifdef __MINGW32__
  CloseHandle(hSerial);
  #endif
  #ifdef __linux
  close(fd);
  #endif
}
