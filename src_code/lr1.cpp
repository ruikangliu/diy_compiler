#include "lr1.h"
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>
#include <algorithm>
#include <stack>
#include <queue>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <unordered_map>

using std::runtime_error;
using std::ostringstream;
using std::istringstream;
using std::endl;
using std::cout;
using std::string;
using std::unordered_set;
using std::vector;
using std::find;
using std::stack;
using std::queue;
using std::setw;
using std::getline;
using std::make_pair;
using std::unordered_map;
using std::ostream;

const Production::sym_t Production::epsilon = string("$");	// 空规则
const Grammer::sym_t Grammer::end_of_sentence = string("#");		// 句子结尾

// 输出四元式
std::ostream& operator<< (std::ostream& os, const Quad& q)
{
	os << "( " << q.opt << ", " << q.lhs << ", " << q.rhs << ", " << q.dest << " )";
	return os;
}

// 输入产生式的格式：Stmts -> Stmts Stmt
// 以一行单独的 # 作为产生式输入结束的标志
bool Production::input_production(const std::string& p)
{
	if (p[0] == '#')	// 产生式部分结束
	{
		return false;
	}

	istringstream in(p);

	in >> left;
	string _;
	in >> _;	// 读入 ->

	while (in >> _)
	{
		right.push_back(_);
	}

	return true;
}

std::ostream& operator<<(std::ostream& out, Production& p)
{
	out << p.left << " -> ";
	for (const auto& r : p.right)
	{
		out << r << " ";
	}

	return out;
}

std::istream& operator>>(std::istream& in, Grammer& g)
{
	Production tmp;
	Grammer::sym_t sym;
	queue<Grammer::sym_t> q;
	string line;

	// 读入增广文法
	// 允许产生式中间有空行
	while (getline(in, line))
	{
		if (line.size() == 0)
		{
			continue;
		}
		if (tmp.input_production(line))
		{
			g.productions.push_back(tmp);
			tmp = Production();
		}
		else {
			break;
		}
	}
	// 读入非终结符
	while ((in >> sym) && sym != "#")
	{
		g.N.insert(sym);
		q.push(sym);
	}

	// 读入终结符
	while ((in >> sym) && sym != "#")
	{
		g.T.insert(sym);
		g.TN.push_back(sym);
	}
	g.TN.push_back(Grammer::end_of_sentence);	// 将 # 加入TN集

	// 将非终结符加入TN集，这样TN集的顺序就是先终结符后非终结符，且顺序与输入顺序一致，这样方便之后打印LR1分析表
	while (!q.empty())
	{
		g.TN.push_back(q.front());
		q.pop();
	}

	g.first_set();	// 求 FIRST 集

	return in;
}

std::ostream& operator<<(std::ostream& out, Grammer& g)
{
	out << "终结符: \n";
	int cnt = 0;
	for (auto it = g.T.begin(); it != g.T.end(); ++it, ++cnt)
	{
		out << *it << " ";
		if (cnt % 10 == 0 && cnt != 0)	// 一行十个符号进行输出
		{
			out << "\n";
		}
	}
	out << "\n非终结符: \n";
	cnt = 0;
	for (auto it = g.N.begin(); it != g.N.end(); ++it, ++cnt)
	{
		out << *it << " ";
		if (cnt % 10 == 0 && cnt != 0)	// 一行十个符号进行输出
		{
			out << "\n";
		}
	}
	out << "\n产生式: \n";
	for (size_t i = 0; i != g.productions.size(); ++i)
	{
		out << g.productions[i] << endl;
	}

	return out;
}

// 输出求得的 FIRST 集
std::ostream& Grammer::output_first_set(std::ostream& os) const
{
	os << "FIRST SET:\n";
	for (auto it = first.begin(); it != first.end(); ++it)
	{
		os << it->first << ":  ";
		const auto& fs = it->second;
		for (auto it1 = fs.begin(); it1 != fs.end(); ++it1)
		{
			os << *it1 << "  ";
		}
		os << "\n";
	}

	return os;
}

void Grammer::first_set()
{
	// 终结符的 FIRST 集为它本身
	for (auto it = T.begin(); it != T.end(); ++it)
	{
		first[*it].insert(*it);
	}

	// 当非终结符的 FIRST 集发生变化时循环
	bool change = true;
	while (change)
	{
		change = false;
		// 枚举每个产生式
		for (auto it = productions.begin(); it != productions.end(); ++it) {
			sym_t left = it->get_left();	// 产生式左部
			vector<sym_t> right = it->get_right();	// 产生式右部
			// A -> $
			// 如果右部第一个符号是空或者是终结符，则加入到左部的 FIRST 集中，并从待处理的产生式中删除该规则
			if (first_is_T(*it))
			{
				// 查找 FIRST 集是否已经存在该符号
				if (first[left].find(right[0]) == first[left].end())
				{
					// FIRST 集不存在该符号
					change = true;	// 标注 FIRST 集发生变化，循环继续
					first[it->get_left()].insert((it->get_right())[0]);
				}
			}
			else {	// 当前符号是非终结符
				bool next = true;	// 如果当前符号可以推出空，则还需判断下一个符号 
				size_t idx = 0;		// 待判断符号的下标

				while (next && idx != right.size()) {
					next = false;
					sym_t n = right[idx];	// 当前处理的右部非终结符

					for (auto it = first[n].begin(); it != first[n].end(); ++it) {
						// 把当前符号的 FIRST 集中非空元素加入到左部符号的 FIRST 集中
						if (*it != Production::epsilon
							&& first[left].find(*it) == first[left].end()) {
							change = true;
							first[left].insert(*it);
						}
					}
					// 当前符号的 FIRST 集中有空, 标记 next 为真，idx 下标+1
					if (first[n].find(Production::epsilon) != first[n].end()) {
						if (idx + 1 == right.size()
							&& first[left].find(Production::epsilon) == first[left].end())
						{
							// 此时说明产生式左部可以推出空；因此需要把 epsilon 加入 FIRST 集
							change = true;
							first[left].insert(Production::epsilon);
						}
						else {
							next = true;
							++idx;
						}
					}
				}
			}
		}
	}
}

