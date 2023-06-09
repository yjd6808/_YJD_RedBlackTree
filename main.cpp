﻿/*
 * 작성자: 윤정도
 * 생성일: 5/26/2023
 * =====================
 * 레드블랙트리를 구현해보자.
 * 학습결과 보고서: https://blog.naver.com/reversing_joa/223116951373
 */

#define DebugMode 1

#include <JCore/Core.h>

#include <JCore/Container/Vector.h>
#include <JCore/Container/HashMap.h>

#include <JCore/Primitives/StringUtil.h>
#include <JCore/Utils/Console.h>

USING_NS_JC;

enum class TreeNodeColor
{
	Red,
	Black
};

enum class TreeNodeRotateMode
{
	RR,
	LL,
	RL,
	LR
};

const char* TreeNodeColorName(TreeNodeColor color) {
	return color == TreeNodeColor::Red ? "Red" : "Black";
}

struct TreeNode
{
	int Data;
	TreeNodeColor Color;
	TreeNode* Parent;
	TreeNode* Left;
	TreeNode* Right;

	#pragma region PUBLIC FIELDS
	TreeNode(int data, TreeNodeColor color = TreeNodeColor::Red)
		: Data(data)
		, Color(color)
		, Parent(nullptr)
		, Left(nullptr)
		, Right(nullptr)
	{}

	// 둘중 할당된 자식 아무거나 반환
	TreeNode* Any() const { return Left ? Left : Right; }

	// 둥중 하나의 자식 아무거나 반환 및 자식이 몇개있는지도 같이 반환
	TreeNode* AnyWithChildrenCount(JCORE_OUT int& count) const {
		if (Left && Right) {
			count = 2;
			return Left;
		}
		if (Left) {
			count = 1;
			return Left;
		}
		if (Right) {
			count = 1;
			return Right;
		}
		count = 0;
		return nullptr;
	}
	bool IsLeft() const { return Parent->Left == this; }
	bool IsRight() const { return Parent->Right == this; }
	int Count() const {
		if (Left && Right) return 2;
		if (Left) return 1;
		if (Right) return 1;
		return 0;
	}
	#pragma endregion
	// PUBLIC FIELDS

	#pragma region PUBLIC FIELDS (DEBUG)
	static void DbgConnectLeft(TreeNode* parent, TreeNode* child) {
		DebugAssertMsg(parent->Left == nullptr, "부모(%d)의 좌측자식(%d)가 이미할당되어있음. %d 자식 연결불가능", parent->Data, parent->Left->Data, child->Data);
		DebugAssertMsg(child->Parent == nullptr, "자식(%d)의 부모(%d)가 이미할당되어있음. %d 부모 연결불가능", child->Data, child->Parent->Data, parent->Data);
		parent->Left = child;
		child->Parent = parent;
	}

	static void DbgConnectRight(TreeNode* parent, TreeNode* child) {
		DebugAssertMsg(parent->Right == nullptr, "부모(%d)의 우측자식(%d)가 이미할당되어있음. %d 자식 연결불가능", parent->Data, parent->Right->Data, child->Data);
		DebugAssertMsg(child->Parent == nullptr, "자식(%d)의 부모(%d)가 이미할당되어있음. %d 부모 연결불가능", child->Data, child->Parent->Data, parent->Data);
		parent->Right = child;
		child->Parent = parent;
	}
	#pragma endregion
	// PUBLIC FIELDS (DEBUG)
};


struct TreeNodeFamily
{
	/* Not Null */ TreeNode* Parent;
	/* Not Null */ TreeNode* Sibling;
	/* Nullable */ TreeNode* NephewLine;
	/* Nullable */ TreeNode* NephewTri;

	TreeNodeColor ParentColor;
	TreeNodeColor SiblingColor;
	TreeNodeColor NephewLineColor;
	TreeNodeColor NephewTriColor;

