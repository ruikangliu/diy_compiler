#pragma once

#include "token.h"
#include <unordered_map>
#include <string>
#include <fstream>
#include <vector>

// 词法分析器
class Lexer {
public:
	Lexer(std::string name = ".\\code.txt");
	~Lexer();

	std::vector<Token*> tokens;		// 用于保存词法分析输出的词法单元

	Token* gen_token();
	int line = 1;				// 行数
	bool fail() { return in.eof(); }	// 是否读到文件尾
	void bind_new_src_file(std::string name = ".\\code.txt"); // 分析新的源程序
	void close_file() {
		if (in.is_open())
		{
			in.close();
		}
	}
private:
	char peek = ' ';			// 预读的一个字符
	std::unordered_map<std::string, int> ID_table;	// 字符串表
	std::ifstream in;			// 源文件

	// 功能函数
	Token* scan();
	Token* skip_blank(void);		// 跳过空白
	Token* handle_slash(void);		// 处理斜杠 /，可能使注释或除号
	Token* handle_relation_opterator(void);	// 处理 > < = !，可能是关系运算符，也可能是赋值运算符或取反运算符
	Token* handle_digit(void);		// 处理数字
	Token* handle_word(void);		// 处理单词 (标识符 或 关键字) 允许下划线或字母打头，后面跟下划线/数字/字母
	Token* handle_char(void);		// 处理字符常量
	Token* handle_string(void);		// 处理字符串常量
};


