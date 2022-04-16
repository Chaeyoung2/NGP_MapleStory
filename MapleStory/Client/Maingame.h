#pragma once

class CField;
class CMaingame
{
public:
	CMaingame() {};
	~CMaingame() { Release(); };

public:
	void Initialize(void);
	void Update(void);
	void Render(void);
	void Release(void);
	static int recvn(SOCKET, char*, int, int);
	thread recvThread;
public:
	void InitializeNetwork(void);
private:
	HDC		m_hDC;
private:
	template <typename T>
	static CObj* CreateSkill(MYPOINT pt, OBJECT_DIR dir)
	{
		CObj* pSkill = CAbstractFactory<T>::CreateObj(
			pt.x, pt.y);
		pSkill->SetAtt(100);
		pSkill->SetDir(dir);
		return pSkill;
	}

};