	#pragma region PUBLIC FIELDS
	TreeNodeFamily(TreeNode* child) {
		const bool bRightChild = child->IsRight();
		Parent = child->Parent;									// 부모 노드
		DebugAssertMsg(Parent, "부모노드 없을 수 없습니다.");

		Sibling = bRightChild ? Parent->Left : Parent->Right;	// 형제 노드 (child가 우측이면 부모의 왼쪽 노드가 형제 노드)
		DebugAssertMsg(Sibling, "형제노드가 없을 수 없습니다.");

		const TreeNode* pNephewLine = nullptr;					// 조카 노드 (일렬로 나열)		
		const TreeNode* pNephewTri = nullptr;					// 조카 노드 (꺽여서 나열)

		if (Sibling->IsLeft()) {
			NephewLine = Sibling->Left;
			NephewTri = Sibling->Right;
		}
		else {
			NephewLine = Sibling->Right;
			NephewTri = Sibling->Left;
		}

		// 노드가 없는 경우 Black으로 판정토록한다.
		ParentColor = Parent->Color;
		SiblingColor = Sibling->Color;
		NephewTriColor = NephewTri ? NephewTri->Color : TreeNodeColor::Black;
		NephewLineColor = NephewLine ? NephewLine->Color : TreeNodeColor::Black;
	}
	#pragma endregion
	// PUBLIC FIELDS

};

class TreeSet
{
public:
	#pragma region PUBLIC FIELDS
	TreeSet() : m_pRoot(nullptr) {}
	~TreeSet() { Clear(); }

	bool Search(int data) { return FindNode(data) != nullptr; }
	bool Insert(int data) {

		TreeNode* pNewNode;

		// 1. 데이터를 먼저 넣는다.
		if (m_pRoot == nullptr) {
			pNewNode = m_pRoot = dbg_new TreeNode{ data };
		}
		else {
			// data가 삽입될 부모 노드를 찾는다.
			TreeNode* pParent = FindParentDataInserted(data);

			if (pParent == nullptr) {
				return false;
			}

			pNewNode = dbg_new TreeNode{ data };
			pNewNode->Parent = pParent;

			if (data > pParent->Data) {
				pParent->Right = pNewNode;
			}
			else {
				pParent->Left = pNewNode;
			}
		}

		// 2. 삽입된 노드를 기준으로 레드블랙트리가 위반되는지 확인하여 바로잡는다.
		InsertFixup(pNewNode);
		return true;
	}

	void DeleteNode(TreeNode* node) {
		if (node == m_pRoot) {
			JCORE_DELETE_SAFE(m_pRoot);
			return;
		}

		if (node->Parent) {
			if (node->Parent->Left == node)
				node->Parent->Left = nullptr;
			else if (node->Parent->Right == node)	// 부유 상태의 node일 수 있으므로 무조건 체크
				node->Parent->Right = nullptr;
		}

		delete node;
	}

	void ConnectPredecessorChildToParent(TreeNode* predecessor, TreeNode* predecessorLeftChild) {

		if (predecessor->IsRight()) {
			predecessor->Parent->Right = predecessorLeftChild;
			predecessorLeftChild->Parent = predecessor->Parent;
			return;
		}

		predecessor->Parent->Left = predecessorLeftChild;
		predecessorLeftChild->Parent = predecessor->Parent;
	}

	bool Remove(int data) {
		TreeNode* pDelNode = FindNode(data);

		if (pDelNode == nullptr) {
			return false;
		}

		// 자식이 없는 경우 그냥 바로 제거 진행
		int iCount = 0;
		TreeNode* pChild = pDelNode->AnyWithChildrenCount(iCount);

		if (iCount == 2) {
			// 자식이 둘 다 있는 경우
			TreeNode* pPredecessor = FindBiggestNode(pDelNode->Left);

			// 전임자는 값을 복사해주고 전임자의 자식을 전임자의 부모와 다시 이어줘야한다.
			pDelNode->Data = pPredecessor->Data;

			if (pPredecessor->Left)
				ConnectPredecessorChildToParent(pPredecessor, pPredecessor->Left);

			// 전임자가 실제로 삭제될 노드이다.
			pDelNode = pPredecessor;


		}
		else if (iCount == 1) {
			// 자식이 한쪽만 있는 경우
			TreeNode* pParent = pDelNode->Parent;
			pChild->Parent = pParent;

			if (pDelNode->Color == TreeNodeColor::Red)
				JCORE_PASS;
			// 삭제되는 노드의 부모가 있을 경우, 삭제되는 노드의 자식과 부모를 올바른 위치로 연결해준다.
			if (pParent) {
				if (pParent->Left == pDelNode)
					pParent->Left = pChild;
				else
					pParent->Right = pChild;
			}
			else {
				// pDelNode의 부모가 없다는 말은
				//  => pDelNode = 루트라는 뜻이므로, 자식을 루트로 만들어준다.
				m_pRoot = pChild;
			}
		}

		TreeNode* n;
		if (pDelNode->Color == TreeNodeColor::Black && (n = pDelNode->Any()))
			JCORE_PASS;

		RemoveFixup(pDelNode);
		DeleteNode(pDelNode);
		return true;
	}

