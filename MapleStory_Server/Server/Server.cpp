#include "Server.h"

vector<SOCKET> g_vecsocket;
vector<PLAYERINFO> g_vecplayer;
vector<SKILLINFO> g_vecskill;
bool g_arrayconnected[MAX_USER]; // connected �迭 (id �ο� ����)

MONSTERPACKET g_monster_packet;

mutex m;

bool isEnd = false;

DWORD Monster_Old_Movetime;
DWORD Monster_Cur_Movetime;
DWORD test_old_time = GetTickCount();

void RecvThread(SOCKET client_sock)
{
	SOCKADDR_IN clientaddr;
	int addrlen = sizeof(clientaddr);
	getpeername(client_sock, (SOCKADDR *)&clientaddr, &addrlen);

	int retval{ 0 };
	PACKETINFO packetinfo;
	PLAYERINFO playerinfo;

	while (true) {
		char buf[BUFSIZE];
		ZeroMemory(&packetinfo, sizeof(packetinfo));
		retval = recvn(client_sock, buf, BUFSIZE, 0);
		if (retval == SOCKET_ERROR) {
			err_display("packetinfo recv()");
			break;
		}
		else
			memcpy(&packetinfo, buf, sizeof(packetinfo));

		switch (packetinfo.type) {
		case CS_PACKET_PLAYERINFO_INITIALLY: 
		{
			ZeroMemory(buf, sizeof(buf));	 
			retval = recvn(client_sock, buf, packetinfo.size, 0);
			if (retval == SOCKET_ERROR) {
				err_display("intial playerinfo recv()");
				break;
			}
			else {
				ZeroMemory(&playerinfo, sizeof(playerinfo));
				memcpy(&playerinfo, buf, sizeof(playerinfo));
			}

			// 2. playerinfo�� ������ ��� �������� �������� ä���ش�. (id, size, hp, mp, ..)
			// id �ο�
			int id = GetEmptyID();
			if (-1 == id) {
				cout << "user�� �� á���ϴ�." << std::endl;
				closesocket(client_sock);
				break;
			}
			playerinfo.id = id;
			playerinfo.connected = true;
			playerinfo.ready = false;
			playerinfo.pt.x = 100.f; // �ʱ� ��ǥ
			playerinfo.pt.y = 500.f;
			playerinfo.hp = 30000;
			playerinfo.size.cx = 100.f;
			playerinfo.size.cy = 100.f;
			playerinfo.attackAccValue = 0.f;
			playerinfo.money = 0;
			playerinfo.frame = { 0, 0, 0, 50 };
			playerinfo.state = PLAYER_STAND;
			playerinfo.prestate = PLAYER_STAND;


			// 3. ������ ��� �������� ä���� playerinfo�� ���Ϳ� push �Ѵ�.
			m.lock();
			g_vecplayer.push_back(playerinfo);
			m.unlock();

			// 4. ���� ������ Ŭ���̾�Ʈ���� "�� ������ �̰ž�!" ��� send �Ͽ� �˸���.
			ZeroMemory(&packetinfo, sizeof(packetinfo));
			packetinfo.id = playerinfo.id;
			packetinfo.size = sizeof(playerinfo);
			packetinfo.type = SC_PACKET_YOUR_PLAYERINFO;
			ZeroMemory(buf, sizeof(buf));
			memcpy(buf, &packetinfo, sizeof(packetinfo));
			retval = send(client_sock, buf, BUFSIZE, 0);
			if (retval == SOCKET_ERROR) {
				cout << ("send() ���� - ���� ���� - SC_PACKET_YOUR_PLAYERINFO") << endl;
				break;
			}
			ZeroMemory(buf, sizeof(buf));
			memcpy(buf, &(playerinfo), sizeof(playerinfo));
			retval = send(client_sock, buf, BUFSIZE, 0);
			if (retval == SOCKET_ERROR) {
				err_display("send() ���� - ���� ���� - SC_PACKET_YOUR_PLAYERINFO");
				break;
			}

			if (g_vecplayer.size() >= 2) {
				// 5. ������ ������ �ִ� Ŭ���̾�Ʈ���� �� Ŭ���̾�Ʈ�� playerinfo�� �����Ѵ�.
				ZeroMemory(&packetinfo, sizeof(packetinfo));
				packetinfo.type = SC_PACKET_NEW_OTHER_PLAYERINFO;
				packetinfo.size = sizeof(playerinfo);
				packetinfo.id = playerinfo.id;
				ZeroMemory(buf, sizeof(buf));
				memcpy(buf, &packetinfo, sizeof(packetinfo));

				int recvid;  // ���� Ŭ���̾�Ʈ�� id?
				if (playerinfo.id == 0) recvid = 1; else recvid = 0;
				retval = send(g_vecsocket[recvid], buf, BUFSIZE, 0);
				if (retval == SOCKET_ERROR) {
					err_display("send() - SC_PACKET_NEW_OTHER_PLAYERINFO");
					break;
				}
				ZeroMemory(buf, sizeof(buf)); 
				memcpy(buf, &playerinfo, sizeof(g_vecplayer[playerinfo.id]));
				retval = send(g_vecsocket[recvid], buf, BUFSIZE, 0);
				if (retval == SOCKET_ERROR) {
					err_display("send() - SC_PACKET_PLAYERINFO");
					break;
				}

			// 6. �� Ŭ���̾�Ʈ���� �̹� �����ϴ� Ŭ���̾�Ʈ�� ������ send �Ѵ�.
				// ���� �÷��̾ 2�� ���� �ش��Ѵ�. �� 1��° Ŭ���̾�Ʈ�� �ش��.
				PACKETINFO temppacketinfo = {};
				temppacketinfo.id = 0;// 1��° Ŭ���̾�Ʈ�� 0��° Ŭ���̾�Ʈ�� ������ �޾ƾ� �Ѵ�.
				temppacketinfo.size = sizeof(g_vecplayer[temppacketinfo.id]);
				temppacketinfo.type = SC_PACKET_NEW_OTHER_PLAYERINFO;
				ZeroMemory(buf, sizeof(buf));
				memcpy(buf, &temppacketinfo, sizeof(temppacketinfo));
				retval = send(g_vecsocket[1], buf, BUFSIZE, 0);
				if (retval == SOCKET_ERROR) {
					err_display("send() - SC_PACKET_NEW_OTHER_PLAYERINFO");
					break;
				}
				ZeroMemory(buf, sizeof(buf));
				memcpy(buf, &(g_vecplayer[temppacketinfo.id]), sizeof(g_vecplayer[temppacketinfo.id]));
				retval = send(g_vecsocket[1], buf, BUFSIZE, 0);
				if (retval == SOCKET_ERROR) {
					err_display("send() - SC_PACKET_NEW_OTHER_PLAYERINFO");
					break;
				}
			}


		}
		break;

		case CS_PACKET_PLAYERINFO_MOVE: // �÷��̾� �̵�
		{
			// -------------------Process---------------------
			// 1. ���� ���� ��Ŷ���� ��� Ŭ�� �ش�Ǵ��� id �˾ƿ�.
			int id = packetinfo.id;
			// 2. ���� ���� ��Ŷ���� playerinfo�� �޴´�.
			PLAYERINFO tempplayerinfo = {};
			ZeroMemory(buf, sizeof(buf));
			retval = recvn(client_sock, buf, BUFSIZE, 0);
			if (retval == SOCKET_ERROR) {
				cout << "���� - recvn - CS_PACKET_PLAYERINFO_MOVE" << endl;
				break;
			}
			else
				memcpy(&tempplayerinfo, buf, sizeof(tempplayerinfo));
			// 3. g_vecplayer[���� playerinfo�� id]�� �����Ͽ� ������ �����Ѵ�.
			m.lock();
			g_vecplayer[id] = tempplayerinfo;

			cout << id << "��° �÷��̾�(" << g_vecplayer[id].nickname << ") �̵�" << endl;

			// 4. �ٸ� Ŭ���̾�Ʈ���Ե� ������. (SC_PACKET_OTHER_PLAYERINFO)
			if (g_vecplayer.size() == 2) { // 2�� ���� �ÿ���.
				ZeroMemory(&packetinfo, sizeof(packetinfo));
				packetinfo.id = id;
				packetinfo.size = sizeof(PLAYERINFO);
				packetinfo.type = SC_PACKET_OTHER_PLAYERINFO;
				memcpy(buf, &packetinfo, sizeof(packetinfo));
				if (0 == id) {	// 0 ������ 1���� ������ �Ѵ�.
					retval = send(g_vecsocket[id + 1], buf, BUFSIZE, 0);
				}
				else if (1 == id) {
					retval = send(g_vecsocket[id - 1], buf, BUFSIZE, 0);
				}
				if (retval == SOCKET_ERROR) {
					cout << "failed : send - ���� - SC_PACKET_OTHER_PLAYERINFO" << endl;
					break;
				}
				ZeroMemory(buf, sizeof(buf));
				memcpy(buf, &(g_vecplayer[id]), sizeof(playerinfo));
				if (0 == id) {	// 0 ������ 1���� ������ �Ѵ�.
					retval = send(g_vecsocket[id + 1], buf, BUFSIZE, 0);
				}
				else if (1 == id) {
					retval = send(g_vecsocket[id - 1], buf, BUFSIZE, 0);
				}
				if (retval == SOCKET_ERROR) {
					cout << "failed : send - ���� - SC_PACKET_OTHER_PLAYERINFO" << endl;
					break;
				}
			}
			m.unlock();
		}
		break;

		case CS_PACKET_PLAYERINFO_INCHANGINGSCENE: // ���� �ٲ���� ��
		{
			int id = packetinfo.id;
			// ���� ���� ��Ŷ�� �޾ƿ´�.

			PACKETINFO packetinfo_send = { SC_PACKET_PLAYERINFO_INCHANGINGSCENE, sizeof(float), id };
			char buf_send_unch[BUFSIZE];
			memcpy(buf_send_unch, &packetinfo_send, sizeof(packetinfo_send));
			retval = send(g_vecsocket[id], buf_send_unch, BUFSIZE, 0);
			if (retval == SOCKET_ERROR) {
				cout << "failed : send - ���� - SC_PACKET_PLAYERINFO_INITIALLY" << endl;
				break;
			}
			float x = 100.f;

			char buf_send_ch[BUFSIZE];
			m.lock();
			g_vecplayer[id].pt.x = x;
			m.unlock();
			memcpy(buf_send_ch, &x, sizeof(x));
			retval = send(g_vecsocket[id], buf_send_ch, BUFSIZE, 0);
			if (retval == SOCKET_ERROR) {
				cout << "failed : send - ���� - SC_PACKET_PLAYERINFO_INITIALLY" << endl;
				break;
			}
		}
		break;

		case CS_PACKET_GRRENMUSH:
		{
			// -------------------Process---------------------
			int id = packetinfo.id;
			MONSTERINFO tempmonsterinfo = {};
			{
				ZeroMemory(buf, sizeof(buf));
				retval = recvn(client_sock, buf, BUFSIZE, 0);
				if (retval == SOCKET_ERROR) {
					cout << "���� - recvn - CS_PACKET_GRRENMUSH" << endl;
					break;
				}
				else
					memcpy(&tempmonsterinfo, buf, sizeof(tempmonsterinfo));
				{
					MONSTERINFO monsterinfo{};
					monsterinfo.id = id;
					monsterinfo.hp = g_monster_packet.green[id].hp;
					monsterinfo.money = g_monster_packet.green[id].money;
					monsterinfo.pt.x = g_monster_packet.green[id].pt.x;
					monsterinfo.pt.y = g_monster_packet.green[id].pt.y;
					monsterinfo.dir = g_monster_packet.green[id].dir;
					monsterinfo.state = g_monster_packet.green[id].state;


					ZeroMemory(&packetinfo, sizeof(packetinfo));
					packetinfo.id = id;
					packetinfo.size = sizeof(MONSTERINFO);
					packetinfo.type = SC_PACKET_GRRENMUSH;

					ZeroMemory(buf, sizeof(buf));
					memcpy(buf, &packetinfo, sizeof(packetinfo));
					retval = send(client_sock, buf, BUFSIZE, 0);
					if (retval == SOCKET_ERROR) {
						err_display("send() - SC_PACKET_GRRENMUSH_INITIALLY");
						break;
					}
					retval = send(client_sock, (char*)&g_monster_packet, sizeof(MONSTERPACKET), 0);

					if (retval == SOCKET_ERROR) {
						err_display("send()");
						break;
					}
				}
			}
		}
		break;

		case CS_PACKET_SKILL_CREATE: // ��ų ��� �� ����Ʈ ����
		{
			// 0. �ʿ��� ������
			SKILLINFO skillinfo;
			int skillid;
			// 1. ���� ������ ��ų�� skillinfo�� recv �� �´�.
			ZeroMemory(buf, sizeof(buf));
			retval = recvn(client_sock, buf, BUFSIZE, 0);
			if (retval == SOCKET_ERROR) {
				err_display("recvn() - CS_PACKET_SKILL_CREATE");
				break;
			}
			memcpy(&skillinfo, buf, sizeof(skillinfo));

			// 2. g_vecskill�� skillinfo�� push �Ѵ�
			m.lock();
			g_vecskill.push_back(skillinfo);
			m.unlock();

			// 3. ���� ������ ��ų���� �ο��� id�� ���Ѵ�.
			skillid = g_vecskill.size() - 1;

			// 4. �ٸ� Ŭ���̾�Ʈ���Ե� ��ų ���� �޽����� send �Ѵ�.
			if (g_vecplayer.size() >= 2) {
				// ���� �ٸ� Ŭ���̾�Ʈ�� id��?
				int recvid{ -1 };
				if (packetinfo.id == 0) 
					recvid = 1;
				else		
					recvid = 0;
				// ���� ����.
				ZeroMemory(&packetinfo, sizeof(PACKETINFO));
				packetinfo.id = /*skillid*/packetinfo.id;
				packetinfo.size = sizeof(packetinfo);
				packetinfo.type = SC_PACKET_SKILL_CREATE;
				ZeroMemory(buf, sizeof(buf));
				memcpy(buf, &packetinfo, sizeof(packetinfo));
				retval = send(g_vecsocket[recvid], buf, BUFSIZE, 0);
				if (retval == SOCKET_ERROR) {
					err_display("send() - SC_PACKET_SKILL_CREATE");
					break;
				}
				// ���� ����.
				ZeroMemory(buf, sizeof(buf));
				memcpy(buf, &skillinfo, sizeof(skillinfo));
				retval = send(g_vecsocket[recvid], buf, BUFSIZE, 0);
				if (retval == SOCKET_ERROR) {
					err_display("send() - SC_PACKET_SKILL_CREATE");
					break;
				}
			}
		}
		break;

		case CS_PACKET_CLIENT_END: // Ŭ���̾�Ʈ ���� ó��
		{
			isEnd = true;
			closesocket(client_sock);
			cout << "[Ŭ���̾�Ʈ ���� ����] IP �ּ� (" << inet_ntoa(clientaddr.sin_addr) << "), ��Ʈ ��ȣ (" << ntohs(clientaddr.sin_port) << ")" << endl;
		}
		break;
		}
	}

	closesocket(client_sock);
	cout << "[Ŭ���̾�Ʈ ���� ����] IP �ּ� (" << inet_ntoa(clientaddr.sin_addr) << "), ��Ʈ ��ȣ (" << ntohs(clientaddr.sin_port) << ")" << endl;

}