std::unordered_set<Grammer::sym_t> Grammer::first_set_string(const std::vector<sym_t>& s) const
{
	unordered_set<sym_t> res;

	for (size_t i = 0; i != s.size(); ++i)
	{
		if (first_is_T(s[i]))	// 遇到终结符 / 空规则，加入 FIRST 集
		{
			res.insert(s[i]);
			break;
		}
		else {
			// 遇到非终结符，需要把该非终结符的 FIRST 集中除了空规则之外的符号都加入 res
			for (const auto& f : first.at(s[i]))
			{
				if (f != Production::epsilon)
				{
					res.insert(f);
				}
			}
			// 推不出空规则，直接结束
			if (first.at(s[i]).find(Production::epsilon) == first.at(s[i]).end())
			{
				break;
			}
			// 该非终结符如果可以推出空规则，则要查看下一个符号
			else if (i + 1 == s.size())	// 是最后一个非终结符，且可以推出空规则，则空规则也要加入 res
			{
				res.insert(Production::epsilon);
			}
		}
	}
	return res;
}

// 比较两个LR1项目是否相等
bool operator==(const LR1Item& item1, const LR1Item& item2)
{
	return (item1.next == item2.next) && (item1.loc == item2.loc)
		&& (item1.get_left() == item2.get_left()) && (item1.get_right() == item2.get_right());
}

LR1DFA::LR1_items_t LR1DFA::closure(const LR1_items_t& items)
{
	LR1_items_t closure_set;	// 最后求得的闭包
	stack<LR1Item> s;

	// 对原项集中的每个项，先把它们存到堆栈里
	for (const auto& item : items)
	{
		s.push(item);
	}
	// 每次从堆栈里弹出一个项，将该项增加的其他 LR1 项压入栈中
	while (!s.empty())
	{
		LR1Item item = s.top();
		s.pop();
		closure_set.insert(item);

		vector<sym_t> right = item.get_right();	// 产生式右部
		if (item.loc == right.size())	// 归约项，直接跳过
		{
			continue;
		}
		// [A -> alpha . B beta, u], B -> gamma   =>   [B -> .gamma, b], b is in FIRST(beta u)
		sym_t B = right[item.loc];
		if (item.loc != right.size() && grammer.N.find(B) != grammer.N.end())
		{
			vector<sym_t> beta_u;
			if (item.loc + 1 != right.size())	// beta 不为空
			{
				beta_u.push_back(right[item.loc + 1]);
			}
			beta_u.push_back(item.next);

			unordered_set<sym_t> first_b_u = grammer.first_set_string(beta_u);	// FIRST(beta u)

			// 查找每个 B -> gamma 产生式，将 [B -> .gamma, b] 入栈
			for (const auto p : grammer.productions)
			{
				if (p.get_left() == B)
				{
					vector<sym_t> r = p.get_right();
					// 因为之前处理的时候都把空规则$直接当成终结符来处理，所以在这里把它消掉
					if (r.size() == 1 && r[0] == Production::epsilon)
					{
						r = vector<sym_t>{};
					}

					for (const auto& b : first_b_u)
					{
						LR1Item tmp = LR1Item(B, r, b, 0);
						if (closure_set.find(tmp) == closure_set.end())
						{	// 只加入没有加进闭包的项目集
							s.push(tmp);
						}
					}
				}
			}
		}
	}

	return closure_set;
}

LR1DFA::LR1_items_t LR1DFA::go_to(const LR1_items_t& items, sym_t sym)
{
	LR1_items_t res;
	for (const auto& item : items)
	{
		auto right = item.get_right();
		if (item.loc != right.size() && right[item.loc] == sym)
		{
			res.insert(LR1Item(item.get_left(), right, item.next, item.loc + 1));
		}
	}
	return closure(res);
}

// 打印 LR1 项目集
std::ostream& LR1DFA::print_lr1_items(const LR1_items_t& items, std::ostream& os) const
{
	for (const auto item : items)
	{
		vector<sym_t> right = item.get_right();
		os << "[ " << item.get_left() << " -> ";
		for (size_t i = 0; i != right.size(); ++i)
		{
			if (i == item.loc)
			{
				os << ". ";
			}
			os << right[i] << " ";
		}
		if (item.loc == right.size())	// 点在产生式最后，是个归约项
		{
			os << ".";
		}
		os << "," << item.next << " ]     ";
	}

	return os;
}

