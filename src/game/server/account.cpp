#include <cstdio>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include <base/system.h>

#include <engine/external/md5/md5.h>
#include <engine/shared/config.h>
#include <game/server/player.h>

#include "account.h"

#define MAX_FILEPATH 256

//#define D(F, A...) printf("%s:%s():%d - " F "\n",__FILE__, __func__,  __LINE__,##A)

const char* CAccount::ms_pPayloadHash = 0;

CAccount::CAccount(const char* pAccName)
{
	if (pAccName)
		str_copy(m_aAccName, pAccName, sizeof m_aAccName);
	else
		m_aAccName[0] = '\0';

	mem_zero(&m_Payload, sizeof m_Payload);
	m_Payload.m_Head.m_RegDate = m_Payload.m_Head.m_LastLoginDate = time_timestamp();// for new accs only
}

CAccount::~CAccount()
{
}

bool CAccount::SetPass(const char *pPass)
{
	char aHashedPass[33];
	if (!pPass || !*pPass)
		return false;

	HashPass(aHashedPass, pPass);

	str_copy(m_Payload.m_Head.m_aPassHash, aHashedPass, sizeof m_Payload.m_Head.m_aPassHash);

	return true;
}

bool CAccount::VerifyPass(const char* pPass) const
{
	if (!pPass || !*pPass)
		return false;

	char aBuf[33];
	HashPass(aBuf, pPass);

	return str_comp_num(aBuf, m_Payload.m_Head.m_aPassHash, (sizeof m_Payload.m_Head.m_aPassHash) - 1) == 0;
}

bool CAccount::Write() const
{
	// we always pad to have the size be a multiple of 32, to counter different
	// padding behaviour of different compilers.
	char aBodyChunk[31 + sizeof m_Payload.m_Body];
	bool Ret = false;
	if (!*m_aAccName || !*g_Config.m_SvAccDir)
		return false;

	// round up to the next full multiple of 32
	int SzBody = (31 + sizeof m_Payload.m_Body) & (~31);

	mem_zero(aBodyChunk, sizeof aBodyChunk);
	mem_copy(aBodyChunk, &m_Payload.m_Body, sizeof m_Payload.m_Body);

	struct flock fl = { 0, 0, 0, F_WRLCK, SEEK_SET };
	fl.l_pid = getpid();

	char aBuf[MAX_FILEPATH];
	str_format(aBuf, sizeof aBuf, "%s/%s_%s.acc", g_Config.m_SvAccDir, ms_pPayloadHash, m_aAccName);

	int fd = open(aBuf, O_WRONLY | O_CREAT | O_TRUNC, ~(S_IWGRP|S_IWOTH));

	if (fd == -1)
		return false;
	if (fcntl(fd, F_SETLKW, &fl) != -1)
	{
		if (write(fd, &m_Payload.m_Head, sizeof m_Payload.m_Head) == sizeof m_Payload.m_Head
				&& write(fd, aBodyChunk, SzBody) == SzBody)
			Ret = true;

		fl.l_type = F_UNLCK;
		fcntl(fd, F_SETLK, &fl);
	}

	close(fd);
	return Ret;
}

bool CAccount::Read()
{
	// we always pad to have the size be a multiple of 32, to counter different
	// padding behaviour of different compilers.
	char aBodyChunk[31 + sizeof m_Payload.m_Body];

	bool Ret = false;
	if (!*m_aAccName || !*g_Config.m_SvAccDir)
		return false;

	// round up to the next full multiple of 32
	int SzBody = (31 + sizeof m_Payload.m_Body) & (~31);

	mem_zero(aBodyChunk, sizeof aBodyChunk);

	struct flock fl = { 0, 0, 0, F_RDLCK, SEEK_SET };
	fl.l_pid = getpid();

	char aBuf[MAX_FILEPATH];
	str_format(aBuf, sizeof aBuf, "%s/%s_%s.acc", g_Config.m_SvAccDir, ms_pPayloadHash, m_aAccName);

	int fd = open(aBuf, O_RDONLY);

	if (fd == -1)
		return false;

	if (fcntl(fd, F_SETLKW, &fl) != -1)
	{
		if (read(fd, &m_Payload.m_Head, sizeof m_Payload.m_Head) == sizeof m_Payload.m_Head
				&& read(fd, aBodyChunk, SzBody) == SzBody)
		{
			Ret = true;
			mem_copy(&m_Payload.m_Body, aBodyChunk, sizeof m_Payload.m_Body);
		}

		fl.l_type = F_UNLCK;
		fcntl(fd, F_SETLK, &fl);
	}

	close(fd);
	return Ret;
}


