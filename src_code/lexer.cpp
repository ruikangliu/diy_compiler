#include "lexer.h"
#include <string>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <cstring>
#include <cmath>
#include <unordered_map>

using std::unordered_map;
using std::runtime_error;
using std::ifstream;
using std::string;
using std::vector;
using std::cout;
using std::endl;
using std::to_string;

// 这里的关键字条目 (词素) 要与 token.h 中的 Token::Tags (词类属性) 相对应 (顺序也必须一样)
static const vector<string> Keywords{
	"true",
	"false",
	"if",
	"else",
	"then",
	"while",
	"do",
	"int",
	"real",
	"or",
	"and",
};

Lexer::Lexer(std::string name) : in(name)
{
	// 将所有关键字存入字符串表
	int start = Token::TAGS::KEYWORD_POS + 1;	// 关键字开始的数值
	for (auto it = Keywords.begin(); it != Keywords.end(); ++it)
	{
		ID_table[*it] = start + static_cast<int>(it - Keywords.begin());
	}
}

Lexer::~Lexer() {
	// 由于使用的动态分配，记得最后要释放空间
	for (auto& token : tokens)
	{
		delete token;
	}
}

Token* Lexer::skip_blank(void)
{
	while (in && (peek == ' ' || peek == '\t' || peek == '\n'))
	{
		if (peek == '\n')
		{
			++line;
		}
		in >> peek;
	}
	in.unget();
	return nullptr;
}

Token* Lexer::handle_slash(void)
{
	char next_peek = in.peek();	
	if (in && next_peek == '/')	// 单行注释
	{
		in >> peek;		// 把 / 从流中读出
		while (in >> peek && peek != '\n')
		{
		}
		++line;		// 此时 peek 为换行符 或 源程序结束
	}
	else if (in && next_peek == '*')	// 多行注释
	{
		char prev_peek = ' ';
		in >> peek;		// 把 * 从流中读出
		while (in >> peek 
			&& !(prev_peek == '*' && peek == '/'))
		{
			prev_peek = peek;
			if (peek == '\n')
			{
				++line;
			}
		}
	}
	else {		// 除号
		return	new Token('/');	
	}
	return nullptr;
}

Token* Lexer::handle_relation_opterator(void)
{
	char next_peek = in.peek();
	if (in && next_peek == '=')	// >= <= == !=
	{
		string lexeme;
		lexeme.push_back(peek);
		in >> next_peek;
		lexeme.push_back(next_peek);
		return new RelOpt(lexeme);
	}
	else {
		// = < > ! Tag都直接当成其ASCII码了
		return new Token(peek);
	}
}
// 支持科学计数法
Token* Lexer::handle_digit(void)
{
	char next_peek = in.peek();
	auto to_digit = [](const char c) -> int { return (c - '0'); };
	double num = to_digit(peek);	// 数字常量的值
	double decimal = 0;					// 小数部分
	int decimal_bit = 0;				// 小数的位数

	// 把 e 之前的数字都读出来存到 string 里，然后用 stod(s) 转成 double 更方便些，但是不想改了
	while (in && isdigit(next_peek))
	{
		in >> peek;
		next_peek = in.peek();
		num = num * 10 + to_digit(peek);
	}
	if (in && next_peek == '.')
	{
		in >> peek;		// 读出小数点
		next_peek = in.peek();

		if (!in || !isdigit(next_peek))	// 小数点后必须要跟一位数字，不然就是拼写错误
		{
			string line_msg = "Line" + to_string(line) + ": ";
			throw runtime_error(line_msg + "No number after \'.\' !");
		}

		in >> peek;		// 读出小数点后的一位数
		next_peek = in.peek();
		decimal = to_digit(peek);
		decimal_bit = 1;
		
		while (in && isdigit(next_peek))
		{
			in >> peek;
			next_peek = in.peek();
			decimal = decimal * 10 + to_digit(peek);
			++decimal_bit;
		}
		decimal /= pow(10, decimal_bit);
		num += decimal;
	}
	if (in && next_peek == 'e')
	{
		int exp = 0;		// 指数
		int flag = 1;		// 指数符号
		in >> peek;			// 读入 e
		next_peek = in.peek();
		if (in && (next_peek == '+' || next_peek == '-'))
		{
			in >> peek;
			flag = (peek == '+') ? 1 : -1;
			next_peek = in.peek();
		}
		if (!isdigit(next_peek))	// e 后必须要跟一位数字，不然就是拼写错误
		{
			string line_msg = "Line" + to_string(line) + ": ";
			throw runtime_error(line_msg + "No number after \'e\' !");
		}
		while (isdigit(next_peek) && in >> peek)
		{
			next_peek = in.peek();
			exp = exp * 10 + to_digit(peek);
		}
		num = num * pow(10, exp);
	}

	return new Num(num);
}

