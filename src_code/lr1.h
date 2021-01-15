#pragma once

#include "token.h"
#include "symbols.h"
#include <string>
#include <fstream>
#include <vector>
#include <iostream>
#include <tuple>
#include <unordered_set>
#include <unordered_map>
#include <list>
#include <stack>
#include <queue>

// 四元式
struct Quad {
	friend std::ostream& operator<< (std::ostream& os, const Quad& q);

	Quad() = default;
	Quad(std::string o, std::string l, std::string r, std::string d)
		: opt(o), lhs(l), rhs(r), dest(d) {}

	std::string opt;
	std::string lhs;
	std::string rhs;
	std::string dest;
};

// 定义一些语法单元的属性
// 因为定义的源语言很简单，属性很少，为了省事我就直接把所有属性都定义到一个类里了
struct Attribute {
	using chain_t = std::unordered_set<size_t>;	// 用于回填的四元式地址链

	Attribute() = default;
	~Attribute() = default;

	// 以下为各种属性
	chain_t chain;	// 和拉链回填有关的属性
	chain_t TC;
	chain_t FC;
	size_t quad;

	Id::Type type;		// 变量类型
	double value;			// 变量或表达式的值
	std::string place;		// 临时变量名

	std::string lexeme;	// 词素

	size_t line;	// 语法单元所在行

	// 回填
	void backpatch(chain_t Attribute::* pchain, size_t n, std::vector<Quad> & quads)
	{
		for (auto& i : this->*pchain)
		{
			quads[i].dest = std::to_string(n);
		}
	}
	void merge(chain_t Attribute::*pchain, const chain_t& chain1, const chain_t& chain2)
	{
		(this->*pchain).insert(chain1.begin(), chain1.end());
		(this->*pchain).insert(chain2.begin(), chain2.end());
	}
};


// 产生式
class Production {
	friend std::ostream& operator<<(std::ostream& out, Production& p);
public:
	using sym_t = std::string;

	Production() = default;
	Production(const sym_t&l, const std::vector<sym_t> &r) : left(l), right(r) {}
	virtual ~Production() {};
	static const sym_t epsilon;	// 产生式中的空规则

	size_t len() const { return right.size(); }	// 返回产生式右部的长度

	std::vector<sym_t> get_right() const{ return right; }	// 返回产生式右部
	sym_t get_left() const { return left; }	// 返回产生式左部
	bool input_production(const std::string& p);
private:
	sym_t left = sym_t();	// 产生式左部
	std::vector<sym_t> right;	// 产生式右部
};

// 文法
class Grammer {
	friend class LR1Processor;
	friend class LR1DFA;
	friend std::ostream& operator<<(std::ostream& out, Grammer& g);
	friend std::istream& operator>>(std::istream& in, Grammer& g);
public:
	using sym_t = Production::sym_t;
	static const sym_t end_of_sentence;		// 句子结尾

	Grammer() : T{ Grammer::end_of_sentence, Production::epsilon } {}	// '#' 表示句子结尾；'$' 表示空规则
	~Grammer() = default;

	std::ostream& output_first_set(std::ostream& os) const;	// 输出求得的 FIRST 集
	std::unordered_set<sym_t> first_set_string(const std::vector<sym_t>& s) const;	// 求一串符号的 FIRST 集
	
private:
	std::vector<Production> productions;	// 产生式
	std::unordered_set<sym_t> T;	// 终结符
	std::unordered_set<sym_t> N;	// 非终结符
	std::vector<sym_t> TN;	// 所有符号 (终结符+非终结符)
	std::unordered_map<sym_t, std::unordered_set<sym_t>> first;	// 终结符和非终结符的 FIRST 集

	// 判断某个产生式第一个符号是否为终结符
	bool first_is_T(const Production&p) const {
		return !(T.find(p.get_right()[0]) == T.end());
	}	
	bool first_is_T(const sym_t& c) const {
		return !(T.find(c) == T.end());
	}
	void first_set();	// 对文法中的所有终结符和非终结符求 FIRST 集
};

// LR1 项目
// hashable
class LR1Item : public Production {
	friend class LR1DFA;
	friend class LR1Processor;
	friend bool operator==(const LR1Item &item1, const LR1Item &item2);
	friend struct std::hash<LR1Item>;
public:
	LR1Item() = default;
	LR1Item(const Production& p, const sym_t& n, const size_t& o) : Production(p), next(n), loc(o) {}
	LR1Item(const sym_t& l, const std::vector<sym_t>& r, const sym_t& n, const size_t &o) : Production(l, r), next(n), loc(o){}
private:
	sym_t next;	// 向前看符号
	size_t loc;	// 项目中点的位置
};

namespace std {
	template<>
	struct hash<LR1Item>	// 特例化 LR1 项目的 hash 类
	{
		using result_type = size_t;
		using argument_type = LR1Item;
		size_t operator()(const LR1Item& item) const
		{
			auto right = item.get_right();
			size_t hash_res = hash<LR1Item::sym_t>()(item.next) ^
								hash<size_t>()(item.loc) ^
								hash< LR1Item::sym_t>()(item.get_left());

			for (const auto& sym : right)
			{
				hash_res ^= hash<LR1Item::sym_t>()(sym);
			}

			return  hash_res;
		}
	};
}

