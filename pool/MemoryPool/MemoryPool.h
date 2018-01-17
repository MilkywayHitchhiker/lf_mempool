/*---------------------------------------------------------------

procademy MemoryPool.

�޸� Ǯ Ŭ����.
Ư�� ����Ÿ(����ü,Ŭ����,����)�� ������ �Ҵ� �� ��������.

- ����.

procademy::CMemoryPool<DATA> MemPool(300, FALSE);
DATA *pData = MemPool.Alloc();

pData ���

MemPool.Free(pData);


!.	���� ���� ���Ǿ� �ӵ��� ������ �� �޸𸮶�� �����ڿ���
Lock �÷��׸� �־� ����¡ ���Ϸ� ���縦 ���� �� �ִ�.
���� �߿��� ��찡 �ƴ��̻� ��� ����.



���ǻ��� :	�ܼ��� �޸� ������� ����Ͽ� �޸𸮸� �Ҵ��� �޸� ����� �����Ͽ� �ش�.
Ŭ������ ����ϴ� ��� Ŭ������ ������ ȣ�� �� Ŭ�������� �Ҵ��� ���� ���Ѵ�.
Ŭ������ �����Լ�, ��Ӱ��谡 ���� �̷����� �ʴ´�.
VirtualAlloc ���� �޸� �Ҵ� �� memset ���� �ʱ�ȭ�� �ϹǷ� Ŭ���������� ���� ����.


----------------------------------------------------------------*/
#ifndef  __MEMORYPOOL__H__
#define  __MEMORYPOOL__H__
#include <assert.h>
#include <new.h>


template <class DATA>
class CMemoryPool
{
private:

	/* **************************************************************** */
	// �� �� �տ� ���� ��� ����ü.
	/* **************************************************************** */
	struct st_BLOCK_NODE
	{
		st_BLOCK_NODE()
		{
			stpNextBlock = NULL;
		}
		st_BLOCK_NODE *stpNextBlock;
	};

	/* **************************************************************** */
	// ������ �޸� Ǯ�� ž ���
	/* **************************************************************** */
	struct st_TOP_NODE
	{
		st_BLOCK_NODE *pTopNode;
		__int64 iUniqueNum;
	};

public:

	//////////////////////////////////////////////////////////////////////////
	// ������, �ı���.
	//
	// Parameters:	(int) �ִ� �� ����.
	//				(bool) �޸� Lock �÷��� - �߿��ϰ� �ӵ��� �ʿ�� �Ѵٸ� Lock.
	// Return:
	//////////////////////////////////////////////////////////////////////////
	CMemoryPool(int iBlockNum, bool bLockFlag = false)
	{
		st_BLOCK_NODE *pNode, *pPreNode;

		////////////////////////////////////////////////////////////////
		// TOP ��� �Ҵ�
		////////////////////////////////////////////////////////////////
		_pTop = (st_TOP_NODE *)_aligned_malloc(sizeof(st_TOP_NODE), 16);
		_pTop->pTopNode = NULL;
		_pTop->iUniqueNum = 0;

		_iUniqueNum = 0;

		////////////////////////////////////////////////////////////////
		// �޸� Ǯ ũ�� ����
		////////////////////////////////////////////////////////////////
		m_iBlockCount = iBlockNum;
		if (iBlockNum < 0)	return;	// Dump

		else if (iBlockNum == 0)
		{
			m_bStoreFlag = true;
			_pTop->pTopNode = NULL;
		}

		////////////////////////////////////////////////////////////////
		// DATA * ũ�⸸ ŭ �޸� �Ҵ� �� BLOCK ����
		////////////////////////////////////////////////////////////////
		else
		{
			m_bStoreFlag = false;

			pNode = (st_BLOCK_NODE *)malloc(sizeof(DATA) + sizeof(st_BLOCK_NODE));
			_pTop->pTopNode = pNode;
			pPreNode = pNode;

			for (int iCnt = 1; iCnt < iBlockNum; iCnt++)
			{
				pNode = (st_BLOCK_NODE *)malloc(sizeof(DATA) + sizeof(st_BLOCK_NODE));
				pPreNode->stpNextBlock = pNode;
				pPreNode = pNode;
			}
		}
	}

