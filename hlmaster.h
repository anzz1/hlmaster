// hlmaster.h

#define MAX_SERVERS 8192
#define MAX_GAMES 128
#define MAX_SERVERS_PER_PACKET 231 // 1386 bytes + header = 1392 bytes
#define TIMEOUT_MILLIS (15 * 60 * 1000)

//

#pragma intrinsic(strcmp)

#ifdef _MSC_VER
  #pragma comment(lib, "ws2_32.lib")
#else
  #define __forceinline __inline__ __attribute__((always_inline))
#endif

#ifdef _WIN32
  #define WINVER 0x0501
  #define _WIN32_WINNT 0x0501
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
  #include <winsock2.h>
  WSADATA wsaData;

  __forceinline static unsigned long milliseconds(void) {
    return GetTickCount();
  }

  const unsigned long timeout = 1000;
  const unsigned long infinite = 0;

#else
  #include <time.h>
  #include <string.h>
  #include <netinet/in.h>
  #include <sys/socket.h>
  #include <unistd.h>
  #include <errno.h>
  #define closesocket close
  typedef int SOCKET;

  __forceinline static unsigned long milliseconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (1000 * ts.tv_sec + ts.tv_nsec / 1000000);
  }

  struct timeval timeout = { 1, 0 };
  struct timeval infinite = { 0, 0 };
#endif

#if !defined(_MSC_VER) && !defined(__clang__)
  __forceinline static void __stosb(
    unsigned char *Dest, const unsigned char Data, size_t Count) {
    __asm__ __volatile__("rep; stosb"
      : [Dest] "=D"(Dest), [Count] "=c"(Count)
      : "[Dest]"(Dest), "a"(Data), "[Count]"(Count));
  }
#endif

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;

#pragma pack(push, 1)

typedef struct peer_t {
  unsigned long ip;
  unsigned short port;
} peer_t;

#pragma pack(pop)

typedef struct server_t {
  peer_t peer;
  u32 t;
} server_t;

typedef struct game_t {
  u32 j_gamedir;
  server_t servers[MAX_SERVERS];
} game_t;

__forceinline static int __strncmp(const char* s1, const char* s2, unsigned int len) {
  unsigned int i;
  for (i = 0; i < len; i++) {
    if (s1[i] > s2[i]) return 1;
    else if (s2[i] > s1[i]) return -1;
    else if (s1[i] == 0) return 0;
  }
  return 0;
}

__forceinline unsigned long __inet_addr(const char* cp) // ipv4
{
  int i,x,y,z;
  const char* p;
  unsigned char ip[4];

  i = 0;
  x = 0;
  p = cp;
  while(p[i] && x < 5) {
    if (p[i] < 48 || p[i] > 57) {
      if (i < 1) return -1;
      if (p[i] != '.') return -1;
      for (y = 0, z = 0; y < i && p[y] != 0; y++) z = z * 10 + p[y] - 48;
      if (z > 255) return -1;
      ip[x] = z;
      x++;
      p += i+1;
      i = 0;
    }
    else
      i++;
  }
  if (x != 3 || *p == 0) return -1;

  z = 0;
  while (*p) {
    if (*p < 48 || *p > 57) return -1;
    z = z * 10 + *p - 48;
    p++;
  }

  if (z > 255) return -1;
  ip[x] = z;

  return *(unsigned long*)ip;
}

__forceinline static int __atoi(char* a)
{
  int i,x;
  for (i = 0, x = 0; a[i] > 47 && a[i] < 58; i++)
    x = x * 10 + a[i] - 48;
  return x;
}

unsigned long joaat_i(const unsigned char* s) {
  unsigned long hash = 0;
  while (*s) {
    hash += ((*s>64&&*s<91)?((*s++)+32):*s++); // A-Z -> a-z
    hash += (hash << 10);
    hash ^= (hash >> 6);
  }
  hash += (hash << 3);
  hash ^= (hash >> 11);
  hash += (hash << 15);
  return hash;
}