Token* Lexer::handle_word(void)
{
	string lexeme;
	char next_peek;
	Token* token;

	lexeme.push_back(peek);
	next_peek = in.peek();

	while (in && (isalpha(next_peek) || isdigit(next_peek) || next_peek == '_'))
	{
		in >> peek;
		
		lexeme.push_back(peek);
		next_peek = in.peek();
	}

	auto it = ID_table.find(lexeme);
	if (it == ID_table.end())	// 该词素不在关键字表内
	{
		token = new Word(lexeme, Token::ID);
	}else{
		token = new Word(lexeme, it->second);
	}

	return token;
}

Token* Lexer::handle_char(void)
{
	Token* token = nullptr;
	if (in)
	{
		in >> peek;
		char next_peek = in.peek();
		if (next_peek != '\'')
		{
			string line_msg = "Line" + to_string(line) + ": ";
			throw runtime_error(line_msg + "Unmatched \'!");
		}
		token = new Char(peek);
		in >> peek;	// 读入右边的 '
	}
	return token;
}

Token* Lexer::handle_string(void)
{
	string tmp;
	while(in >> peek && peek != '\"')
	{
		tmp.push_back(peek);
		if (peek == '\n')
		{
			++line;
		}
	}
	if (!in && peek != '\"')
	{
		string line_msg = "Line" + to_string(line) + ": ";
		throw runtime_error(line_msg + "Unmatched \"!");
	}
	return new String(tmp);
}

Token* Lexer::scan(){
	Token *token;	// 识别出的词法单元
	if (!(in >> peek))
	{
		return nullptr;
	}
	char judger = peek;			// 放在 switch case 里做条件判断

	if (isdigit(judger))
	{
		judger = '0';			// 所有数字都转成 0
	}
	else if (isalpha(judger))
	{
		judger = 'a';			// 所有字母都转成 a
	}

	switch (judger)
	{
	case ' ': case '\t': case '\n':
		token = skip_blank();		// 跳过空白
		break;
	case '/':
		token = handle_slash();	// 处理注释和除号
		break;
	case '>': case '<': case '!': case '=':
		token = handle_relation_opterator();
		break;
	case '0':					// 处理数字
		token = handle_digit();
		break;
	case 'a': case '_':			// 处理 标识符 / 关键字 
		token = handle_word();
		break;
	case '\'':		// 处理字符常量
		token = handle_char();
		break;
	case '\"':		// 处理字符串常量
		token = handle_string();
		break;
	default:	// 其他单个的字符，组成词法单元
		token = new Token(peek);
		break;
	}
	if (token)
	{
		token->line = line;
	}
	
	return token;
}

Token* Lexer::gen_token()
{
	Token* t = nullptr;

	while (in && (t = this->scan()) == nullptr)
	{
	}
	// 读完 或 读出一个词法单元
	if (t)
	{
		tokens.push_back(t);
		return t;
	}
	return nullptr;
}

void Lexer::bind_new_src_file(string name)
{
	close_file();
	in.open(name);

	if (!in)
	{
		throw runtime_error("Wrong file name!\n");
	}
	in >> std::noskipws;	// 读入时不跳过源程序中的空白，因为要输出行数以及检查错误

	line = 1;
	peek = ' ';
	// 由于使用的动态分配，记得最后要释放空间
	// 由于使用的动态分配，记得最后要释放空间
	for (auto& token : tokens)
	{
		delete token;
	}
	tokens = vector<Token*>();

	// 将所有关键字存入字符串表
	ID_table = unordered_map<string, int>();
	// 将所有关键字存入字符串表
	int start = Token::TAGS::KEYWORD_POS + 1;	// 关键字开始的数值
	for (auto it = Keywords.begin(); it != Keywords.end(); ++it)
	{
		ID_table[*it] = start + static_cast<int>(it - Keywords.begin());
	}
}