void AIThread()
{
	Monster_Old_Movetime = GetTickCount();
	Monster_Cur_Movetime;
	while (1)
	{
		Monster_Cur_Movetime = GetTickCount();
		if (Monster_Cur_Movetime - Monster_Old_Movetime >= 30)
		{
			GreenMushRoom_MoveInPattern();
			Monster_Old_Movetime = GetTickCount();
		}
	}
}

void MonsterThread()
{
	PACKETINFO packetinfo;
	int retval{ 0 };
	char buf[BUFSIZE];
	DWORD test_cur_time;

	while (1)
	{
		if (isEnd)	// Ŭ���̾�Ʈ ������ ����Ǹ� ���� ���ϵ� ����
		{
			cout << "[Ŭ���̾�Ʈ ���� ����: ���� ������ ���� ����]" << endl;
			break;
		}

		test_cur_time = GetTickCount();
		if (test_cur_time - test_old_time > 100)
		{
			if (g_vecplayer.size() >= 1 && g_vecsocket.size() >= 1)
			{
				//cout << "g_Vec������:" << g_vecsocket.size() << endl;

				ZeroMemory(&packetinfo, sizeof(packetinfo));
				packetinfo.id = 0;
				packetinfo.size = sizeof(MONSTERINFO);
				packetinfo.type = SC_PACKET_GRRENMUSH;

				//���� ������ ������ 
				ZeroMemory(buf, sizeof(buf));
				memcpy(buf, &packetinfo, sizeof(packetinfo));
				retval = send(g_vecsocket[0], buf, BUFSIZE, 0);
				if (retval == SOCKET_ERROR) {
					err_display("send() - SC_PACKET_GRRENMUSH");
					break;
				}

				// ���������� ������ 					
				{
					retval = send(g_vecsocket[0], (char*)&g_monster_packet, sizeof(MONSTERPACKET), 0);
				}
				if (g_vecplayer.size() >= 2 && g_vecsocket.size() >= 2)
				{
					retval = send(g_vecsocket[1], buf, BUFSIZE, 0);
					retval = send(g_vecsocket[1], (char*)&g_monster_packet, sizeof(MONSTERPACKET), 0);
				}
				if (retval == SOCKET_ERROR) {
					err_display("send()");
					break;
				}
			}
			test_old_time = GetTickCount();
		}
	}

	m.lock();
	closesocket(g_vecsocket[0]);
	if (g_vecplayer.size() >= 2 && g_vecsocket.size() >= 2)
		closesocket(g_vecsocket[1]);
	m.unlock();

	if (!isEnd)
		cout << "[Ŭ���̾�Ʈ ���� ����: ���� ������ ���� ����]" << endl;

}