	void Clear() {
		DeleteNodeRecursive(m_pRoot);
		m_pRoot = nullptr;
	}

	int Count() {
		int iCount = 0;
		CountRecursive(m_pRoot, iCount);
		return iCount;
	}

	int GetMaxHeight() {
		int iMaxHeight = 0;
		GetMaxHeightRecursive(m_pRoot, 1, iMaxHeight);
		return iMaxHeight;
	}

	#pragma endregion
	// PUBLIC FIELDS

	#pragma region PUBLIC FIELDS (DEBUG)
	void DbgGenerateTreeWithString(String data) {
		Clear();
		data.Split(" ").ForEach([this](String& s) {
			int a = StringUtil::ToNumber<Int32>(s.Source());
			Insert(a);
		});
	}
	void DbgRemoveWithString(String data) {
		Console::WriteLine("데이터 갯수: %d", Count());
		DbgPrintHierarchical();
		data.Split(" ").ForEach([this, &data](String& s) {
			int a = StringUtil::ToNumber<Int32>(s.Source());
			Console::WriteLine("%d 삭제", a);
			DebugAssertMsg(Remove(a), "%d 노드 삭제 실패", a);
			Console::WriteLine("데이터 갯수: %d", Count());
			DbgPrintHierarchical();
		});
	}
	void DbgRoot(TreeNode* root) {
		DeleteNodeRecursive(m_pRoot);
		m_pRoot = root;
	}
	void DbgPrintHierarchical() {

		HashMap<int, Vector<TreeNode*>> hHierarchy;
		for (int i = 0; i < 200; ++i) {
			hHierarchy.Insert(i, Vector<TreeNode*>{});
		}
		RecordDataOnHierarchy(m_pRoot, 1, hHierarchy);
		static const char* Left = "L";
		static const char* Right = "R";
		static const char* None = "-";
		for (int i = 1; i < 200; ++i) {
			auto& nodes = hHierarchy[i];
			if (nodes.Size() <= 0) continue;
			Console::Write("[%d] ", i);
			for (int j = 0; j < nodes.Size(); ++j) {
				const char* l = nullptr;
				if (nodes[j]->Parent == nullptr) {
					l = None;
				}
				else {
					if (nodes[j]->Parent->Left == nodes[j])
						l = Left;
					else
						l = Right;
				}
				Console::Write("%d(%s, %d, %s) ",
					nodes[j]->Data,
					TreeNodeColorName(nodes[j]->Color),
					nodes[j]->Parent ? nodes[j]->Parent->Data : -1,
					l
				);
			}
			Console::WriteLine("");
		}
		Console::WriteLine("==============================");
	}
	#pragma endregion
	// DEBUG_METHODS (DEBUG)
private:

	#pragma region PRIVATE FIELDS
	TreeNode* FindNode(int data) {
		TreeNode* pCur = m_pRoot;

		while (pCur != nullptr) {
			if (data == pCur->Data) {
				return pCur;
			}

			if (data > pCur->Data) {
				pCur = pCur->Right;
			}
			else {
				pCur = pCur->Left;
			}
		}

		return nullptr;
	}

	static TreeNode* FindBiggestNode(TreeNode* cur) {
		while (cur != nullptr) {
			if (cur->Right == nullptr) {
				return cur;
			}

			cur = cur->Right;
		}

		return cur;
	}

