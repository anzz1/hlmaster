// hlmaster.c

#include "hlmaster.h"

static game_t games[MAX_GAMES];
static u32 g_time = 0;

static void check_timedout(void)
{
  u32 t = seconds();
  if (g_time + TIMEOUT_SECONDS < t) {
    for (int i = 0; i < MAX_GAMES && games[i].gamedir[0]; i++) {
      for (int j = 0; j < MAX_SERVERS; j++) {
        if (games[i].servers[j].t > t || games[i].servers[j].t + TIMEOUT_SECONDS < t)
          games[i].servers[j].peer.ip = 0;
      }
    }
    g_time = t;
  }
}

static int del_server(peer_t* peer)
{
  for (int i = 0; i < MAX_GAMES && games[i].gamedir[0]; i++) {
    for (int j = 0; j < MAX_SERVERS; j++) {
      if (games[i].servers[j].peer.ip == peer->ip && games[i].servers[j].peer.port == peer->port) {
        games[i].servers[j].peer.ip = 0;
        return 1;
      }
    }
  }
  return 0;
}

static int add_server(const char* gamedir, peer_t* peer)
{
  server_t *server = 0;
  int i;
  u8 found = 0;
  for (i = 0; i < MAX_GAMES; i++) {
    if (!found && !games[i].gamedir[0]) {
      __strncpy(games[i].gamedir, gamedir, 31);
      games[i].gamedir[31] = 0;
      server = &games[i].servers[MAX_SERVERS-1];
      break;
    }
    else if (!strcmp(games[i].gamedir, gamedir)) {
      for (int j = 0; j < MAX_SERVERS; j++) {
        if (games[i].servers[j].peer.ip == peer->ip && games[i].servers[j].peer.port == peer->port) {
          games[i].servers[j].t = seconds();
          return 1;
        }
        else if (games[i].servers[j].peer.ip == 0) {
          server = &games[i].servers[j];
        }
      }
      found = 1;
    }
    else {
      for (int j = 0; j < MAX_SERVERS; j++) {
        if (games[i].servers[j].peer.ip == peer->ip && games[i].servers[j].peer.port == peer->port) {
          games[i].servers[j].peer.ip = 0;
          break;
        }
      }
    }
  }
  if (server) {
    server->peer.ip = peer->ip;
    server->peer.port = peer->port;
    server->t = seconds();
    return 1;
  }
  return 0;
}

static int parseChallengeResponse(peer_t* from, u8 *response)
{
  u8 *p = response;
  u8 *gamedir = 0;
  u8 good = 0;

  while (*p && *p != '\n') {
    if (!good && !__strncmp(p, "\\protocol\\", 10)) {
      p += 10;
      if (*(u16*)p != 0x3834 && *(u16*)p != 0x3734) // protocol 47/48
        return 0;
      good = 1;
      if (gamedir)
        break;
      p += 2;
    }
    if (!__strncmp(p, "\\gamedir\\", 9)) {
      p += 9;
      gamedir = p;
      while (*p && *p != '\\') p++;
      *p = 0;
      if (good)
        break;
    }
    p++;
  }
  if (good && gamedir)
    return add_server(gamedir, from);

  return 0;
}

static int parseListRequest(u8 *packet)
{
  const char* gamedir = 0;
  u8 *p = packet;
  p++; // '1'
  p++; // region
  u32 ip = 0;
  u16 port = 0;

  {
    u8* p2 = p;
    while (*p && *p != ':') p++;
    *p = 0;
    p++;

    ip = __inet_addr(p2);
    if (ip == -1) return 0;
    port = __atoi(p);
  }

  while (*p) p++;
  p++;
  while (*p) {
    if (!__strncmp(p, "\\gamedir\\", 9)) {
      p += 9;
      gamedir = p;
      while (*p && *p != '\\') p++;
      *p = 0;
      break;
    }
    p++;
  }

  *(u32*)packet = 0xFFFFFFFF;
  packet[4] = 0x66;
  packet[5] = 0x0A;

  peer_t *reply = (peer_t*)(&packet[6]);
  int i = 0, j = 0, k = 0, l = 0, m = MAX_GAMES;

  while (i < MAX_GAMES && games[i].gamedir[0]) i++;
  m = i;

  if (gamedir && *gamedir) {
    for (i = 0; i < m; i++) {
      if (!strcmp(gamedir, games[i].gamedir)) {
        k = i;
        m = i+1;
        break;
      }
    }
    if (i == m) {
      reply->ip = 0;
      reply->port = 0;
      return 6+sizeof(peer_t);
    }
  }

  if (ip && port) {
    for (; i < m; i++) {
      for (j = 0; j < MAX_SERVERS; j++) {
        if (games[i].servers[j].peer.ip == ip && games[i].servers[j].peer.port == port) {
          k = i;
          l = j + 1;
        }
      }
      if (l)
        break;
    }
  }

  i = 0;
  for (; k < m; k++) {
    for (; l < MAX_SERVERS; l++) {
      if (games[k].servers[l].peer.ip) {
        reply->ip = games[k].servers[l].peer.ip;
        reply->port = games[k].servers[l].peer.port;
        reply++;
        i++;
      }
      if (i == MAX_SERVERS_PER_PACKET) break;
    }
    if (i == MAX_SERVERS_PER_PACKET) break;
    l = 0;
  }

  if (i != MAX_SERVERS_PER_PACKET) {
    reply->ip = 0;
    reply->port = 0;
    i++;
  }

  return 6+i*sizeof(peer_t);
}