void CAccount::Init(const char *pPayloadHash)
{
 	CAccount::ms_pPayloadHash = pPayloadHash;
	if (!*g_Config.m_SvAccDir)
		str_copy(g_Config.m_SvAccDir, ".", 2);

	if (!fs_is_dir(g_Config.m_SvAccDir) && (fs_makedir(g_Config.m_SvAccDir) != 0 || !fs_is_dir(g_Config.m_SvAccDir))) // sic
	{
		dbg_msg("acc", "failed to create account directory \"%s\", falling back to \"./\"", g_Config.m_SvAccDir);
		str_copy(g_Config.m_SvAccDir, ".", 2);
	}
}

void CAccount::HashPass(char *pDst, const char *pSrc)
{
	if (!pSrc)
		return;

	char aBuf[33];
	md5_state_t State;
	md5_byte_t aDigest[16];

	md5_init(&State);
	md5_append(&State, (const md5_byte_t *)pSrc, str_length(pSrc));
	md5_finish(&State, aDigest);

	for (int i = 0; i < 16; ++i)
		str_format(aBuf + 2*i, 3, "%02x", aDigest[i]);

	str_copy(pDst, aBuf, 33);
}

bool CAccount::IsValidAccName(const char *pSrc)
{
	if (!pSrc || !*pSrc)
		return false;

	while(*pSrc)
	{
		const char *pAC = g_Config.m_SvAccAllowedNameChars;
		bool Okay = false;
		while(*pAC)
			if (*pSrc == *(pAC++))
			{
				Okay = true;
				break;
			}

		if (!Okay)
			return false;

		pSrc++;
	}

	return true;
}

bool CAccount::ParseAccline(char *pDstName, unsigned int SzName, char *pDstPass, unsigned int SzPass, const char *pLine)
{
	if (!pDstName || !SzName || !pDstPass || !SzPass || !pLine || !*pLine)
		return false;

	pDstName[0] = pDstPass[0] = '\0';

	char aLine[128];
	str_copy(aLine, pLine, sizeof aLine);

	char *pWork = str_skip_whitespaces(aLine);

	if (*pWork)
	{
		char *pEnd = str_skip_to_whitespace(pWork);

		str_copy(pDstName, pWork, SzName);

		if ((unsigned int)(pEnd - pWork) < SzName)
			pDstName[pEnd - pWork] = '\0';

		pWork = str_skip_whitespaces(pEnd);

		if (*pWork)
		{
			str_copy(pDstPass, pWork, SzPass);
			pEnd = pDstPass + str_length(pDstPass) - 1;
			while(pEnd >= pDstPass && (*pEnd == ' ' || *pEnd == '\t' || *pEnd == '\n' || *pEnd == '\r'))
				*(pEnd--) = '\0';
		}
	}

	return *pDstName && *pDstPass;
}

void CAccount::OverrideName(char *pDst, unsigned SzDst, class CPlayer *pPlayer, const char *pWantedName)
{
	if (pPlayer->GetAccount() && *g_Config.m_SvAccMemberPrefix)
		str_copy(pDst, g_Config.m_SvAccMemberPrefix, SzDst);
	else if (!pPlayer->GetAccount() && *g_Config.m_SvAccGuestPrefix)
		str_copy(pDst, g_Config.m_SvAccGuestPrefix, SzDst);
	else
		pDst[0] = '\0';

	str_append(pDst, (g_Config.m_SvAccEnforceNames && pPlayer->GetAccount())?pPlayer->GetAccount()->m_aAccName:pWantedName, SzDst);
}