int main()
{
	// ���� �ʱ�ȭ
	InitializeNetwork();

	g_vecplayer.reserve(MAX_USER);

	// ���� ��ǥ �ʱ�ȭ
	InitializeMonsterInfo();

	int retval;

	// socket()
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET)
		err_quit("socket()");

	// �ּ� ����ü ����
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);

	// bind()
	retval = bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR)
		err_quit("bind()");

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR)
		err_quit("listen()");

	// ������ ��ſ� ����� ����
	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen;

	thread aiThread{ AIThread };
	thread monThread{ MonsterThread };
	thread clientThreads[MAX_USER] = {};

	while (true) {
		// accept()
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (SOCKADDR*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			err_display("accept()");
			break;
		}

		// ���� �������� Ȯ�� �� Ŭ���̾�Ʈ ������ ����
		int id = GetEmptyID();
		if(id != -1)
			clientThreads[id] = thread{ RecvThread, client_sock };

		// ������ Ŭ���̾�Ʈ socket�� �迭�� ��´�. (1201)
		g_vecsocket.push_back(client_sock);
		cout << "����:" << g_vecsocket.size() << endl;
		cout << "[Ŭ���̾�Ʈ ����] IP �ּ� (" << inet_ntoa(clientaddr.sin_addr) <<
			"), ��Ʈ ��ȣ (" << ntohs(clientaddr.sin_port) << ")" << endl;
		
	}
	// ������ ���� ���
	for (int i = 0; i < MAX_USER; ++i) {
		clientThreads[i].join();
	}
	aiThread.join();
	monThread.join();

	// closesocket()
	closesocket(listen_sock);

	EndNetwork();

	return 0;
}