static int replyTo(SOCKET sd, struct sockaddr_in *to, u8 *packet, u32 len)
{
  int addrlen = sizeof(struct sockaddr);
  if (sendto(sd, (const char*)packet, len, 0, (struct sockaddr*)to, addrlen) == -1) {
    closesocket(sd);
    return 0;
  }
  return 1;
}

static int parsePacket(SOCKET sd, struct sockaddr_in* from, u8 *packet)
{
  if (packet[0] == 'q') { // hello
    *(u32*)packet = 0xFFFFFFFF;
    packet[4] = 0x73;
    packet[5] = 0x0A;
    *(u32*)((u8*)(packet+6)) = 1234567890;
    return replyTo(sd, from, packet, 10);
  }
  else if (packet[0] == 'b') { // bye
    peer_t peer;
    peer.ip = from->sin_addr.s_addr;
    peer.port = from->sin_port;
    del_server(&peer);
    return 1;
  }
  else if (packet [0] == '0' && packet[1] == '\n' && packet[2] == '\\') { // addserver
    peer_t peer;
    peer.ip = from->sin_addr.s_addr;
    peer.port = from->sin_port;
    return parseChallengeResponse(&peer, packet+2);
  }
  else if (packet[0] == 'i' || (*(u32*)packet == 0xFFFFFFFF && packet[5] == 'i')) { // ping
    *(u32*)packet = 0xFFFFFFFF;
    packet[4] = 0x6A;
    packet[5] = 0x00;
    return replyTo(sd, from, packet, 6);
  }
  else if (packet[0] == '1') { // list
    int len = parseListRequest(packet);
    if (len)
      return replyTo(sd, from, packet, len);
  }
  return 0;
}

int main()
{
  SOCKET sd;
  struct sockaddr_in server, client;
  unsigned char data[8192];
  int rcv, addrlen;

#ifdef _WIN32
  if (WSAStartup(MAKEWORD(2, 2), &wsaData)) return 1;
#endif

  __stosb((unsigned char*)&games, 0, sizeof(games));
  g_time = seconds();

  sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sd == -1) return 1;

  __stosb((unsigned char*)&server, 0, sizeof(server));
  server.sin_family = AF_INET;
  server.sin_port = 0x8269; // htons(27010)

  if (bind(sd, (struct sockaddr*)&server, sizeof(struct sockaddr)) == -1) {
    closesocket(sd);
    return 1;
  }

  setsockopt(sd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&dwTimeout, sizeof(dwTimeout));
  setsockopt(sd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&dwTimeout, sizeof(dwTimeout));
  setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&dwInfinite, sizeof(dwInfinite));
  setsockopt(sd, SOL_TCP, TCP_USER_TIMEOUT, (const char*)&dwTimeout, sizeof(dwTimeout));

  while (1) {
    __stosb((unsigned char*)&client, 0, sizeof(client));
    __stosb(data, 0, sizeof(data));
    addrlen = sizeof(struct sockaddr);
    rcv = recvfrom(sd, (char*)data, sizeof(data)-1, 0, (struct sockaddr*)&client, &addrlen);
    if (rcv == -1) {
      closesocket(sd);
      return 1;
    }

    if (rcv != 0)
      parsePacket(sd, &client, data);

    check_timedout();
  }

  return 0;
}
