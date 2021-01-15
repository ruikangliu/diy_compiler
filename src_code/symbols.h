#pragma once
#include <unordered_map>
#include <string>
#include <memory>

// 默认初始值为 0
class Id {
public:
	enum Type{
		INT = 0,
		REAL,
		BOOL
	};
	Id(const std::string &id = std::string(), const Type &t = INT, const double& val = 0) : name(id), type(t), value(val) {}
	~Id() = default;

	std::string name;	// 标识符
	Type type;
	double value;
};

// 符号表
class SymTable {
public:
	SymTable() = default;
	bool insert(const Id& id)		// 在符号表中插入一项
	{
		if (table.find(id.name) != table.end())
		{
			return false;
		}
		else {
			table[id.name] = id;
			return true;
		}
	}
	void erase(const std::string& name)
	{
		table.erase(name);
	}
	bool get_info(const std::string& name, Id& id) const
	{
		if (table.find(name) == table.end())
		{
			return false;
		}
		else {
			id = table.at(name);
			return true;
		}
	}
	bool set_val(const std::string& name, double val)
	{
		if (table.find(name) == table.end())
		{
			return false;
		}
		else {
			table[name].value = val;
			return true;
		}
	}

	std::unordered_map<std::string, Id> table;
};