	// data가 삽입될 부모를 찾는다.
	TreeNode* FindParentDataInserted(int data) {
		TreeNode* pParent = nullptr;
		TreeNode* pCur = m_pRoot;

		while (pCur != nullptr) {
			pParent = pCur;

			if (data > pCur->Data) {
				pCur = pCur->Right;
			}
			else {
				pCur = pCur->Left;
			}
		}

		return pParent;
	}


	// 삽입 위반 수정
	void InsertFixup(TreeNode* child) {

		// (1) 루트 노드는 Black이다.
		if (child == m_pRoot) {
			child->Color = TreeNodeColor::Black;
			return;
		}

		TreeNode* pParent = child->Parent;		// (1)에서 종료되지 않았다면 무조건 부모가 존재함.
		TreeNodeColor eParentColor = pParent->Color;

		/*  (2) Red 노드의 자식은 Black이어야한다.
		 *  만약 자식과 부모가 색상이 모두 빨간색이 아닌 경우 더이상 검사할 필요가 없다.
		 *  조상님이 없는 경우, 즉 pParent가 루트 노드인 경우
		 *  루트 노드는 무조건 Black이고 새로 삽입된 노드는 Red이므로 트리 높이가 2일때는 항상 RB트리의 모든 조건에 만족한다.
		 *   => 따라서 InsertFixup 수행시 아무것도 할게 없다.
		*
		 *     5    root = parent (black)          5         root = parent (black)
		 *   1	 ?	child (red)                  ?   10		 child (red)
		 *
		 */
		if (eParentColor != TreeNodeColor::Red || child->Color != TreeNodeColor::Red) {
			return;
		}

		// 노드 깊이(트리 높이)가 2인 경우는 모두 위 IF문에서 걸러지므로 이후로 GrandParent가 nullptr일 수 없다.
		TreeNode* pGrandParent = pParent->Parent;
		TreeNode* pUncle = nullptr;						// 삼촌 노드정보 (부모가 조상님의 왼쪽자식인 경우 조상님의 오른쪽 자식이 삼촌 노드)
		if (pGrandParent != nullptr) {
			if (pGrandParent->Left == pParent)
				pUncle = pGrandParent->Right;
			else
				pUncle = pGrandParent->Left;
		}
		DebugAssertMsg(pGrandParent, "그랜드 부모가 NULL입니다.");
		const TreeNodeColor eUncleColor = pUncle ? pUncle->Color : TreeNodeColor::Black; // 삼촌 노드는 있을 수도 없을 수도 있고. NIL 노드는 Black이다.


		/*
		 * Case 1: 삼촌 노드가 Black일 경우
		 *			Case 1-1
		 *			----------------------------------------------
		 *			       10(B)				<- grandparent
		 *			    5(R)	 ?(B)			<- parent, uncle
		 *			  1(R) ?					<- child
		 *
		 *			Case 1-2
		 *			----------------------------------------------
		 *			       10(B)				<- grandparent
		 *		       ?(B)   15(R)				<- uncle, parent
		 *                       21(R)			<- child
		 *
		 *
		 *		    Case 1-3 (삼각형 모양) - 5를 RR회전하여 Case 1-1의 모양으로 변환해줘야한다.
		 *			----------------------------------------------
		 *			       10(B)				<- grandparent
		 *			    5(R)	 ?(B)			<- parent, uncle
		 * 				   7(R) 				<- child
		 *				              ↓ 변환 후
		 *			       10(B)				<- grandparent
		 *			     7(R)	 ?(B)			<- child, uncle	==>
		 * 			  5(R) ?					<- parent
		 *
		 *		    Case 1-4 (삼각형 모양) - 5를 RR회전하여 Case 1-1의 모양으로 변환해줘야한다.
		 *			----------------------------------------------
		 *			       10(B)				<- grandparent
		 *			    ?(B)	 15(R)			<- parent, uncle
		 * 				      12(R) 			<- child
		 *				              ↓ 변환 후
		 *			       10(B)				<- grandparent
		 *			    ?(B)	12(R)			<- child, uncle
		 * 				            10(R) 		<- parent
		 *
		 *
		 */

		 // Case 1 
		if (eUncleColor == TreeNodeColor::Black) {
			if (pParent->IsLeft()) {
				if (child->IsLeft()) {
					// Case 1-1
					pGrandParent->Color = TreeNodeColor::Red;
					pParent->Color = TreeNodeColor::Black;
					RotateLL(pGrandParent);

					// 조상이 루트노드였다면 회전 후 부모가 루트노드로 올라오므로 변경해줘야함
					if (m_pRoot == pGrandParent) {
						m_pRoot = pParent;
					}

				}
				else {
					// Case 1-3
					RotateRR(pParent);
					InsertFixup(pParent);
				}
			}
			else {
				if (child->IsRight()) {
					// Case 1-2
					pGrandParent->Color = TreeNodeColor::Red;
					pParent->Color = TreeNodeColor::Black;
					RotateRR(pGrandParent);

					// 조상이 루트노드였다면 회전 후 부모가 루트노드로 올라오므로 변경해줘야함
					if (m_pRoot == pGrandParent) {
						m_pRoot = pParent;
					}
				}
				else {
					// Case 1-4
					RotateLL(pParent);
					InsertFixup(pParent);
				}
			}
			return;
		}


		/*
		 * Case 2: 삼촌 노드가 Red일 경우
		 *     이경우 Case1보다 훨씬 단순하다. 부모, 삼촌의 색상과 조상님의 색상을 바꿔줌으로써
		 *	   RB트리 속성 4번이 위배되지 않도록 만든다.
		 *	   그리고 조상님이 Red가 되었기 때문에 조상님의 부모가 마찬가지로 Red일 수가 있으므로
		 *	   조상님을 기준으로 다시 Fixup을 수행해주면 된다.
		 *
		 *			Case 1-1
		 *			----------------------------------------------
		 *			       10(B)				<- grandparent
		 *			    5(R)	 15(R)			<- parent, uncle
		 *			 1(R) 						<- child
		 *
		 *			Case 1-2
		 *			----------------------------------------------
		 *			       10(B)				<- grandparent
		 *		        5(R)   15(R)			<- uncle, parent
		 *                        21(R)			<- child
		 *
		*		    Case 1-3 (삼각형 모양)
		 *			----------------------------------------------
		 *			       10(B)				<- grandparent
		 *			    5(R)	 15(R)			<- parent, uncle
		 * 				   7(R) 				<- child
		 *
		 *		    Case 1-4 (삼각형 모양)
		 *			----------------------------------------------
		 *			       10(B)				<- grandparent
		 *			    5(R)	 15(R)			<- parent, uncle
		 * 				      12(R) 			<- child
		 *
		 * @참고: Uncle이 Red로 판정되었다는 말은 nullptr이 아니기도하다.
		 */



		pUncle->Color = TreeNodeColor::Black;
		pParent->Color = TreeNodeColor::Black;
		pGrandParent->Color = TreeNodeColor::Red;
		InsertFixup(pGrandParent);
	}