	virtual	~CMemoryPool()
	{
		st_BLOCK_NODE *pNode;

		for (int iCnt = 0; iCnt < m_iBlockCount; iCnt++)
		{
			pNode = _pTop->pTopNode;
			_pTop->pTopNode = _pTop->pTopNode->stpNextBlock;
			free(pNode);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// �� �ϳ��� �Ҵ�޴´�.
	//
	// Parameters: ����.
	// Return: (DATA *) ����Ÿ �� ������.
	//////////////////////////////////////////////////////////////////////////
	DATA	*Alloc(bool bPlacementNew = true)
	{
		st_BLOCK_NODE *stpBlock;
		st_TOP_NODE pPreTopNode;
		int iBlockCount = m_iBlockCount;
		InterlockedIncrement64((LONG64 *)&m_iAllocCount);

		if (iBlockCount < m_iAllocCount)
		{
			if (m_bStoreFlag)
			{
				stpBlock = (st_BLOCK_NODE *)malloc(sizeof(DATA) + sizeof(st_BLOCK_NODE));
				stpBlock->stpNextBlock = NULL;
				InterlockedIncrement64((LONG64 *)&m_iBlockCount);
			}

			else
				return nullptr;
		}

		else
		{
			__int64 iUniqueNum = InterlockedIncrement64(&_iUniqueNum);

			do
			{
				pPreTopNode.iUniqueNum = _pTop->iUniqueNum;
				pPreTopNode.pTopNode = _pTop->pTopNode;

			} while (!InterlockedCompareExchange128((volatile LONG64 *)_pTop,
				iUniqueNum,
				(LONG64)_pTop->pTopNode->stpNextBlock,
				(LONG64 *)&pPreTopNode));

			stpBlock = pPreTopNode.pTopNode;
		}

		if (bPlacementNew)	new ((DATA *)(stpBlock + 1)) DATA;

		return (DATA *)(stpBlock + 1);
	}

	//////////////////////////////////////////////////////////////////////////
	// ������̴� ���� �����Ѵ�.
	//
	// Parameters: (DATA *) �� ������.
	// Return: (BOOL) TRUE, FALSE.
	//////////////////////////////////////////////////////////////////////////
	bool	Free(DATA *pData)
	{
		st_BLOCK_NODE *stpBlock;
		st_TOP_NODE pPreTopNode;

		__int64 iUniqueNum = InterlockedIncrement64(&_iUniqueNum);

		do {
			pPreTopNode.iUniqueNum = _pTop->iUniqueNum;
			pPreTopNode.pTopNode = _pTop->pTopNode;

			stpBlock = ((st_BLOCK_NODE *)pData - 1);
			stpBlock->stpNextBlock = _pTop->pTopNode;
		} while (!InterlockedCompareExchange128((volatile LONG64 *)_pTop, iUniqueNum, (LONG64)stpBlock, (LONG64 *)&pPreTopNode));

		InterlockedDecrement64((LONG64 *)&m_iAllocCount);
		return true;
	}


	//////////////////////////////////////////////////////////////////////////
	// ���� ������� �� ������ ��´�.
	//
	// Parameters: ����.
	// Return: (int) ������� �� ����.
	//////////////////////////////////////////////////////////////////////////
	int		GetAllocCount(void) { return m_iAllocCount; }

	int		GetBlockCount(void) { return m_iBlockCount; }
private:
	//////////////////////////////////////////////////////////////////////////
	// ��� ������ ž
	//////////////////////////////////////////////////////////////////////////
	st_TOP_NODE *_pTop;

	//////////////////////////////////////////////////////////////////////////
	// ž�� Unique Number
	//////////////////////////////////////////////////////////////////////////
	__int64 _iUniqueNum;

	//////////////////////////////////////////////////////////////////////////
	// �޸� Lock �÷���
	//////////////////////////////////////////////////////////////////////////
	bool m_bLockFlag;

	//////////////////////////////////////////////////////////////////////////
	// �޸� ���� �÷���, true�� ������ �����Ҵ� ��
	//////////////////////////////////////////////////////////////////////////
	bool m_bStoreFlag;

	//////////////////////////////////////////////////////////////////////////
	// ���� ������� �� ����
	//////////////////////////////////////////////////////////////////////////
	int m_iAllocCount;

	//////////////////////////////////////////////////////////////////////////
	// ��ü �� ����
	//////////////////////////////////////////////////////////////////////////
	int m_iBlockCount;
};

#endif