/*
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
LR1Processor::LR1Processor(std::string file_name) : in(file_name) {
	in >> grammer;
	dfa.grammer = grammer;

	cout << "输入文法: \n" << grammer << endl;	// 输出文法
	grammer.output_first_set(cout) << endl;		// 输出求得的FIRST集
	construct_table();			// 构造LR1分析表

	if (!ambiguity_terms.empty())
	{
		solve_ambiguity();	// 解决二义性问题
		if (!ambiguity_terms.empty())
		{
			// 二义性问题还没有解决
			cout << endl << "************************************" << endl;
			cout << "Warning: This is not an LR1 grammer!" << endl;
			cout << "The ambiguity cannot be solved!" << endl;
			cout << "************************************" << endl << endl;
		}
		else {
			cout << endl << "************************************" << endl;
			cout << "Ambiguity solved!" << endl;
			cout << "解决二义性之后的 LR1 分析表: " << endl;
		}
	}

	print_lr1_table(cout) << endl << endl;		// 打印LR1分析表
}

std::ostream& operator<<(std::ostream& out, LR1Processor& l)
{
	out << l.grammer;
	return out;
}

void LR1Processor::construct_table()
{
	queue<LR1DFA::LR1_items_t> q;	// 保存待处理的 LR1 项集
	unordered_map<LR1DFA::LR1_items_t, size_t> items_set;	// 记录已经处理过的 LR1 项集及其在LR1规范项集族中的序号，防止重复处理

	// 初始项目：[S' -> .S, #]
	LR1DFA::LR1_items_t lr1_items{ LR1Item(grammer.productions[0], Grammer::end_of_sentence, 0) };
	lr1_items = dfa.closure(lr1_items);	// closure([S' -> .S, #])
	q.push(lr1_items);	// 加入待处理的项集队列

	lr1_sets.push_back(lr1_items);	// 将初始项目加入最终的 LR1 规范项集族
	action_table.push_back({});	// 先将对应的空表项加入action表
	goto_table.push_back({});	// 先将对应的空表项加入goto表
	items_set.insert(make_pair(lr1_items, lr1_sets.size() - 1));	// 项集+序号

	size_t idx = 0;	// 记录正在填的LR1分析表的行数

	while (!q.empty())
	{
		lr1_items = q.front();
		q.pop();

		// 查看该项集中是否有可以归约的项目，根据其对应的向前看符号将其填入 ACTION 表
		for (const auto& item : lr1_items)
		{
			if (item.loc == item.len())	// 可归约
			{
				// 找到当前归约的规则对应的序号
				size_t i = 0;
				for (i = 0; i != grammer.productions.size(); ++i)
				{
					sym_t grammer_left = grammer.productions[i].get_left();
					sym_t item_left = item.get_left();
					vector<sym_t> grammer_right = grammer.productions[i].get_right();
					vector<sym_t> item_right = item.get_right();

					if (grammer_left == item_left
						&& (grammer_right == item_right
							|| (item_right.size() == 0 && grammer_right.size() == 1 && grammer_right[0] == Production::epsilon)))// 空规则
					{
						break;
					}
				}
				if (i == 0)	// 用第一条产生式归约，说明归约到开始符号
				{
					action_table[idx][item.next] = string("ACC");
				}
				else {
					ostringstream formatted;
					formatted << "R" << i;
					action_table[idx][item.next] = formatted.str();
				}
			}
		}

		// 对每个符号，调用 GOTO 求新的 LR1 项集
		// 同时填 action 和 goto 表
		for (const auto& t : grammer.T)	// action 表
		{
			if (t != Production::epsilon && t != Grammer::end_of_sentence)
			{
				LR1DFA::LR1_items_t tmp = dfa.go_to(lr1_items, t);	// 新的 LR1 项集 或 已经出现过的 LR1 项集
				if (tmp.size() != 0)	// 项集有效，说明 t 在该状态是合法符号
				{
					auto it = items_set.find(tmp);
					ostringstream formatted;	// 最后填入 ACTION 表的字符串流

					if (it != items_set.end())	// 该项集已经出现过了
					{
						formatted << "S" << it->second;
					}
					else {	// 新的 LR1 项集
						q.push(tmp);
						lr1_sets.push_back(tmp);	// 加入最终的 LR1 规范项集族
						action_table.push_back({});	// 先将对应的空表项加入action表
						goto_table.push_back({});	// 先将对应的空表项加入goto表
						items_set.insert(make_pair(tmp, lr1_sets.size() - 1));

						formatted << "S" << lr1_sets.size() - 1;
					}
					// 把转换关系填入 ACTION 表
					if (action_table[idx].find(t) != action_table[idx].end())
					{
						// LR1 分析表冲突
						cout << endl << "************************************" << endl;
						cout << "Warning: This is not an LR1 grammer!" << endl;
						action_table[idx][t] += " " + formatted.str(); // 把两项都填入表内
						cout << "冲突项 [ " << idx << ", " << t << " ]: " << action_table[idx][t] << endl << endl;
						ambiguity_terms.push_back({ static_cast<int>(idx), t });
						cout << "************************************" << endl << endl;
					}
					else {
						action_table[idx][t] = formatted.str();
					}
				}
			}
		}
		for (const auto& n : grammer.N)	// goto 表
		{
			if (n != grammer.productions[0].get_left())	// 开始符号的归约在遇到ACTION表中的ACC时就已经完成了，不用填到GOTO表中
			{
				LR1DFA::LR1_items_t tmp = dfa.go_to(lr1_items, n);	// 新的 LR1 项集 或 已经出现过的 LR1 项集

				if (tmp.size() != 0)	// 项集有效，说明 n 在该状态是合法符号
				{
					auto it = items_set.find(tmp);
					ostringstream formatted;	// 最后填入 GOTO 表的字符串流

					if (it != items_set.end())	// 该项集已经出现过了
					{
						formatted << it->second;
					}
					else {	// 新的 LR1 项集
						q.push(tmp);
						lr1_sets.push_back(tmp);	// 加入最终的 LR1 规范项集族
						action_table.push_back({});	// 先将对应的空表项加入action表
						goto_table.push_back({});	// 先将对应的空表项加入goto表
						items_set.insert(make_pair(tmp, lr1_sets.size() - 1));

						formatted << lr1_sets.size() - 1;
					}
					// 把转换关系填入 GOTO 表
					if (goto_table[idx].find(n) != goto_table[idx].end())
					{
						// LR1 分析表冲突
						cout << "Warning: This is not an LR1 grammer!" << endl << endl;
						goto_table[idx][n] = " " + formatted.str();	// 把两项都填入表内
						ambiguity_terms.push_back({ static_cast<int>(idx), n });
					}
					else {
						goto_table[idx][n] = formatted.str();
					}
				}
			}
		}
		++idx;
	}
}

std::ostream& LR1Processor::print_lr1_sets(std::ostream& os) const
{
	for (size_t i = 0; i != lr1_sets.size(); ++i)
	{
		os << "LR1Items " << i << ";\n";
		dfa.print_lr1_items(lr1_sets[i], os) << endl;
	}
	return os;
}

std::ostream& LR1Processor::print_lr1_table(std::ostream& os) const
{
	const long long col_width = 10;	// 输出分析表一列的宽度
	const long long first_col_width = 10;
	const string err_str(" ");	// LR1分析表中错误的项怎么打印，可以用空白，也可以换成“err”

	os << "LR1项集族: " << endl;
	print_lr1_sets(os) << endl; //打印 LR1 规范项集族

	os << std::left;	// 左对齐
	os << setw(first_col_width) << "STATE"
		<< setw((grammer.T.size() - 1) * col_width) << "ACTION"		// 去除$的宽度
		<< setw(grammer.N.size() * col_width) << "GOTO" << endl;
	os << setw(first_col_width) << " ";

	for (const auto& c : grammer.TN)
	{
		os << setw(col_width) << c;
	}
	os << endl;

	for (size_t i = 0; i != lr1_sets.size(); ++i)
	{
		os << setw(first_col_width) << i;
		// 打印 action 表的一行
		for (size_t j = 0; j + 1 != grammer.T.size(); ++j)
		{
			auto it = action_table[i].find(grammer.TN[j]);
			if (it != action_table[i].end())
			{
				os << setw(col_width) << it->second;
			}
			else {
				os << setw(col_width) << err_str;
			}
		}
		// 打印 goto 表的一行
		for (size_t j = grammer.T.size() - 1; j != grammer.TN.size(); ++j)
		{
			auto it = goto_table[i].find(grammer.TN[j]);
			if (it != goto_table[i].end())
			{
				os << setw(col_width) << it->second;
			}
			else {
				os << setw(col_width) << err_str;
			}
		}
		os << endl;
	}
	os << std::right;	// 恢复右对齐
	return os;
}

/*
该函数中处理了 if 语句的二义性问题，即遇到移进 - 归约冲突，且向前看符号为 else，则强制移进
*/
void LR1Processor::solve_ambiguity_if()
{
	int change_flag = 1;
	while (change_flag)
	{
		change_flag = 0;
		for (auto it = ambiguity_terms.begin(); it != ambiguity_terms.end(); ++it)
		{
			if (it->second == "else")
			{
				istringstream is(action_table[it->first][it->second]);
				string tmp;
				while (is >> tmp)
				{
					if (tmp[0] == 'S')	// 移进项目
					{
						action_table[it->first][it->second] = tmp;	// 只取移进项目
						break;
					}
				}
				ambiguity_terms.erase(it);
				change_flag = 1;
				break;	// 去除了一个冲突项之后就马上跳出，进行下一次遍历，因为删除元素后会使迭代器失效
			}
		}
	}
}

