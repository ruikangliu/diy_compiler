#include "parser.h"
#include <string>
#include <iostream>
#include <vector>
#include <exception>

using std::cout;
using std::endl;
using std::string;
using std::vector;
using std::runtime_error;

// 将词法单元 Token 转为语法分析中的终结符
// 我目前定义的文法中有 id, num 是带属性的，这些符号要将相应属性传递给语义分析处理
Parser::sym_t Parser::token_to_grammer_sym(Token* token)
{	
	Parser::sym_t grammer_sym;
	
	if (token->tag < 255)	
	{
		// 是单个符号，这类词法单元直接返回词素即可
		// { } ; , = + - * / ! ( ) < >
		grammer_sym.push_back(static_cast<char>(token->tag));
	}
	else if (token->tag > Token::KEYWORD_POS)
	{
		// 是关键字，这类词法单元直接返回词素即可
		// int real if then else while do or and true false
		grammer_sym = dynamic_cast<Word*>(token)->lexeme;
	}
	else if (token->tag == Token::REL_OPT)
	{
		// 是关系运算符，这类词法单元直接返回词素即可
		// == !=  <= >= 
		grammer_sym = dynamic_cast<RelOpt*>(token)->lexeme;
	}
	else if (token->tag == Token::NUM)
	{
		// num 数字常量
		grammer_sym = "num";
	}
	else if (token->tag == Token::ID)
	{
		// id 标识符
		grammer_sym = "id";
	}

	return grammer_sym;
}

void Parser::parser_analyze(std::string new_file_name, bool verbose)
{
	Token* ptoken = nullptr;
	vector<Parser::sym_t> grammer_program;	// 将词法分析结果转存为语法分析器所需的格式
	
	lexer.bind_new_src_file(path_prefix + new_file_name);

	// 词法分析，输出结果存在 lexer.tokens 中
	while (ptoken = lexer.gen_token())
	{
		ptoken->print(cout) << endl;
		grammer_program.push_back(token_to_grammer_sym(ptoken));
	}
	grammer_program.push_back("#");	// 增加一个结束符号

	lr1.driver(grammer_program, lexer.tokens, verbose);

	cout << endl;
}


