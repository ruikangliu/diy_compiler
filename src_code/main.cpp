#include "parser.h"
#include <string>
#include <iostream>
#include <sstream>
#include <exception>

using std::string;
using std::cout;
using std::cin;
using std::endl;
using std::getline;
using std::istringstream;
using std::runtime_error;

/*
path_prefix 为路径前缀；输入或输出所需要的文件均在该路径下
grammer.txt 为文法
*/

int main(int argc, char* argv[])
{	
	string path_prefix = "E:\\Workspace\\diy_compiler\\compiler\\Debug\\";
	Parser parser(path_prefix);

	while (true)
	{
		try {
			string line;

			cout << ">>> ";
			getline(cin, line);
			if (line.empty())
			{
				continue;
			}

			// 编译源文件的命令格式：>>> lianli code.txt -v
			// -v(erbose) 表示冗余模式，输出归约信息 
			istringstream is(line);
			string cmd;
			is >> cmd;
			if (cmd == "lianli")
			{
				string file_name, opt;
				bool verbose = false;
				is >> file_name;
				if (!is.eof())
				{
					is >> opt;
					if (opt == "-v")
					{
						verbose = true;
					}
				}
				
				parser.parser_analyze(file_name, verbose);
			}
			else {
				cout << "Cmd not found!\nCmd format: lianli code.txt" << endl;
			}
		}
		catch (runtime_error err)
		{
			cout << "************************************" << endl;
			cout << "ERROR! " << endl << err.what() << endl;
			cout << "************************************" << endl;
		}
	}
	
	return 0;
}