/*
*
*如果之后有其他二义性问题需要解决，也应该在该函数中添加相应的功能函数
*/
void LR1Processor::solve_ambiguity()
{
	solve_ambiguity_if();
}


/* 功能函数，由当前的状态以及下一个输入符号，通过查LR1分析表，输出相应的动作
* 如果动作为归约，则 next 为产生式编号
* 如果动作为GOTO，则 next 为下一个状态
*/
LR1Processor::TableAction_t LR1Processor::gen_next_mov(const int& state, sym_t next_sym, int& next) const
{
	// 这里假设输入的均为合法符号；非法符号应该在词法分析阶段报错
	if (grammer.T.find(next_sym) != grammer.T.end())
	{
		// 是终结符，查 ACTION 表
		auto it = action_table[state].find(next_sym);

		if (it == action_table[state].end())
		{
			// ACTION 表对应项目为空，该句子错误
			return TableAction_t::ERR;
		}
		else {
			string action = it->second;
			if (action[0] == 'S')	// 动作为移进
			{
				istringstream is(action);
				char _;
				is >> _ >> next;

				return TableAction_t::ACTION_READ;
			}
			else if (action[0] == 'A')	// ACC
			{
				return TableAction_t::ACC;
			}
			else {	// 动作为归约
				istringstream is(action);
				char _;
				is >> _ >> next;

				return TableAction_t::ACTION_REDUCE;
			}
		}
	}
	else {
		// 是非终结符，查 GOTO 表
		auto it = goto_table[state].find(next_sym);

		if (it == goto_table[state].end())
		{
			// GOTO 表对应项目为空，该句子错误
			return TableAction_t::ERR;
		}
		else {
			string go_to = it->second;
			istringstream is(go_to);

			is >> next;

			return TableAction_t::GOTO;
		}
	}
}

/*
* 错误处理
* @state_stack: 状态栈
* @sym_stack: 符号栈
* @s: 输入串
* @p: 指示当前读到输入串的哪个位置
* @os: 错误信息输出流，默认为 std::cerr
*/
std::ostream& LR1Processor::err_proc(std::vector<sym_t>& s, size_t& p, const vector<Token*>& tokens, std::ostream& os)
{
	/*
	进行简单的短语层次错误恢复，思路如下：
		如果栈顶状态对应的 LR1 项目集中有一个归约项且向前看符号为 ;  } )，
		则强制进行归约并输出相应的出错信息，并假设已读入相应的  ;  } )

		因此，现在源程序里少个右括号和分号都问题不大了 :)

		以后有时间的话可以增加更多合理的出错处理程序~
	*/
	int now_state = state_stack.top();
	bool err_recover_flag = false;		// 是否成功进行错误恢复

	for (const auto& lr1_item : lr1_sets[now_state])
	{
		if (lr1_item.loc == lr1_item.len()
			&& (lr1_item.next == ";" || lr1_item.next == "}" || lr1_item.next == ")"))
		{
			size_t line = (p == 0) ? p : p - 1;	// 因为调用该函数的时候已经读入分界符 ) } ; 的前一个符号，所以这里的行号应该为前一个字符的行号
			os << endl << "Line" << tokens[line]->line << ": " << endl;
			os << "Warning " << warning_num << ": Lack of " << lr1_item.next << endl;

			++warning_num;
			err_add_syms.push(lr1_item.next);	// 向输入的程序中添加进适当的分界符来进行错误恢复
			err_recover_flag = true;
			break;
		}
	}

	if (err_recover_flag == false)	// 无法错误恢复，直接报错并输出可行的输入符号
	{
		ostringstream err_msg;

		err_msg << "Line" << tokens[p]->line << ": " << endl;
		err_msg << "state " << now_state << " can't accept " << s[p] << " !" << endl;
		err_msg << "possible symbols are: " << endl;
		for (const auto& t : grammer.T)
		{
			int _;
			if (TableAction_t::ERR != gen_next_mov(state_stack.top(), t, _))
			{
				err_msg << t << " ";
			}
		}
		throw runtime_error(err_msg.str());
	}

	return os;
}

void LR1Processor::init_state()
{
	err_add_syms = queue<sym_t>();
	state_stack = stack<int>();
	sym_stack = stack<sym_t>();
	attr_stack = stack<Attribute>();
	sym_table = vector<SymTable>();
	nxq = 1;
	quads = { Quad() };
	tmp_idx = 0;
	warning_num = 0;
}