void InitializeNetwork()
{
	// ���� �ʱ�ȭ.
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		err_quit("WSAStartup()");
}

void InitializeMonsterInfo()
{
	int greenposX[] = { HENESISCX * 0.5f,HENESISCX * 0.7f , HENESISCX * 0.6f ,HENESISCX * 0.7f,HENESISCX * 0.5f, HENESISCX * 0.4f,
		HENESISCX * 0.53f,HENESISCX * 0.68f , HENESISCX * 0.6f ,HENESISCX * 0.72f,HENESISCX * 0.55f, HENESISCX * 0.41f };
	int greenposY[] = { HENESISCY - 460.f,HENESISCY - 460.f, HENESISCY - 460.f, HENESISCY - 460.f, HENESISCY - 460.f, HENESISCY - 460.f, 
		HENESISCY - 220.f, HENESISCY - 220.f, HENESISCY - 220.f, HENESISCY - 220.f, HENESISCY - 220.f, HENESISCY - 220.f, };
	OBJECT_DIR greenDir[] = { DIR_LEFT, DIR_RIGHT ,DIR_RIGHT, DIR_LEFT, DIR_RIGHT, DIR_RIGHT,
		DIR_RIGHT, DIR_LEFT ,DIR_RIGHT, DIR_LEFT, DIR_RIGHT, DIR_LEFT };
	int greenPtr[] = { 1,1,3,1,1,3,1,1,3,1,1,3 };

	srand(time(NULL));

	// �ʷϹ��� �ʱ�ȭ
	for (int i = 0; i < MAX_GREEN; ++i)
	{
		MONSTERINFO monsterinfo{};
		monsterinfo.id = i;
		monsterinfo.hp = 100;
		monsterinfo.money = rand() % 1000 + 1;	// ������ �� �ο�
		monsterinfo.pt.x = greenposX[i];
		monsterinfo.pt.y = greenposY[i];
		monsterinfo.dir = greenDir[i];
		monsterinfo.state = MONSTER_WALK;

		g_monster_packet.green[i] = monsterinfo;
	}
}