class LR1DFA {
	friend class LR1Processor;
public:
	using sym_t = Production::sym_t;
	using LR1_items_t = std::unordered_set<LR1Item>;		// LR1 项目集 hashable
	
	LR1DFA(const Grammer& g = Grammer()) : grammer(g){}
	std::ostream& print_lr1_items(const LR1_items_t& items, std::ostream& os) const;
private:
	LR1_items_t closure(const LR1_items_t& items);	// 对 LR1 项集族求闭包
	LR1_items_t go_to(const LR1_items_t& items, sym_t c);	// GOTO 函数，由 items 项集读入 c 之后到达另一个项集，如果不在已求出的项集族中，就添加一下
	Grammer grammer;
};

namespace std {
	template<>
	struct hash<LR1DFA::LR1_items_t>	// 特例化 LR1 项目集的 hash 类
	{
		using result_type = size_t;
		using argument_type = LR1DFA::LR1_items_t;
		size_t operator()(const LR1DFA::LR1_items_t& items) const
		{
			size_t hash_res = 0;

			for (const auto& item : items)
			{
				hash_res ^= hash<LR1Item>()(item);
			}

			return  hash_res;
		}
	};
}

/*
* 负责由增广文法构建 LR1 分析表，分析以 '#' 结尾的句子
* 
* 输入要求：
* 必须为增广文法，且第一条产生式左部为开始符号
* 可以为二义性文法，但这意味着LR分析表有冲突，因此必须在程序中手动解决冲突
* 目前只支持 if else 语句的二义性
*
* 输入格式：
* 产生式...
* #
* 终结符 #
* 非终结符 #
*
* 输入样例：(符号与符号之间一定要有空格，终结符和非终结符支持多个字母)
* A -> S
* S -> C C
* C -> c C
* C -> d
* #
* A S C #
* c d #
*/
class LR1Processor {
	friend std::ostream& operator<<(std::ostream& out, LR1Processor& g);
	friend struct Attribute;
public:
	using sym_t = Production::sym_t;
	using ambiguity_term_t = std::pair<int, sym_t>;	// 记录LR1分析表中有二义性的位置

	LR1Processor(std::string file_name = "grammer.txt");
	std::vector<std::unordered_map<sym_t, std::string>> action_table;	// ACTION表
	std::vector<std::unordered_map<sym_t, std::string>> goto_table;	// GOTO表
	std::vector<LR1DFA::LR1_items_t> lr1_sets;	// LR1 规范项集族
	std::list<ambiguity_term_t> ambiguity_terms;	// 记录LR1分析表中有二义性的位置

	void solve_ambiguity_if();	// 解决 if 的二义性问题
	void solve_ambiguity();		// 解决输入文法的二义性问题
	std::ostream& print_lr1_table(std::ostream& os) const;	// 打印 LR1 分析表
	std::ostream& print_lr1_sets(std::ostream& os) const;	// 打印 LR1 规范项集族
	std::ostream& print_quads(std::ostream& os) const;	// 打印四元式
	
	void driver(std::vector<sym_t> &s, std::vector<Token*> tokens, bool verbose = true, std::ostream& os = std::cout);		// LR1 分析驱动程序 / 总控程序
	bool get_info(const std::string& name, Id& id) const;
	bool set_val(const std::string& name, double val);
private:
	std::ifstream in;	// 保存文法的文件
	Grammer grammer;	// 文法
	LR1DFA dfa;
	enum TableAction_t : unsigned {	// Action / Goto 表中的动作
		ACTION_READ = 0,	// 移进
		ACTION_REDUCE,	// 归约
		GOTO,			// GOTO表
		ACC,			// accept
		ERR,			// 错误
	};

	std::queue<sym_t> err_add_syms;	// 由错误恢复程序插入的符号
	void construct_table();	// 构建 LR1 分析表
	TableAction_t gen_next_mov(const int& state, sym_t next_sym, int &next) const;
	std::ostream& err_proc(std::vector<sym_t>& s, size_t &p, const std::vector<Token*> &tokens, std::ostream& os = std::cerr);		// 错误处理
	void production_action(size_t n, std::ostream& warning_msg);
	void init_state();	// 更新回初始状态

	std::stack<int> state_stack;		// 状态栈
	std::stack<sym_t> sym_stack;		// 符号栈
	std::stack<Attribute> attr_stack;	// 属性栈
	std::vector<SymTable> sym_table;			// 符号表
	size_t nxq = 1;	// 下一条四元式序号，第一条四元式地址为 1
	std::vector<Quad> quads = { Quad() };	// 四元式
	size_t tmp_idx = 0;	// 分配给临时变量的序号
	size_t warning_num = 0;	// 编译中 Warning 的个数
};

std::ostream& operator<<(std::ostream& out, Production& p);
std::ostream& operator<<(std::ostream& out, Grammer& g);
std::ostream& operator<<(std::ostream& out, LR1Processor& l);
std::istream& operator>>(std::istream& in, Grammer& g);
bool operator==(const LR1Item &item1, const LR1Item &item2);
std::ostream& operator<< (std::ostream& os, const Quad& q);
