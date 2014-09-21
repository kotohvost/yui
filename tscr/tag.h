/*
	$Id: tag.h,v 3.2.2.2 2007/07/24 11:28:12 shelton Exp $
*/

#ifndef _TAG_PROCESSOR_
#define _TAG_PROCESSOR_

#define TAG_NONE	0
#define TAG_START	1
#define TAG_NAME	2
#define TAG_PARAM_WAIT	3
#define TAG_PARAM_NAME	4
#define TAG_EQU_WAIT	5
#define TAG_VAL_WAIT	6
#define TAG_VAL_VALUE	7
#define TAG_FOUND	8

#define TAG_COMMENT_EOL		9
#define TAG_COMMENT_START	10
#define TAG_COMMENT_END		11

/* ���������� ������� "A-Z", "a-z" � "_", ����� 53 */
#define TAG_PROCESSOR_LEN	53

struct TagInfo
{
  char *name;			// ���������� ��� ����
  char type;			// ��� ����
  unsigned char param:1;	// ���� ����������� ���������� ��� ���� (� html)
  unsigned char hend:1;		// ���� ������� ������������ ���� (� html)
};

struct TagElement
{
  struct TagElement *chain;
  char type;			// ��� ����
  unsigned char start;		// ������ ������� �������� - ��� ����������� ������
  unsigned char end;		// ������ ���������� �������� - ��� ����������� ������
  unsigned char param:1;	// ���� ����������� ���������� ��� ���� (� html)
  unsigned char finish:1;	// ���� ����� ����
  unsigned char hend:1;		// ���� ������� ������������ ���� (� html)
  unsigned char name_len:5;	// ����� ����� ����, <= 32
};

class TagProcessor
{
protected:
	struct TagElement *first, *next, *prev;
	int base_chars, index, packed, start, end, prev_symbol;
	void freeChain( struct TagElement *chain, unsigned char start, unsigned char len );
	void packChain( struct TagElement *&chain, unsigned char start, unsigned char end );
public:
	int state;
	TagProcessor( int b_chars );
	virtual ~TagProcessor();
	void clear();
	void insertTag( TagInfo *tag );
	void pack();
	int empty() { return first ? 0 : 1; }
	virtual int checkTag( int ch, int &found_type, int &name_len );
	virtual int tagSymbol( int &ch );
	virtual void reset();
	virtual void reset_prev() { prev_symbol = 0; }
	virtual int startTag( int ch );
	virtual int endTag( int ch );
};

class HTMLTagProcessor : public TagProcessor
{
protected:
	int lock_value, flag_slash;
public:
	HTMLTagProcessor();
	int checkTag( int ch, int &found_type, int &name_len );
	int tagSymbol( int &ch );
	void reset();
	int startTag( int ch );
	int endTag( int ch );
};

/*
 * ���������� ������� � ������������ * ! # ; /
 */

class CommentTagProcessor : public TagProcessor
{
public:
	CommentTagProcessor();
	int checkTag( int ch, int &found_type, int &name_len );
	int tagSymbol( int &ch );
};

#endif /* _TAG_PROCESSOR_ */
