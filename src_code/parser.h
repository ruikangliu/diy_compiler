#pragma once

#include "lexer.h"
#include "lr1.h"
#include <string>
#include <unordered_set>
#include <memory>


// 语法分析器
class Parser {
public:
	using sym_t = Production::sym_t;

	Parser(std::string pathprefix = "./", std::string code_file_name = "code.txt")
		: lexer(pathprefix + code_file_name), lr1(pathprefix + "grammer.txt"), path_prefix(pathprefix){}

	void parser_analyze(std::string new_file_name = "", bool verbose = true);	// 语法分析
private:
	sym_t token_to_grammer_sym(Token* token);

	Lexer lexer;	// 词法分析器
	LR1Processor lr1;	// LR1 分析器
	std::string path_prefix;	// 源文件路径前缀
};