	// 삭제 위반 수정
	void RemoveFixup(TreeNode* child) {

		if (child->Color == TreeNodeColor::Red) {
			return;
		}

		// [1. 삭제될 노드가 자식이 1개 경우]
		TreeNode* pChild = child->Any();
		if (pChild) {
			// 케이스 1. 자식이 한개만 있는경우 (이 자식은 무조건 Red일 것이다.)
			DebugAssertMsg(child->Count() == 1, "1. 삭제될 노드에 자식이 1개만 있어야하는데 2개 있습니다.");
			DebugAssert(child->Color == TreeNodeColor::Black);
			DebugAssert(pChild->Color == TreeNodeColor::Red);
			pChild->Color = TreeNodeColor::Black;
			return;
		}

		if (child == m_pRoot) {
			return;
		}

		RemoveFixupExtraBlack(child);
	}

	// 엑스트라 Black 속성이 부여된 노드를 대상으로 위반 수정
	// 난 엑스트라 Black 속성이 이 함수에 들어온 것 자체로 부여되었다는 걸로 간주하기로 함.
	void RemoveFixupExtraBlack(TreeNode* child) {

		if (m_pRoot == child) {
			// 루트는 엑스트라 Black속성이 부여될 경우 없애기만 하면 됨.
			//	난 엑스트라 Black이라는 추가 정보를 굳이 노드에 담아서 표현할 필요 없다고 생각한다.
			//	삭제중 일시적으로 존재하는 속성이기 떄문이다.
			return;
		}

		const bool bRightChild = child->IsRight();
		const TreeNodeFamily family(child);


		// 그룹 케이스 2: 부모의 색이 Black인 경우
		if (family.ParentColor == TreeNodeColor::Black) {

			// 케이스 5. (형제가 Red인 경우)
			if (family.SiblingColor == TreeNodeColor::Red) {
				family.Parent->Color = TreeNodeColor::Red;
				family.Sibling->Color = TreeNodeColor::Black;
				RotateNode(family.Parent, bRightChild ? TreeNodeRotateMode::LL : TreeNodeRotateMode::RR);
				RemoveFixupExtraBlack(child);
				return;
			}

			// 케이스 1 ~ 4 (형제가 Black인 경우)
			if (family.NephewTriColor == TreeNodeColor::Black &&
				family.NephewLineColor == TreeNodeColor::Black) {
				// 케이스 1. 조카 모두 Black인 경우
				family.Sibling->Color = TreeNodeColor::Red;
				RemoveFixupExtraBlack(family.Parent);			// Extra Black을 없앨 수 없으므로 부모로 전달
				return;
			}

			if (family.NephewLineColor == TreeNodeColor::Red) {
				// 케이스 2. 라인조카가 Red인 경우
				family.NephewLine->Color = TreeNodeColor::Black;
				RotateNode(family.Parent, bRightChild ? TreeNodeRotateMode::LL : TreeNodeRotateMode::RR);
				return;
			}

			if (family.NephewTriColor == TreeNodeColor::Red) {
				// 케이스 3. 꺽인조카가 Red인 경우
				family.NephewTri->Color = TreeNodeColor::Black;
				family.Sibling->Color = TreeNodeColor::Red;
				RotateNode(family.Sibling, bRightChild ? TreeNodeRotateMode::RR : TreeNodeRotateMode::LL);
				RemoveFixupExtraBlack(child);	// 케이스 2로 처리하기위해 재호출
				return;
			}

			return;
		}

		DebugAssertMsg(family.SiblingColor == TreeNodeColor::Black, "[그룹 케이스 1] 형제노드가 Black이 아닙니다.");
		// 그룹 케이스 1: 부모의 색이 Red인 경우
		if (family.NephewTriColor == TreeNodeColor::Black &&
			family.NephewLineColor == TreeNodeColor::Black) {
			// 케이스 1. 조카 모두 Black인 경우
			family.Sibling->Color = TreeNodeColor::Red;
			family.Parent->Color = TreeNodeColor::Black;
			return;
		}

		if (family.NephewLineColor == TreeNodeColor::Red) {
			// 케이스 2. 라인조카가 Red인 경우

			family.NephewLine->Color = TreeNodeColor::Black;
			family.Sibling->Color = TreeNodeColor::Red;
			family.Parent->Color = TreeNodeColor::Black;
			RotateNode(family.Parent, bRightChild ? TreeNodeRotateMode::LL : TreeNodeRotateMode::RR);
			return;
		}

		if (family.NephewTriColor == TreeNodeColor::Red) {
			// 케이스 3. 꺽인조카가 Red인 경우
			family.NephewTri->Color = TreeNodeColor::Black;
			family.Sibling->Color = TreeNodeColor::Red;
			RotateNode(family.Sibling, bRightChild ? TreeNodeRotateMode::RR : TreeNodeRotateMode::LL);
			RemoveFixupExtraBlack(child); // 케이스 2로 처리하기위해 재호출
		}

	}