// LR1 分析驱动程序 / 总控程序；分析以 '#' 结尾的句子
void LR1Processor::driver(std::vector<sym_t>& s, std::vector<Token*> tokens, bool verbose, std::ostream& os)
{
	size_t p = 0;				// 指向输入串的指针
	bool acc_flag = 0;
	ostringstream warning_msg;

	if (verbose)
	{
		os << endl << "LR1 analysis for \"";
		for (const auto& str : s)
		{
			os << str << " ";
		}
		os << "\" :" << endl;
	}

	// 初始化状态
	init_state();
	state_stack.push(0);
	sym_stack.push(Grammer::end_of_sentence);
	attr_stack.push(Attribute());

	warning_msg << "***********************************" << endl;

	while ((p != s.size() || !err_add_syms.empty())
		&& acc_flag == false)
	{
		int state = state_stack.top();
		int next = 0;
		TableAction_t next_mov;
		ostringstream _;
		bool use_err_buff_flag = !err_add_syms.empty();	// 错误恢复中将字符插入到err_add_syms中，因此优先读其中的字符
		sym_t next_sym = use_err_buff_flag ? err_add_syms.front() : s[p];

		next_mov = gen_next_mov(state, next_sym, next);

		switch (next_mov)
		{
		case TableAction_t::ACTION_READ:	// 移进
			if (use_err_buff_flag)
			{
				sym_stack.push(err_add_syms.front());
				err_add_syms.pop();
				attr_stack.push(Attribute());
			}
			else {
				sym_stack.push(s[p]);
				if (s[p] == "num")
				{
					Attribute tmp;
					tmp.value = dynamic_cast<Num*>(tokens[p])->value;
					tmp.line = tokens[p]->line;
					attr_stack.push(tmp);
				}
				else if (s[p] == "id")
				{
					Attribute tmp;
					tmp.lexeme = dynamic_cast<Word*>(tokens[p])->lexeme;
					tmp.line = tokens[p]->line;
					attr_stack.push(tmp);
				}
				else {
					Attribute tmp;
					tmp.line = tokens[p]->line;

					attr_stack.push(tmp);
				}
				++p;
			}
			state_stack.push(next);
			if (next_sym == "{")	// 进入块，压入一个新的符号表
			{
				sym_table.push_back(SymTable());
			}
			else if (next_sym == "}")	// 退出块，弹出一个符号表
			{
				sym_table.pop_back();
			}
			break;
		case TableAction_t::ACTION_REDUCE:	// 归约
		{
			Production production = grammer.productions[next];	// 归约时要使用的产生式
			int next_state = 0;

			production_action(next, _);	// 完成产生式对应的语义动作

			// 如果是空规则，就把 [A->$] 改为 [A->] 的形式
			if (production.get_right().size() == 1 && production.get_right()[0] == Production::epsilon)
			{
				production = Production(production.get_left(), vector<sym_t>());
			}

			for (size_t i = 0; i != production.get_right().size(); ++i)	// 符号栈和状态栈弹出相应的元素
			{
				state_stack.pop();
				sym_stack.pop();
			}
			sym_stack.push(production.get_left());	// 归约得到的非终结符入符号栈
			// 查GOTO表，找下一个状态
			if (TableAction_t::GOTO == gen_next_mov(state_stack.top(), production.get_left(), next_state))
			{
				state_stack.push(next_state);
			}
			else {	// ERR
				ostringstream err_msg;

				err_msg << "Line" << tokens[p]->line << ": " << endl;
				err_msg << "state " << state_stack.top() << " can't accept " << production.get_left() << " !" << endl;
				err_msg << "??????????? from GOTO" << endl;	// 这里应该不会出错才对
				throw runtime_error(err_msg.str());
			}

			if (verbose)
			{
				os << production << endl << "          符号栈: ";
				stack<sym_t> s = sym_stack;
				while (!s.empty())
				{
					os << s.top() << " ";
					s.pop();
				}
				cout << endl;
			}

			break;
		}
		case TableAction_t::ACC:	// accept
			if (verbose)
			{
				cout << grammer.productions[0] << endl << "ACC!" << endl;
			}
			production_action(0, _);	// 完成产生式对应的语义动作
			acc_flag = true;
			break;
		default:		// err
			// 错误处理的buffer读完之前应该都不会进这里
			err_proc(s, p, tokens, warning_msg);
			break;
		}
	}
	if (warning_num == 0)
	{
		warning_msg << "No Warning!" << endl;
	}
	else {
		warning_msg << endl << warning_num << " Warning(s)!";
	}

	warning_msg << endl << "************************************" << endl;

	os << warning_msg.str() << endl;
	os << "四元式: " << endl;
	print_quads(os);
}