void GreenMushRoom_MoveInPattern()
{
	float m_fSpeed = 0.3f;

	for (int i = 0; i < MAX_GREEN; ++i)
	{

		if (DIR_LEFT == g_monster_packet.green[i].dir)
		{
			switch (g_monster_packet.green[i].state)
			{
			case MONSTER_WALK:
			{
				g_monster_packet.green[i].pt.x -= m_fSpeed;

				if (g_monster_packet.green[i].pt.x < 465.f)
					g_monster_packet.green[i].dir = DIR_RIGHT;
			}
			break;
			case MONSTER_STAND:
				break;
			}

		}
		else
		{
			switch (g_monster_packet.green[i].state)
			{
			case MONSTER_WALK:
			{
				g_monster_packet.green[i].pt.x += m_fSpeed;
				if (g_monster_packet.green[i].pt.x > 1390.f)
					g_monster_packet.green[i].dir = DIR_LEFT;
			}
			break;
			case MONSTER_STAND:
				break;
			}
		}
	}
}

int GetEmptyID()
{
	int id = 0;
	for (;  id < g_vecplayer.size(); ++id) {
		if (g_vecplayer[id].connected == true) continue;
		if (id == MAX_USER - 1) return -1;
	}
	return id;
}


void EndNetwork()
{
	// ���� ����
	WSACleanup();
}

int recvn(SOCKET s, char *buf, int len, int flags)
{
	int received;
	char *ptr = buf;
	int left = len;

	while (left > 0) {
		received = recv(s, ptr, left, flags);

		if (received == SOCKET_ERROR)
			return SOCKET_ERROR;

		else if (received == 0)
			break;

		left -= received;
		ptr += received;
	}
	return len - left;
}