	void RotateNode(TreeNode* node, TreeNodeRotateMode mode) {
		switch (mode) {
		case TreeNodeRotateMode::RR: RotateRR(node); return;
		case TreeNodeRotateMode::LL: RotateLL(node); return;
		case TreeNodeRotateMode::RL: RotateRL(node); return;
		case TreeNodeRotateMode::LR: RotateLR(node); return;
		}
	}

	// 노드가 왼쪽/왼쪽으로 붙은 경우
	void RotateLL(TreeNode* node) {
		//        ?		- pParent
		//      5		- pCur
		//    3			- pChild
		//  1   ?		- pChildRight

		//      ?		- pParent
		//    3			- pChild
		//  1   5		- pCur
		//    ?			- pChildRight

		TreeNode* pParent = node->Parent;
		TreeNode* pCur = node;
		TreeNode* pChild = node->Left;
		TreeNode* pChildRight = node->Left->Right;

		if (pParent) {
			if (pParent->Left == pCur)
				pParent->Left = pChild;
			else
				pParent->Right = pChild;
		}
		pChild->Parent = pParent;

		pCur->Left = pChildRight;
		if (pChildRight)
			pChildRight->Parent = pCur;

		pChild->Right = pCur;
		pCur->Parent = pChild;

		// 회전으로 인한 루트 변경 업데이트
		if (m_pRoot == pCur) {
			m_pRoot = pChild;
		}
	}
	// 노드가 오른쪽/오른쪽으로 붙은 경우
	void RotateRR(TreeNode* node) {
		//  ?   		- ? : pParent
		//    1 		- 1 : pCur
		//      3		- 3 : pChild
		//    ?	  5  	- ? : pChildLeft
		// 
		//         ↓ 변환 후
		//  ?			- ? : pParent
		//    3			- 3 : pChild
		//  1   5		- 1 : pCur
		//   ?			- ? : pChildLeft

		TreeNode* pParent = node->Parent;
		TreeNode* pCur = node;
		TreeNode* pChild = node->Right;
		TreeNode* pChildLeft = node->Right->Left;

		if (pParent) {
			if (pParent->Left == pCur)
				pParent->Left = pChild;
			else
				pParent->Right = pChild;
		}
		pChild->Parent = pParent;


		pCur->Right = pChildLeft;
		if (pChildLeft)
			pChildLeft->Parent = pCur;

		pChild->Left = pCur;
		pCur->Parent = pChild;

		// 회전으로 인한 루트 변경 업데이트
		if (m_pRoot == pCur) {
			m_pRoot = pChild;
		}
	}
	void RotateLR(TreeNode* cur) {
		RotateRR(cur->Left);
		RotateLL(cur);
	}
	void RotateRL(TreeNode* cur) {
		RotateLL(cur->Right);
		RotateRR(cur);
	}