// n 为产生式序号
void LR1Processor::production_action(size_t n, ostream& warning_msg)
{
	Attribute res;	// 归约之后产生式左部的属性
	vector<Attribute> attr_cache;	// 缓存一下弹出的属性
	Production production = grammer.productions[n];	// 归约时要使用的产生式
	Attribute::chain_t Attribute::* pchain;
	Id id;
	SymTable* now_sym_table = nullptr;
	int warning_num = 0;

	// 如果没有符号表了，说明归约到程序开始和结束的程序块 { decls stmts }，以后也不需要符号表了
	if (sym_table.size() != 0)
	{
		now_sym_table = &(sym_table.back());	// 当前块对应的符号表
	}

	// 如果是空规则，就把 [A->$] 改为 [A->] 的形式
	if (production.get_right().size() == 1 && production.get_right()[0] == Production::epsilon)
	{
		production = Production(production.get_left(), vector<sym_t>());
	}

	for (size_t i = 0; i != production.get_right().size(); ++i)	// 属性栈弹出相应的元素
	{
		attr_cache.push_back(attr_stack.top());
		attr_stack.pop();
	}

	if (attr_cache.size() != 0)
	{
		res.line = attr_cache[attr_cache.size() - 1].line;
	}

	switch (n)	// n 为产生式编号
	{
	case 0:
		// program->block
		pchain = &Attribute::chain;	// 回填最后一部分
		attr_cache[0].backpatch(pchain, nxq, quads);
		quads.push_back(Quad("End", "_", "_", "_"));	// 产生最后一条产生式
		++nxq;
		break;
	case 1:		// block -> { decls stmts }
		res.chain = attr_cache[1].chain;
		break;
	case 2:		// decls -> decl ; decls
		break;
	case 3:		// decls -> $
		break;
	case 4:		// decl -> decl , id
		res.type = attr_cache[2].type;
		if (!now_sym_table->insert(Id(attr_cache[0].lexeme, res.type)))
		{
			// 重定义
			string err_msg = "Line" + std::to_string(res.line) + ": ";
			throw runtime_error(err_msg + "multiple definition!");
		}
		break;
	case 5:		// decl -> int id
		res.type = Id::Type::INT;
		if (!now_sym_table->insert(Id(attr_cache[0].lexeme, res.type)))
		{
			// 重定义
			string err_msg = "Line" + std::to_string(res.line) + ": ";
			throw runtime_error(err_msg + "multiple definition!");
		}
		break;
	case 6:		// decl -> real id
		res.type = Id::Type::REAL;
		if (!now_sym_table->insert(Id(attr_cache[0].lexeme, res.type)))
		{
			// 重定义
			string err_msg = "Line" + std::to_string(res.line) + ": ";
			throw runtime_error(err_msg + "multiple definition!");
		}
		break;
	case 7:		// stmts -> LS stmt
		res.chain = attr_cache[0].chain;
		break;
	case 8:		// stmts -> $
		break;
	case 9:		// LS -> stmts
		pchain = &Attribute::chain;
		attr_cache[0].backpatch(pchain, nxq, quads);
		break;
	case 10:		// stmt -> C stmt
		pchain = &Attribute::chain;
		res.merge(pchain, attr_cache[0].chain, attr_cache[1].chain);
		break;
	case 11:		// stmt -> TP stmt
		pchain = &Attribute::chain;
		res.merge(pchain, attr_cache[0].chain, attr_cache[1].chain);
		break;
	case 12:		// C -> if bool then
		pchain = &Attribute::TC;
		attr_cache[1].backpatch(pchain, nxq, quads);
		res.chain = attr_cache[1].FC;
		break;
	case 13:		// TP -> C stmt else
	{
		Attribute::chain_t q = { nxq };
		quads.push_back(Quad("j", "_", "_", std::to_string(0)));
		++nxq;
		pchain = &Attribute::chain;
		attr_cache[2].backpatch(pchain, nxq, quads);
		pchain = &Attribute::chain;
		res.merge(pchain, attr_cache[1].chain, q);
		break;
	}
	case 14:		// stmt -> Wd stmt
		pchain = &Attribute::chain;
		attr_cache[0].backpatch(pchain, attr_cache[1].quad, quads);
		quads.push_back(Quad("j", "_", "_", std::to_string(attr_cache[1].quad)));
		++nxq;
		res.chain = attr_cache[1].chain;
		break;
	case 15:		// stmt -> Ww bool ;
		pchain = &Attribute::TC;
		attr_cache[1].backpatch(pchain, attr_cache[2].quad, quads);
		res.chain = attr_cache[1].FC;
		break;
	case 16:		// Wd -> W bool D
		pchain = &Attribute::TC;
		attr_cache[1].backpatch(pchain, nxq, quads);
		res.chain = attr_cache[1].FC;
		res.quad = attr_cache[2].quad;
		break;
	case 17:		// Ww -> D stmt W
		pchain = &Attribute::chain;
		attr_cache[1].backpatch(pchain, nxq, quads);
		res.quad = attr_cache[2].quad;
		break;
	case 18:		// W -> while
		res.quad = nxq;
		break;
	case 19:		// D -> do
		res.quad = nxq;
		break;
	case 20:		// stmt -> id = expr ;
		res.chain = Attribute::chain_t();

		if (get_info(attr_cache[3].lexeme, id))
		{
			attr_cache[3].type = id.type;
			if (attr_cache[1].type != attr_cache[3].type)
			{
				string new_place = "t" + std::to_string(tmp_idx++);
				if (attr_cache[3].type == Id::Type::INT)
				{
					// 自动类型转换，可能会损失精度
					warning_msg << endl << "Line" << std::to_string(res.line) << ": " << endl
						<< "Warning " << warning_num++ << ": Real to int!" << endl;

					// real to int
					quads.push_back(Quad("rtoi", attr_cache[1].place, "_", new_place));
				}
				else {
					// int to real
					quads.push_back(Quad("itor", attr_cache[1].place, "_", new_place));
				}
				++nxq;
				attr_cache[1].place = new_place;
			}
			quads.push_back(Quad("=", attr_cache[1].place, "_", "ENTRY(" + attr_cache[3].lexeme + ")"));
			++nxq;
			set_val(attr_cache[3].lexeme, attr_cache[1].value);
		}
		else {
			string err_msg = "Line" + std::to_string(res.line) + ": "
				+ "Undefined variant " + attr_cache[3].lexeme + " !";
			throw runtime_error(err_msg);
		}

		break;
	case 21:		// stmt -> block
		res.chain = attr_cache[0].chain;
		break;
	case 22:		// expr -> expr + term
		if (attr_cache[0].type != attr_cache[2].type)
		{
			string new_place = "t" + std::to_string(tmp_idx++);
			if (attr_cache[2].type == Id::Type::INT)
			{
				// int to real
				quads.push_back(Quad("itor", attr_cache[2].place, "_", new_place));
				attr_cache[2].place = new_place;
			}
			else {
				// int to real
				quads.push_back(Quad("itor", attr_cache[0].place, "_", new_place));
				attr_cache[0].place = new_place;
			}
			res.type = Id::Type::REAL;
			++nxq;
		}
		else {
			res.type = attr_cache[0].type;
		}

		res.place = "t" + std::to_string(tmp_idx++);
		quads.push_back(Quad("+", attr_cache[2].place, attr_cache[0].place, res.place));
		++nxq;
		res.value = attr_cache[2].value + attr_cache[0].value;
		break;
	case 23:		// expr -> expr - term
		if (attr_cache[0].type != attr_cache[2].type)
		{
			string new_place = "t" + std::to_string(tmp_idx++);
			if (attr_cache[2].type == Id::Type::INT)
			{
				// int to real
				quads.push_back(Quad("itor", attr_cache[2].place, "_", new_place));
				attr_cache[2].place = new_place;
			}
			else {
				// int to real
				quads.push_back(Quad("itor", attr_cache[0].place, "_", new_place));
				attr_cache[0].place = new_place;
			}
			res.type = Id::Type::REAL;
			++nxq;
		}
		else {
			res.type = attr_cache[0].type;
		}

		res.place = "t" + std::to_string(tmp_idx++);
		quads.push_back(Quad("-", attr_cache[2].place, attr_cache[0].place, res.place));
		++nxq;
		res.value = attr_cache[2].value - attr_cache[0].value;
		break;
	case 24:		// expr -> term
		res.type = attr_cache[0].type;
		res.place = attr_cache[0].place;
		res.value = attr_cache[0].value;
		break;
	case 25:		// term -> term * unary
		if (attr_cache[0].type != attr_cache[2].type)
		{
			string new_place = "t" + std::to_string(tmp_idx++);
			if (attr_cache[2].type == Id::Type::INT)
			{
				// int to real
				quads.push_back(Quad("itor", attr_cache[2].place, "_", new_place));
				attr_cache[2].place = new_place;
			}
			else {
				// int to real
				quads.push_back(Quad("itor", attr_cache[0].place, "_", new_place));
				attr_cache[0].place = new_place;
			}
			res.type = Id::Type::REAL;
			++nxq;
		}
		else {
			res.type = attr_cache[0].type;
		}

		res.place = "t" + std::to_string(tmp_idx++);
		quads.push_back(Quad("*", attr_cache[2].place, attr_cache[0].place, res.place));
		++nxq;
		res.value = attr_cache[2].value * attr_cache[0].value;
		break;
	case 26:		// term -> term / unary
		if (attr_cache[0].type != attr_cache[2].type)
		{
			string new_place = "t" + std::to_string(tmp_idx++);
			if (attr_cache[2].type == Id::Type::INT)
			{
				// int to real
				quads.push_back(Quad("itor", attr_cache[2].place, "_", new_place));
				attr_cache[2].place = new_place;
			}
			else {
				// int to real
				quads.push_back(Quad("itor", attr_cache[0].place, "_", new_place));
				attr_cache[0].place = new_place;
			}
			res.type = Id::Type::REAL;
			++nxq;
		}
		else {
			res.type = attr_cache[0].type;
		}

		res.place = "t" + std::to_string(tmp_idx++);
		quads.push_back(Quad("/", attr_cache[2].place, attr_cache[0].place, res.place));
		++nxq;
		if (attr_cache[0].value == 0)
		{
			string err_msg = "Line" + std::to_string(res.line) + ": "
				+ "Err: Divided by zero!";
			throw runtime_error(err_msg);
		}
		else {
			res.value = attr_cache[2].value / attr_cache[0].value;
		}
		break;
	case 27:		// term -> unary
		res.type = attr_cache[0].type;
		res.place = attr_cache[0].place;
		res.value = attr_cache[0].value;
		break;
	case 28:		// unary -> - unary
		res.type = attr_cache[0].type;
		res.place = "t" + std::to_string(tmp_idx++);
		quads.push_back(Quad("@", attr_cache[0].place, "_", res.place));
		++nxq;
		res.value = -attr_cache[0].value;
		break;
	case 29:		// unary -> factor
		res.type = attr_cache[0].type;
		res.place = attr_cache[0].place;
		res.value = attr_cache[0].value;
		break;
	case 30:		// factor -> ( expr )
		res.type = attr_cache[1].type;
		res.place = attr_cache[1].place;
		res.value = attr_cache[1].value;
		break;
	case 31:		// factor -> id
		if (!get_info(attr_cache[0].lexeme, id))
		{
			string err_msg = "Line" + std::to_string(res.line) + ": "
				+ "Undefined variant " + attr_cache[0].lexeme + " !";
			throw runtime_error(err_msg);
		}
		else {
			res.place = "ENTRY(" + attr_cache[0].lexeme + ")";
			res.type = id.type;
			res.value = id.value;
		}
		break;
	case 32:		// factor -> num
		res.type = Id::Type::REAL;
		res.place = std::to_string(attr_cache[0].value);
		res.value = attr_cache[0].value;
		break;
	case 33:		// bool -> boolor join
		res.place = attr_cache[1].place + " || " + attr_cache[0].place;

		pchain = &Attribute::TC;
		res.merge(pchain, attr_cache[1].TC, attr_cache[0].TC);
		res.FC = attr_cache[0].FC;
		break;
	case 34:		// bool -> join
		res.place = attr_cache[0].place;
		res.value = attr_cache[0].value;
		res.TC = attr_cache[0].TC;
		res.FC = attr_cache[0].FC;
		break;
	case 35:		// join -> joinand boolterm 
		res.place = attr_cache[1].place + " && " + attr_cache[0].place;

		pchain = &Attribute::FC;
		res.merge(pchain, attr_cache[1].FC, attr_cache[0].FC);
		res.TC = attr_cache[0].TC;
		break;
	case 36:		// join -> boolterm
		res.place = attr_cache[0].place;
		res.value = attr_cache[0].value;
		res.TC = attr_cache[0].TC;
		res.FC = attr_cache[0].FC;
		break;
	case 37:		// boolor -> bool or
		res.place = attr_cache[1].place;
		res.value = attr_cache[1].value;
		res.TC = attr_cache[1].TC;
		pchain = &Attribute::FC;
		attr_cache[1].backpatch(pchain, nxq, quads);
		break;
	case 38:		// joinand -> join and
		res.place = attr_cache[1].place;
		res.value = attr_cache[1].value;
		res.FC = attr_cache[1].FC;
		pchain = &Attribute::TC;
		attr_cache[1].backpatch(pchain, nxq, quads);
		break;
	case 39:		// boolterm -> equality
		res.place = attr_cache[0].place;
		res.value = attr_cache[0].value;
		res.TC = { nxq };
		res.FC = { nxq + 1 };
		quads.push_back(Quad("jtrue", attr_cache[0].place, "_", std::to_string(0)));
		++nxq;
		quads.push_back(Quad("j", "_", "_", std::to_string(0)));
		++nxq;
		break;
	case 40:		// equality -> equality == rel
		res.place = "t" + std::to_string(tmp_idx++);
		quads.push_back(Quad("==", attr_cache[2].place, attr_cache[0].place, res.place));
		++nxq;
		res.value = (attr_cache[2].value == attr_cache[0].value);
		break;
	case 41:		// equality -> equality != rel
		res.place = "t" + std::to_string(tmp_idx++);
		quads.push_back(Quad("!=", attr_cache[2].place, attr_cache[0].place, res.place));
		++nxq;
		res.value = (attr_cache[2].value != attr_cache[0].value);
		break;
	case 42:		// equality -> rel
		res.place = attr_cache[0].place;
		res.value = attr_cache[0].value;
		break;
	case 43:		// equality -> true
		res.place = "true";
		res.value = 1;
		break;
	case 44:		// equality -> false
		res.place = "false";
		res.value = 0;
		break;
	case 45:		// rel -> rel < relexpr
		res.place = "t" + std::to_string(tmp_idx++);
		quads.push_back(Quad("<", attr_cache[2].place, attr_cache[0].place, res.place));
		++nxq;
		res.value = (attr_cache[2].value < attr_cache[0].value);
		break;
	case 46:		// rel -> rel <= relexpr
		res.place = "t" + std::to_string(tmp_idx++);
		quads.push_back(Quad("<=", attr_cache[2].place, attr_cache[0].place, res.place));
		++nxq;
		res.value = (attr_cache[2].value <= attr_cache[0].value);
		break;
	case 47:		// rel -> rel >= relexpr
		res.place = "t" + std::to_string(tmp_idx++);
		quads.push_back(Quad(">=", attr_cache[2].place, attr_cache[0].place, res.place));
		++nxq;
		res.value = (attr_cache[2].value >= attr_cache[0].value);
		break;
	case 48:		// rel -> rel > relexpr
		res.place = "t" + std::to_string(tmp_idx++);
		quads.push_back(Quad(">", attr_cache[2].place, attr_cache[0].place, res.place));
		++nxq;
		res.value = (attr_cache[2].value > attr_cache[0].value);
		break;
	case 49:		// rel -> relexpr
		res.place = attr_cache[0].place;
		res.value = attr_cache[0].value;
		break;
	case 50:		// relexpr -> relexpr + relterm 
		res.place = "t" + std::to_string(tmp_idx++);
		quads.push_back(Quad("+", attr_cache[2].place, attr_cache[0].place, res.place));
		++nxq;
		res.value = attr_cache[2].value + attr_cache[0].value;
		break;
	case 51:		// relexpr -> relexpr - relterm
		res.place = "t" + std::to_string(tmp_idx++);
		quads.push_back(Quad("-", attr_cache[2].place, attr_cache[0].place, res.place));
		++nxq;
		res.value = attr_cache[2].value - attr_cache[0].value;
		break;
	case 52:		// relexpr -> relterm
		res.place = attr_cache[0].place;
		res.value = attr_cache[0].value;
		break;
	case 53:		// relterm -> relterm * relunary
		res.place = "t" + std::to_string(tmp_idx++);
		quads.push_back(Quad("*", attr_cache[2].place, attr_cache[0].place, res.place));
		++nxq;
		res.value = attr_cache[2].value * attr_cache[0].value;
		break;
	case 54:		// relterm -> relterm / relunary
		res.place = "t" + std::to_string(tmp_idx++);
		quads.push_back(Quad("/", attr_cache[2].place, attr_cache[0].place, res.place));
		++nxq;
		if (attr_cache[0].value == 0)
		{
			string err_msg = "Line" + std::to_string(res.line) + ": "
				+ "Err: Divided by zero!";
			throw runtime_error(err_msg);
		}
		else {
			res.value = attr_cache[2].value / attr_cache[0].value;
		}
		break;
	case 55:		// relterm -> relunary
		res.place = attr_cache[0].place;
		res.value = attr_cache[0].value;
		break;
	case 56:		// relunary -> ! relunary
		res.place = "t" + std::to_string(tmp_idx++);
		quads.push_back(Quad("!", attr_cache[0].place, "_", res.place));
		++nxq;
		res.value = (attr_cache[0].value == 0) ? 1 : 0;
		break;
	case 57:		// relunary -> - relunary
		res.place = "t" + std::to_string(tmp_idx++);
		quads.push_back(Quad("@", attr_cache[0].place, "_", res.place));
		++nxq;
		res.value = -attr_cache[0].value;
		break;
	case 58:		// relunary -> relfactor
		res.place = attr_cache[0].place;
		res.value = attr_cache[0].value;
		break;
	case 59:		// relfactor -> ( bool )
		pchain = &Attribute::TC;
		attr_cache[1].backpatch(pchain, nxq, quads);
		pchain = &Attribute::FC;
		attr_cache[1].backpatch(pchain, nxq, quads);
		res.place = attr_cache[1].place;
		res.value = attr_cache[1].value;
		break;
	case 60:		// relfactor -> id
		if (!get_info(attr_cache[0].lexeme, id))
		{
			string err_msg = "Line" + std::to_string(res.line) + ": "
				+ "Undefined variant " + attr_cache[0].lexeme + " !";
			throw runtime_error(err_msg);
		}
		else {
			res.place = "ENTRY(" + attr_cache[0].lexeme + ")";
			res.value = id.value;
		}
		break;
	case 61:		// relfactor -> num
		res.place = std::to_string(attr_cache[0].value);
		res.value = attr_cache[0].value;
		break;
	default:
		cout << "Err!" << endl;
		break;
	}
	attr_stack.push(res);
}

bool LR1Processor::get_info(const std::string& name, Id& id) const
{
	// 从当前块开始，向外层块逐层寻找变量
	for (auto it = sym_table.rbegin(); it != sym_table.rend(); ++it)
	{
		if (it->get_info(name, id))
		{
			return true;
		}
	}
	return false;
}

bool LR1Processor::set_val(const std::string& name, double val)
{
	Id id;
	// 从当前块开始，向外层块逐层寻找变量
	for (auto it = sym_table.rbegin(); it != sym_table.rend(); ++it)
	{
		if (it->table.find(name) != it->table.end())
		{
			(it->table)[name].value = val;
			return true;
		}
	}
	return false;
}

std::ostream& LR1Processor::print_quads(std::ostream& os) const
{
	for (size_t i = 1; i != quads.size(); ++i)
	{
		os << i << ": " << quads[i] << endl;
	}
	return os;
}



