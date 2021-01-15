#pragma once

#include <string>
#include <iostream>

// 词法单元
class Token {
public:
	// 下面是词法单元的词类符号
	// 用大于 char (255) 的数值表示终结符号
	// 小于 255 ：用作  operator (运算符),  delimiter (界符)
	enum TAGS : unsigned {
		NUM = 256,		// 数值常量
		ID,				// 标识符
		REL_OPT,		// 关系运算符
		CHAR,			// 字符常量
		STRING,			// 字符串常量
		// 以下为关键字，均用大写表示
		// ***********在这里添加关键字之后也要在 lexer.cpp 的 Keywords 中添加相应项目***********
		KEYWORD_POS,	// 保留，标记关键字开始的数值
		TRUE,
		FALSE,
		IF,
		ELSE,
		THEN,
		WHILE,
		DO,
		INT,
		REAL,
		OR,
		AND,
	};

	Token(const int &t = 0) : tag(t) {}
	virtual ~Token() = default;
	virtual std::ostream& print(std::ostream& os) { os << "line" << line << ": <" << static_cast<char>(tag) << '>'; return os; }

	int tag;		// 词类编码
	size_t line;	// 每个词法单元出现的行数
};

// 数值常量
// 支持科学计数法
class Num : public Token {
public:
	Num() = default;
	Num(const double& v): Token(TAGS::NUM), value(v){}
	std::ostream& print(std::ostream& os) override { 
		os << "line" << line << ": <" << "NUM" << ',' << value << '>';
		return os;
	}
	std::string type;	// int / real 目前没用，所有常量数值都当作浮点数处理
	const double value;
};

// 标识符 & 关键字
class Word : public Token {
public:
	Word() = default;
	Word(const std::string& v, int t = TAGS::ID) : Token(t), lexeme(v) {}
	std::ostream& print(std::ostream& os) override {
		std::string t = (tag == TAGS::ID) ? "ID" : "KEY_WORD";
		os << "line" << line << ": <" << t << ',' << lexeme << '>';
		return os;
	}

	std::string lexeme;	// 词素
};

// 关系运算符 >= <= != ==
// > < 由于是单个符号，Tag 暂时先直接设为它们的 Ascii 码了
class RelOpt : public Token {
public:
	RelOpt() = default;
	RelOpt(const std::string& v, int t = TAGS::REL_OPT) : Token(t), lexeme(v) {}
	std::ostream& print(std::ostream& os) override {
		os << "line" << line << ": <" << "RELOPT" << ',' << lexeme << '>';
		return os;
	}

	const std::string lexeme;	// 词素
};

// 字符常量
class Char : public Token {
public:
	Char() = default;
	Char(const char& v) : Token(CHAR), value(v) {}
	std::ostream& print(std::ostream& os) override {
		os << "line" << line << ": <\'" << value << "\'>";
		return os;
	}

	char value;	// 字符常量值
};

// 字符串常量
class String : public Token {
public:
	String() = default;
	String(const std::string& v) : Token(STRING), value(v) {}
	std::ostream& print(std::ostream& os) override {
		os << "line" << line << ": <\"" << value << "\">";
		return os;
	}

	std::string value;	// 字符串常量值
};