	static void RecordDataOnHierarchy(TreeNode* node, int depth, HashMap<int, Vector<TreeNode*>>& hierarchy) {
		if (node == nullptr) return;
		hierarchy[depth].PushBack(node);
		RecordDataOnHierarchy(node->Left, depth + 1, hierarchy);
		RecordDataOnHierarchy(node->Right, depth + 1, hierarchy);
	}
	static void DeleteNodeRecursive(TreeNode* node) {
		if (node == nullptr) return;
		DeleteNodeRecursive(node->Left);
		DeleteNodeRecursive(node->Right);
		delete node;
	}
	static void GetMaxHeightRecursive(TreeNode* node, int height, int& maxHeight) {
		if (node == nullptr) {
			maxHeight = Math::Max(maxHeight, height);
			return;
		}

		GetMaxHeightRecursive(node->Left, height + 1, maxHeight);
		GetMaxHeightRecursive(node->Right, height + 1, maxHeight);
	}
	static void CountRecursive(TreeNode* node, int& count) {
		if (node == nullptr) {
			return;
		}
		count++;
		CountRecursive(node->Left, count);
		CountRecursive(node->Right, count);
	}

	TreeNode* m_pRoot;

	#pragma endregion
	// PRIVATE FIELDS

};

int main() {
	Console::SetSize(800, 600);
	dbg_new char[] ("force leak");	// 일부러 남긴 릭

	{
		Console::WriteLine("기능 테스트");
		TreeSet set;
		for (int i = 0; i < 16; ++i) {
			set.Insert(i);
		}

		set.DbgPrintHierarchical();
		for (int i = 15; i >= 0; --i) {
			Console::WriteLine("%d 삭제");
			set.Remove(i);
			set.DbgPrintHierarchical();
		}
	}


	{
		Console::WriteLine("특정 케이스");
		TreeSet set;
		set.DbgGenerateTreeWithString("9 7 0 15 14 12 3 13 1 10 6 2 4 5 8 11");
		set.DbgRemoveWithString("10 15 14 0 1 9 6 4 11 13 5 3 8 2 12 7");
	}

	return 0;
}
