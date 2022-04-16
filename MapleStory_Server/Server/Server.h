#pragma once
#include <winsock2.h>
#include <iostream>
#include <vector>
#include <time.h>
#include <mutex>
#include "../../MapleStory/Headers/Include.h"
using namespace std;

#pragma comment(lib, "Ws2_32.lib")


#define START_X 100.f
#define START_Y 496.f
#define START_HP 30000

void RecvThread(SOCKET client_sock);
void AIThread();
void MonsterThread();

void InitializeNetwork();

void InitializeMonsterInfo();

int recvn(SOCKET, char *, int, int);

void GreenMushRoom_MoveInPattern();

int GetEmptyID();

bool isAllConnected();

void EndNetwork();
