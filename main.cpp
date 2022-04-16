#define WIN32_LEAN_AND_MEAN
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <windows.h>
#include <winsock2.h>
using namespace std;
#pragma comment(lib, "ws2_32.lib")

const BYTE VERSION_MAJOR = 1;
const BYTE VERSION_MINOR = 1;

#define CRLF "\r\n"

void ShowUsage(void)
{

  cout<<"Arguments were not set"<<endl;

  exit(1);
}

void Check(int iStatus, char *szFunction)
{
  if((iStatus != SOCKET_ERROR) && (iStatus))
    return;

  cerr << "Error during call to " << szFunction << ": " << iStatus << " - " << GetLastError() << endl;
}

int main(int argc, char *argv[])
{
  int         iProtocolPort        = 0;
  char        szSmtpServerName[64] = "";
  char        szToAddr[64]         = "";
  char        szFromAddr[64]       = "";
  char        szFromPassword[64]   = "";
  char        szBuffer[4096]       = "";
  char        szLine[255]          = "";
  char        szMsgLine[255]       = "";
  SOCKET      hServer;
  WSADATA     WSData;
  LPHOSTENT   lpHostEntry;
  LPSERVENT   lpServEntry;
  SOCKADDR_IN SockAddr;
  argc = 5;
  if(argc != 5)
    ShowUsage();

  lstrcpy(szSmtpServerName, argv[1]);
  lstrcpy(szToAddr, argv[2]);
  lstrcpy(szFromAddr, argv[3]);

  ifstream MsgFile(argv[4]);

  if(WSAStartup(MAKEWORD(VERSION_MAJOR, VERSION_MINOR), &WSData))
  {
    cout << "Cannot find Winsock v" << VERSION_MAJOR << "." << VERSION_MINOR << " or later!" << endl;

    return 1;
  }

  lpHostEntry = gethostbyname(szSmtpServerName);
  if(!lpHostEntry)
  {
    cout << "Cannot find SMTP mail server " << szSmtpServerName << endl;

    return 1;
  }

  hServer = socket(PF_INET, SOCK_STREAM, 0);
  if(hServer == INVALID_SOCKET)
  {
    cout << "Cannot open mail server socket" << endl;

    return 1;
  }

  lpServEntry = getservbyname("mail", 0);

  if(!lpServEntry)
    iProtocolPort = htons(IPPORT_SMTP);
  else
    iProtocolPort = lpServEntry->s_port;

  SockAddr.sin_family = AF_INET;
  SockAddr.sin_port   = iProtocolPort;
  SockAddr.sin_addr   = *((LPIN_ADDR)*lpHostEntry->h_addr_list);

  cout<<"Connection Established"<<endl;
  cout<<"Sending Mail from "<<szFromAddr<<" to  "<<szToAddr<<endl;

  if(connect(hServer, (PSOCKADDR) &SockAddr, sizeof(SockAddr)))
  {
    cout << "Error connecting to Server socket" << endl;

    return 1;
  }

  Check(recv(hServer, szBuffer, sizeof(szBuffer), 0), "recv() Reply");

  sprintf(szMsgLine, "HELO %s%s", szSmtpServerName, CRLF);
  Check(send(hServer, szMsgLine, strlen(szMsgLine), 0), "send() HELO");
  Check(recv(hServer, szBuffer, sizeof(szBuffer), 0), "recv() HELO");

  sprintf(szMsgLine, "MAIL FROM:<%s>%s", szFromAddr, CRLF);
  Check(send(hServer, szMsgLine, strlen(szMsgLine), 0), "send() MAIL FROM");
  Check(recv(hServer, szBuffer, sizeof(szBuffer), 0), "recv() MAIL FROM");

  sprintf(szMsgLine, "RCPT TO:<%s>%s", szToAddr, CRLF);
  Check(send(hServer, szMsgLine, strlen(szMsgLine), 0), "send() RCPT TO");
  Check(recv(hServer, szBuffer, sizeof(szBuffer), 0), "recv() RCPT TO");

  sprintf(szMsgLine, "DATA%s", CRLF);
  Check(send(hServer, szMsgLine, strlen(szMsgLine), 0), "send() DATA");
  Check(recv(hServer, szBuffer, sizeof(szBuffer), d0), "recv() DATA");

  MsgFile.getline(szLine, sizeof(szLine));

  do
  {
    sprintf(szMsgLine, "%s%s", szLine, CRLF);
    Check(send(hServer, szMsgLine, strlen(szMsgLine), 0), "send() message-line");
    MsgFile.getline(szLine, sizeof(szLine)); // get next line.
  } while(MsgFile.good());

  sprintf(szMsgLine, "%s.%s", CRLF, CRLF);
  Check(send(hServer, szMsgLine, strlen(szMsgLine), 0), "send() end-message");
  Check(recv(hServer, szBuffer, sizeof(szBuffer), 0), "recv() end-message");

  sprintf(szMsgLine, "QUIT%s", CRLF);
  Check(send(hServer, szMsgLine, strlen(szMsgLine), 0), "send() QUIT");
  Check(recv(hServer, szBuffer, sizeof(szBuffer), 0), "recv() QUIT");

  cout << "Sent " << argv[4] << " as email message to " << szToAddr << endl;

  closesocket(hServer);

  WSACleanup();

  return 0;
}
