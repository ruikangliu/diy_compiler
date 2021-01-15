- **代码已上传至** [Github](https://github.com/lianli-o/diy_compiler.git)



# 源语言、语义动作、中间代码定义
## 整体框架

$$\begin{aligned}
program &➔ block\\
block &➔ \{ decls\ \  stmts\}\\
\end{aligned}$$
- 源语言的一个程序由一个块组成，该块中包含可选的声明和语句；其中声明总是在块的最前面
- 支持两种注释：单行注释：$//$；多行注释：$/**/$

***
**语义动作**：
$$\begin{aligned}
program &➔ block\\
\{ &BACKPATCH (block.CHAIN,NXQ);\\
&GEN(End, \_, \_, \_);\}\end{aligned}$$
- 使用此产生式进行归约表示整个程序已经规约完成，因此产生一条四元式 $GEN(End, \_, \_, \_)$ 表示整个程序的结束，并将 $block.CHAIN$ 中的四元式跳转地址回填为该四元式标号  

$$\begin{aligned}
block &➔ \{ decls\ \  stmts\}\\
\{ &block .CHAIN := stmts.CHAIN\}\\\end{aligned}$$
- 暂时还无法回填 $stmts.CHAIN$，因此用 $block.CHAIN$ 保存该属性

> 在自底向上的语法制导翻译中，我主要运用符号栈进行语义处理，所以在使用一个产生式进行归约时，产生式右部的符号就会弹出，属性就会丢失；因此翻译时的关键就是对需要归约的产生式右部各个文法符号的属性进行**及时的保存或回填**；下面还有很多产生式的语义处理以及文法的改造也都是基于这个原则
## 声明

$$\begin{aligned}
decls ➔& decl;decls\\
&| \varepsilon\\
decl➔& decl,id\\
&|int\ \  id\\
&| real\ \  id\\
\end{aligned}$$

- $int$ 和 $real$ 分别表示整型和浮点型，编译程序支持 $int$ 和 $real$ 之间的自动类型转换
- 声明语句以分号结尾，支持连续定义多个同类型变量，例如 `int a, b, c, d;`

***
**语义动作**：
$$\begin{aligned}
decls ➔& decl;decls^{(1)}\\
\{&\}\\
decls ➔& \varepsilon\\
\{&\}\\
decl➔ &decl ^{(1)},id\\
\{&FILL(ENTRY(id),decl ^{(1)}.TYPE);\\
&decl .TYPE:= decl ^{(1)}.TYPE\}\\
decl➔ & int\ id\\
\{&FILL(ENTRY(id),int);\\&decl .TYPE:= int\}\\
decl ➔ &real\ id\\
\{&FILL(ENTRY(id),real);\\&decl .TYPE:= real\}
\end{aligned}$$

- $FILL(ENTRY(id),...)$ 表示将变量 $id$ 的属性值填入符号表，如果发现符号表中已有该变量，还需要产生重定义的报错


## 语句
$$\begin{aligned}
stmts ➔& stmts\ \ stmt \ \\&| \ \varepsilon\\
stmt➔& block
\end{aligned}$$
- $stmts$ 为语句串，由零个或多个语句组成
- $stmt$ 为语句，可以是一个语句块
***
为了方便的进行自底向上归约 (及时进行回填)，需要对上述文法进行**改写**：
$$\begin{aligned}
stmts ➔& L^S\ \ stmt \ \\&| \ \varepsilon\\
L^S➔&stmts\\
stmt➔& block
\end{aligned}$$

**语义动作**：
$$\begin{aligned}
stmts➔&L^S\ stmt\\
\{ &stmts .CHAIN := stmt.CHAIN\}\\
stmts ➔&\varepsilon\\
\{ &stmts.CHAIN:=0\}\\
L^S➔&stmts\\
\{ &BACKPATCH(stmts.CHAIN,NXQ) \} \\
stmt ➔&block\\
\{&stmt.CHAIN=block.CHAIN\}\\
\end{aligned}$$
- $stmts.CHAIN:=0$ 表示一条空链


### $if$ 语句
$$\begin{aligned} stmt➔ & if\ bool\ then\ stmt\\
&|if\ bool\ then\ stmt\ else\ stmt\\\end{aligned}$$


***

对文法进行**改写**：
$$\begin{aligned} stmt➔ & C\ stmt\\
&| T^P\ stmt\\
C➔ &if\ bool\ then\\
T^P➔ &C\ stmt\ else\\\end{aligned}$$

- 当然，这个文法依然是二义性的 ($if$-$then$-$if$-$then$-$else$)，无法使 $else$ 就近配对。我没有在语法定义层面消除 $if$ 语句的二义性问题，因为这样会破坏整个程序文法的可读性；我选择的是**在语义分析阶段特殊处理**
  - 在用 $C\ stmt$ 进行归约时，向前多看一个符号，如果遇到 $else$ 就使用  $T^P➔ C\ stmt\ else$ 进行归约，这样就可以保证 $else$ 的就近配对原则
  - 程序中的一些提示性输出如下，在构建完 $LR(1)$ 分析表之后，发现分析表冲突，以第一个 Warning 为例，冲突项为第 153 个 $LR1$ 项集，在遇到 $else$ 时产生了移进-归约冲突，其中归约使用的第10条产生式正是 $stmt➔ C\ stmt$：
![在这里插入图片描述](https://img-blog.csdnimg.cn/20210101002711757.png#pic_center)
  - 处理的方法也很简单，强行消去归约项只留移进项即可
![在这里插入图片描述](https://img-blog.csdnimg.cn/202101010030169.png)




**语义动作**：
$$\begin{aligned}
C ➔ &if\ bool\ then\\
\{ &BACKPATCH (bool.TC, NXQ);\\
&C.CHAIN := bool.FC \}\\
stmt➔ &C\  stmt^{(1)}\\
\{&stmt.CHAIN := MERG(C.CHAIN, stmt^{(1)}.CHAIN)\}\\
T^P ➔ &C\ stmt^{(1)}\ else\\
\{ &q := NXQ;\\
&GEN(j, \_, \_, 0);\\
&BACKPATCH(C.CHAIN,NXQ);\\
&T^P.CHAIN := MERGE(stmt^{(1)}.CHAIN,q)\}\\
stmt➔ &T^P\ stmt^{(2)}\\
\{ &stmt.CHIAN := MERG(T^P.CHIAN, stmt^{(2)}.CHIAN)\}\end{aligned}$$


### $while$ 语句
$$\begin{aligned} stmt➔ &while\ bool\ do\ stmt\\
&|do\ \ stmt\ \ while\ \ bool;\end{aligned}$$
***

对文法进行**改写**：
$$\begin{aligned} stmt➔ & W^d\ stmt\\
|&W^w\ \ bool;\\
W^d➔ &W\ bool\ D\\
W^w➔ &D\ \ stmt\ \ W\\
W ➔ &while\\
D➔ &do\end{aligned}$$

**语义动作**：
$$\begin{aligned}
W ➔ &while\\
\{ &W.QUAD := NXQ \}\end{aligned}$$
- $while$ 归约为 $W$ 是因为要记录 $do$ 中循环体做完之后的跳回位置；$W.QUAD$ 即为循环体做完判断 $E$ 的地方

$$\begin{aligned}
W^w➔ &D\ \ stmt\ \ W\\
\{ &BACKPATCH (stmt.CHAIN,NXQ);\\
&W^w.QUAD := D.QUAD \}\\
stmt➔ &W^w\ \ bool;\\
\{ &BACKPATCH(bool.TC,W^w.QUAD);\\
&stmt.CHAIN := bool.FC\}\\
stmt➔ &W^d stmt^{(1)}\\
\{ &BACHPATCH(stmt^{(1)}.CHAIN,W^d.QUAD);\\
&GEN(j,\_,\_, W^d.QUAD); \\
&stmt.CHAIN := W^d.CHAIN\}\\
W^d ➔ &W\ bool\ D\\
\{ &BACKPATCH (bool.TC,NXQ);\\
&W^d.CHAIN := bool.FC;\\
&W^d.QUAD := W.QUAD \}\\
D➔ &do\\
\{ &D.QUAD:=NXQ\}
\end{aligned}$$
- $do$ 归约为 $D$ 是因为要用 $D.QUAD$ 记录 $do-while$ 中 $bool$ 语句为 $true$ 时的跳回位置

### 赋值语句
$$\begin{aligned}
stmt➔&\ \ \ id=expr;
\end{aligned}$$
- 分号出现在所有不以 $stmt$ 结尾的产生式的末尾。这种方法可以避免在 $if$ 或 $while$ 这样的语句后面出现多余的分号，因为 $if$ 和 $while$ 语句的最后是一个嵌套的子语句。当嵌套子语句是一个赋值语句或 $do$-$while$ 语句时，分号将作为这个子语句的一部分被生成


**语义动作**：
$$\begin{aligned}
stmt➔&\ \ \ id=expr;\\
\{ &stmt.CHAIN := 0\\
&GEN(=,expr.PLACE,\_,ENTRY(id))\}\end{aligned}$$
- 如果在查找符号表时没有查找到 $id$ 的信息，需要**报错** (未定义变量)
- 如果 $expr$ 的类型与 $id$ 类型不一样，还要进行**自动类型转换**，如果 $id$ 为 $real$，则在上面的语义动作之前加上如下语义动作： $t=NEWTMP,GEN(itor,expr.PLACE,\_,t),expr.PLACE=t$，即，将 $expr$ 的值转换为新的类型值保存在临时变量中，之后将临时变量的值赋给 $id$；如果 $id$ 为 $int$，则添加的语义动作为 $t=NEWTMP,GEN(rtoi,expr.PLACE,\_,t),expr.PLACE=t$
***
$$\begin{aligned}
expr ➔&expr+ term\ \  |\ \  expr - term\ \  |\ \  term\\
term ➔& term* unary\ \  |\ \   term / unary\ \  |\ \  unary\\
unary ➔&-unary\ \  |\ \   factor\\
factor ➔& (expr )\ \   |\ \  id\ \  |\ \   num\\
\end{aligned}$$
- 为了避免文法的二义性，表达式的产生式处理了运算符的结合性和优先级。每个优先级级别都使用了一个非终结符号

***
**语义动作**：
$$\begin{aligned}
expr ➔&expr^{(1)}+ term\\
\{&expr.PLACE:=NEWTEMP;\\
&GEN(+,expr^{(1)}.PLACE,term.PLACE,expr.PLACE)\}\\
expr ➔&expr^{(1)}-term\\
\{&expr.PLACE:=NEWTEMP;\\
&GEN(-,expr^{(1)}.PLACE,term.PLACE,expr.PLACE)\}\\
expr ➔&term\\
\{&expr.PLACE:=term.PLACE\}\\
term ➔&term ^{(1)}*unary\\
\{&term .PLACE:=NEWTEMP;\\
&GEN(*,term ^{(1)}.PLACE,unary.PLACE,term .PLACE)\}\\
term ➔&term ^{(1)}/unary\\
\{&term .PLACE:=NEWTEMP;\\
&GEN(/,term ^{(1)}.PLACE,unary.PLACE,term .PLACE)\}\\
term ➔&unary\\
\{&term .PLACE:=unary.PLACE\}\\
unary ➔&-unary^{(1)}\\
\{&unary .PLACE:=NEWTEMP;\\
&GEN(@,unary^{(1)}.PLACE,\_,unary .PLACE)\}\\
unary ➔&factor\\
\{&unary .PLACE:=factor.PLACE\}\\
factor ➔& (expr)\\
\{&factor.PLACE:=expr.PLACE\}\\
factor ➔& id\\
\{&factor.PLACE=ENTRY(id)\}\\
factor ➔& num\\
\{&factor.PLACE=num.PLACE\}\\
\end{aligned}$$
- 在如下四条产生式中，还要添加类型转换的语义动作：如果两个运算分量的类型不一样，需要将 $int$ 型的值提升到 $real$，通过四元式 $(itor,...,\_,NEWTMP)$ 完成，提升后的值存入临时变量
$$expr ➔expr+ term\ \  |\ \  expr - term\\
term ➔ term* unary\ \  |\ \   term / unary$$
- 在 $term ➔term ^{(1)}/unary$ 的归约过程中，如果 $unary$ 的值为 0，也需要报错 (除以0)


### 布尔表达式
$$\begin{aligned}
bool➔&bool\ \  or\ \   join\ \  |\ \   join\\
join➔&join\ \ and\ \  equality\ \  |\ \  equality\\
equality➔&equality==rel\ \ |\ \ equality\ \ !=\ \ rel \ \ |\ \  rel\\
&|\ \ true\ \ |\ \ false\\
rel➔& rel<relexpr\ \ |\ \ rel<=relexpr\ \ |\ \ rel>=relexpr\ \ |\\
&rel>relexpr\ \ |\ \ relexpr\\
relexpr ➔&relexpr+ relterm\ \  |\ \  relexpr -relterm\ \  |\ \  relterm\\
relterm ➔& relterm* relunary\ \  |\ \   relterm / relunary\ \  |\ \  relunary\\
relunary ➔&!relunary\ \ |\ \ -relunary\ \  |\ \   relfactor\\
relfactor ➔& (bool)\ \   |\ \  id\ \  |\ \   num\\
\end{aligned}$$
***
为了更方便地进行语义分析，对该文法进一步**改造**：
$$\begin{aligned}
bool➔&bool^{or}\ \   join\ \  |\ \   join\\
join➔&join^{and}\ \  bool^{term}\ \  |\ \  bool^{term}\\
bool^{or}➔&bool\ \ or\\
join^{and}➔&join\ \ and\\
bool^{term}➔&equality\\
equality➔&equality==rel\ \ |\ \ equality\ \ !=\ \ rel \ \ |\ \  rel\\
&|\ \ true\ \ |\ \ false\\
rel➔& rel<relexpr\ \ |\ \ rel<=relexpr\ \ |\ \ rel>=relexpr\ \ |\\
&rel>relexpr\ \ |\ \ relexpr\\
relexpr ➔&relexpr+ relterm\ \  |\ \  relexpr -relterm\ \  |\ \  relterm\\
relterm ➔& relterm* relunary\ \  |\ \   relterm / relunary\ \  |\ \  relunary\\
relunary ➔&!relunary\ \ |\ \ -relunary\ \  |\ \   relfactor\\
relfactor ➔& (bool)\ \   |\ \  id\ \  |\ \   num\\
\end{aligned}$$
- $relexpr ,relterm ,relunary ,relfactor$ 的语义动作与 $expr ,term ,unary ,factor$ 类似，我将它们分开的原因是：在自底向上归约到 $bool^{term}$ 时会产生真出口和假出口的跳转指令，如果不分开的话就会在普通的赋值语句中也产生两条没有意义的跳转语句

***
**语义动作**
$$\begin{aligned}
bool➔&bool^{or}\ \   join\\
\{&bool.PLACE=bool^{or}.PLACE\ ||\  join.PLACE\\
&bool.TC=MERG(bool^{or}.TC,join.TC);\\
&bool.FC=join.FC\}\\
bool➔&join\\
\{&bool.PLACE=join.PLACE\\
&bool.TC=join.TC;\\
&bool.FC=join.FC\}\\
join➔&join^{and}\ \  bool^{term}\\
\{&join.PLACE=join^{and}.PLACE\ \&\&\ bool^{term}.PLACE\\
&join.FC=MERG(join^{and}.FC, bool^{term}.FC);\\
&join.TC= bool^{term}.TC\}\\
join➔&bool^{term}\\
\{&join.PLACE=bool^{term}.PLACE\\
&join.TC= bool^{term}.TC;\\
&join.FC= bool^{term}.FC\}\\
bool^{or}➔&bool\ \ or\\
\{&bool^{or}.PLACE=bool.PLACE\\
&BACKPATCH(bool.FC,NXQ);\\
&bool^{or}.TC=bool.TC\}\\
join^{and}➔&join\ \ and\\
\{&join^{and}.PLACE=join.PLACE\\
&BACKPATCH(join.TC,NXQ);\\
&join^{and}.FC=join.FC\}\\
bool^{term}➔&equality\\
\{&bool^{term}.PLACE=equality.PLACE\\
&bool^{term}.TC=NXQ;\\
&bool^{term}.FC=NXQ+1;\\
&GEN(j_{true},equality.PLACE,\_,0);\\
&GEN(j,\_,\_,0)\}\\
equality➔&equality^{(1)}==rel\\
\{&equality.PLACE=NEWTEMP;\\
&GEN(==,equality^{(1)}.PLACE,rel.PLACE,equality.PLACE)\}\\
equality➔&equality^{(1)}!=rel\\
\{&equality.PLACE=NEWTEMP;\\
&GEN(!=,equality^{(1)}.PLACE,rel.PLACE,equality.PLACE)\}\\
equality➔&rel\\
\{&equality.PLACE=rel.PLACE\}\\
equality➔&true\\
\{&equality.PLACE=true\}\\
equality➔&false\\
\{&equality.PLACE=false\}\\
rel➔& rel^{(1)}<relexpr\\
\{&rel.PLACE=NEWTEMP;\\
&GEN(<,rel^{(1)}.PLACE,relexpr.PLACE,rel.PLACE)\}\\
rel➔& rel^{(1)}<=relexpr\\
\{&rel.PLACE=NEWTEMP;\\
&GEN(<=,rel^{(1)}.PLACE,relexpr.PLACE,rel.PLACE)\}\\
rel➔& rel^{(1)}>relexpr\\
\{&rel.PLACE=NEWTEMP;\\
&GEN(>,rel^{(1)}.PLACE,relexpr.PLACE,rel.PLACE)\}\\
rel➔& rel^{(1)}>=relexpr\\
\{&rel.PLACE=NEWTEMP;\\
&GEN(>=,rel^{(1)}.PLACE,relexpr.PLACE,rel.PLACE)\}\\
rel➔& relexpr\\
\{&rel.PLACE=relexpr.PLACE)\}\\
relexpr ➔&relexpr^{(1)}+ relterm\\
\{&relexpr.PLACE:=NEWTEMP;\\
&GEN(+,relexpr^{(1)}.PLACE,relterm.PLACE,relexpr.PLACE)\}\\
relexpr ➔&relexpr^{(1)}-term\\
\{&relexpr.PLACE:=NEWTEMP;\\
&GEN(-,relexpr^{(1)}.PLACE,relterm.PLACE,relexpr.PLACE)\}\\
relexpr ➔&relterm\\
\{&relexpr.PLACE:=relterm.PLACE\}\\
relterm ➔&relterm ^{(1)}*relunary\\
\{&relterm .PLACE:=NEWTEMP;\\
&GEN(*,relterm ^{(1)}.PLACE,relunary.PLACE,relterm .PLACE)\}\\
relterm ➔&relterm ^{(1)}/relunary\\
\{&relterm .PLACE:=NEWTEMP;\\
&GEN(/,term ^{(1)}.PLACE,relunary.PLACE,relterm .PLACE)\}\\
relterm ➔&relunary\\
\{&relterm .PLACE:=relunary.PLACE\}\\
relunary ➔&-relunary^{(1)}\\
\{&relunary .PLACE:=NEWTEMP;\\
&GEN(@,relunary^{(1)}.PLACE,\_,relunary .PLACE)\}\\
relunary ➔&!relunary^{(1)}\\
\{&relunary .PLACE:=NEWTEMP;\\
&GEN(!,relunary^{(1)}.PLACE,\_,relunary .PLACE)\}\\
relunary ➔&relfactor\\
\{&relunary .PLACE:=relfactor.PLACE\}\\
relfactor ➔& num\\
\{&relfactor.PLACE=num.PLACE\}\\
relfactor ➔& id\\
\{&relfactor.PLACE=ENTRY(id)\}\\
relfactor➔& (bool)\\
\{&BACKPATCH(bool.TC,NXQ);\\
&BACKPATCH(bool.FC,NXQ);\\
&relfactor.PLACE=bool.PLACE\}\\
\end{aligned}$$
# 词法分析
- 词法分析器可以识别以下词法单元：
  - 数值常量
    - 支持科学计数法 $dd^*(.dd^*|\varepsilon)(e(+|-|\varepsilon)dd^*|\varepsilon)$
    - 所有数值常量的类型均为 $real$
  - 标识符：除关键字以外的，以字母或下划线打头，后跟数字字母串的单词
  - 关键字
$$true,false,if,else,then,while,do,int,real,or,and$$
  - 关系运算符：$>=,<=,!=,==,>,<$
  - 字符常量和字符串常量：定义与 C 语言相同，例如 $'c',"abc"$
  - 除了上面提到的词法单元，默认将其他所有单个字符都识别为单独的词法单元，例如取反符号 $!$ 被识别为词法单元 $<!>$




## 词类编码定义
- 对于所有词素为单个符号的词法单元，词类编码即为它们的 ASCII 码，其余词素为多个符号的词法单元对应的编码均为大于 255 的整数值
- 数字的编码为 256 ($NUM$)
- 标识符的编码为 257 ($ID$)
- 关系运算符的编码为 258 ($REL\_OPT$)
- 字符常量的编码为 259 ($CHAR$)
- 字符串常量的编码为 260 ($STRING$)
- 关键字的编码从 262 开始依次排列，顺序为 $TRUE,FALSE,IF,ELSE,THEN,WHILE,DO,INT,REAL,OR,AND$
***
- 部分代码如下；`Token` 类为词法单元的基类，定义了词类编码以及词法单元的行数；`Num` 类 (数值常量)、`Word` 类 (标识符 & 关键字)、`RelOpt` 类 (关系运算符 `>=` `<=` `!=` `==`)、`Char` 类 (字符常量)、`String` 类 (字符串常量) 均为 `Token` 类的派生类，分别添加了词素、数值等属性值
```cpp
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
	virtual std::ostream& print(std::ostream& os) 
		{ os << "line" << line << ": <" << static_cast<char>(tag) 
			<< '>'; return os; }

	int tag;		// 词类编码
	size_t line;	// 每个词法单元出现的行数
};
```
## 词法分析器
- 词法分析器由 `Lexer` 类实现

```cpp
// 词法分析器
class Lexer {
public:
	Lexer(std::string name = ".\\code.txt");
	~Lexer();

	Token* gen_token();
	// ... 这里省略了一些源代码，只粘贴一些比较重要的代码
private:
	char peek = ' ';			// 从源文件预读的一个字符
	std::unordered_map<std::string, int> ID_table;	// 关键字表
	std::ifstream in;			// 源文件

	// 功能函数
	Token* scan();
	Token* skip_blank(void);		// 跳过空白
	Token* handle_slash(void);		// 处理斜杠 /，可能使注释或除号
	// 处理 > < = !，可能是关系运算符，也可能是赋值运算符或取反运算符
	Token* handle_relation_opterator(void);	
	Token* handle_digit(void);		// 处理数字
	// 处理单词 (标识符 或 关键字) 允许下划线或字母打头，后面跟下划线/数字/字母
	Token* handle_word(void);		
	Token* handle_char(void);		// 处理字符常量
	Token* handle_string(void);		// 处理字符串常量
};
```
### 总控识别函数
- 识别流程基本仿照 $DFA$，根据预读的第一个字符转交给各个子程序去识别各词法单元
```cpp
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
```
> 下面仅介绍识别关键字、标识符、数字常量的功能函数，其余函数较简单且思路相同
### 识别关键字 与 标识符
- 区分 关键字 与 标识符
  - 首先在词法分析器 `Lexer` 的初始化函数中将所有关键字以及对应的词类编码加入关键字表中；关键字表的数据类型为 `unordered_map<string, int>`，底层使用哈希函数进行查找，效率较高
- 识别过程：先预读一个符号存入 `next_peek`，只要预读的符号为字母、数字或下划线，就一直读入，直至读出整个标识符的词素；接着在关键字表中查找该单词的词素，如果关键字表中找到该词素，说明单词为关键字，返回关键字表中的词类编码；否则返回标识符的词类编码
```cpp
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
```

### 识别数值常量
- 数值常量的正则表达式为 $dd^*(.dd^*|\varepsilon)(e(+|-|\varepsilon)dd^*|\varepsilon)$，识别过程对应的 $DFA$ 如下：
![在这里插入图片描述](https://img-blog.csdnimg.cn/20210106232544569.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3dlaXhpbl80MjQzNzExNA==,size_16,color_FFFFFF,t_70)
- 在数值常量的识别过程中，不仅要输出数值常量的词类编码，还需要输出相应的数值作为之后语义分析中的综合属性
```cpp
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
```




   
# $LR(1)$ 语法分析 + 语法制导翻译
- 语法分析部分我采用的是自底向上分析的 $LR(1)$ 分析法。 $LR(1)$ 分析法相比递归下降分析速度更快，适用的文法范围更广，而且通过读入文法自动构造 $LR(1)$ 分析表，能使扩展文法、增加语言特性变得极为方便
- 自底向上的语法制导翻译对应的语义动作在源语言介绍阶段已经有过详细介绍

## 文法输入
- `Grammer` 类：主要用来存储输入的文法并计算该文法的所有文法符号的 $FIRST$ 集
  - 我通过重载输入运算符来进行文法的读入，在程序中只需将文法内容保存在 `grammer.txt` 中即可自动读入文法并求出 $FIRST$ 集；默认读入的文法为增广文法并且第一条规则左部即为开始符号。允许文法存在二义性 (例如 $if$-$else$ 语句)，但这意味着之后 $LR$ 分析表有冲突，因此必须在程序中手动解决冲突；输入样例如下：

```cpp
// 输入样例：(符号与符号之间一定要有空格，终结符和非终结符支持多个字母)
// 先输入产生式，后输入非终结符，最后输入终结符，中间用 # 隔开
A -> S
S -> C C
C -> c C
C -> d
#
A S C #
c d #
```



## 自动构造 $LR(1)$ 分析表
### 求 $FIRST$ 集
- `Grammer` 类中的 `first_set()` 成员函数提供了求 $FIRST$ 集的功能，思路如下：
  - (1) 先将所有终结符的 $FIRST$ 集设为它们自身
  - (2) 枚举每个产生式
    - 如果产生式右部第一个符号是终结符或者是空产生式 ($\varepsilon$ 在程序中用 `$` 表示)，则将其加入到产生式左部非终结符 $S$ 的 $FIRST$ 集中
    - 如果产生式右部第一个符号是非终结符 $A$，就将 $A$ 的 $FIRST$ 集中除了 $\varepsilon$ 以外的元素全部加入左部终结符 $S$ 的 $FIRST$ 集中，如果 $A$ 的 $FIRST$ 集中有 $\varepsilon$ 且不为最后一个符号，则继续看下一个符号，把下一个符号当作第一个符号，重复 (2)；如果 $A$ 的 $FIRST$ 集中有 $\varepsilon$ 且为最后一个符号，则将  $\varepsilon$  加入 $S$ 的 $FIRST$ 集
  - (3) 如果 (2) 中在任何一个非终结符的 $FIRST$ 集中新增了元素，则重复执行 (2)，否则就说明所有符号的 $FIRST$ 集均已求出，结束执行

```cpp
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
			// 如果右部第一个符号是空或者是终结符，则加入到左部的 FIRST 集中
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
				bool next = true;	// 如果当前符号可以推出空，则还需判
									// 断下一个符号 
				size_t idx = 0;		// 待判断符号的下标

				while (next && idx != right.size()) {
					next = false;
					sym_t n = right[idx];	// 当前处理的右部非终结符

					for (auto it = first[n].begin(); it != first[n].end(); ++it) {
						// 把当前符号的 FIRST 集中非空元素加入
						// 到左部符号的 FIRST 集中
						if (*it != Production::epsilon
							&& first[left].find(*it) == first[left].end()) {
							change = true;
							first[left].insert(*it);
						}
					}
					// 当前符号的 FIRST 集中有空, 标记 next 为真，
					// idx 下标+1
					if (first[n].find(Production::epsilon) != first[n].end()) {
						if (idx + 1 == right.size()
							&& first[left].find(Production::epsilon) == first[left].end())
						{
							// 此时说明产生式左部可以推出空；因此需要
							// 把 epsilon 加入 FIRST 集
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
```
- `Grammer` 类同样提供了 `first_set_string()` 用于求一个字符串的 $FIRST$ 集，思路如下：
  -  如果遇到终结符 / 空规则，加入 $FIRST$ 集；如果遇到非终结符，需要把该非终结符的 $FIRST$ 集中除了空规则之外的符号都加入  $FIRST$ 集，若该非终结符如果可以推出空规则，则要查看下一个符号，否则就结束执行，如果该非终结符可以推出空规则且为最后一个符号，则将 $\varepsilon$ 加入 $FIRST$ 集
### $LR(1)$ 项目、$LR(1)$ 项集、$LR(1)$ 项集族
- 我用 `LR1Item` 类 (继承自产生式类 `Production`) 表示一个 LR1 项目，而 LR(1) 项集用 `unordered_set<LR1Item>` 表示，通过类型别名定义为 `LR1_items_t`；LR(1) 项集族用 `vector<LR1_items_t>` 表示
  - `LR1Item` 具有产生式左部 `left`、产生式右部 `right`、向前看符号 `next`、项目中点的位置 `loc` 四个属性
  - 同时通过特例化 `hash` 模板类，定义了  `LR1Item` (LR1 项目) 以及 `LR1_items_t` (LR(1) 项集) 的哈希函数，便于之后利用 `unordered_set` 高效实现在 LR(1) 项集中插入 LR(1) 项目以及在 LR(1) 规范项集族中插入 LR(1) 项集 (避免 LR(1) 项集中加入重复的  LR(1) 项目 或者 LR(1) 规范项集族中加入重复的 LR(1) 项集而导致算法无限循环)
### $Closure()$、$Goto()$
- 这两个函数均由 `LR1DFA` 类提供
***
$Closure()$ 用于对一个 LR1 项集求闭包，具体算法步骤如下：
- (1) 设置一个栈 $s$ 用于记录待处理的 LR1 项目，将原 LR1 项集中的所有项目都压入栈中。当 $s$ 非空时，执行 (2)
- (2) 从 $s$ 中取出一个 LR1 项目加入闭包中 (由于闭包是由 `unordered_set` 实现的，因此可以保证最后求得的闭包中没有重复的 LR1 项目)
  - 如果该 LR1 项目对应的产生式形式为 $[A→α.Bβ, u]$，则对每个 $B→ \gamma$ 的产生式，对于 $FIRST(βu)$ 中的每一个终极符 $b$，如果 $[B→.\gamma,b]$ 不在已求出的闭包内，则将其入栈

```cpp
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
```

***
$Goto()$ 为构造转换函数，具体算法步骤如下：
  - (1) 将 $J$ 初始化为空集；对 $I$ 中的每个项 $[A→α.Xβ, a]$，将项 $[A→αX.β, a]$ 加入到集合 $J$
  - (2) $GOTO(I, X) =closure(J)$，其中 $I$ 是任一项目集，$X$ 是文法符号

```cpp
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
```
### $ACTION$ 表、$GOTO$ 表的数据结构
- $ACTION$ 表、$GOTO$ 表的数据结构均为 `vector<unordered_map<sym_t, string>>`
  - 可以用索引取得每个 LR1 规范项集对应的 ACTION 和 GOTO 动作，例如 `action_table[0]['{']` 就表示第 0 个 LR1 规范项集在识别出 '`{`' 后需要执行的 ACTION 动作
  - 而具体的 ACTION、GOTO 动作均用 `string` 表示；例如，ACTION 表中的移进符号并转移到状态 3 用字符串 `S3` 表示，使用第 3 条产生式归约则用字符串 `R3` 表示；GOTO 表中的动作统统用数字表示


### 构建 $ACTION$ 表、$GOTO$ 表
- 构建 $ACTION$ 表、$GOTO$ 表的算法由 `LR1Processor` 类的 `construct_table()` 成员函数实现，是在求 LR1 规范项集族的过程中逐步构建的
***
**算法思路**：
-  LR1 规范项集族初始项目：$closure([S' \rightarrow.S，\#])= C$
- 对 $C$ 中的每个项集 $I$
  - 如果该项集中有归约项，就根据其向前看符号将其填入 $ACTION$ 表
  - 对每个终结符 $t$，如果  $GOTO(I,t)$ 非空，则将转换关系填入 $ACTION$ 表，如果其不在 $C$ 中，则将其加入 $C$
  - 对每个非终结符 $n$，如果  $GOTO(I,n)$ 非空，则将转换关系填入 $GOTO$ 表，如果其不在 $C$ 中，则将其加入 $C$
- 重复上一步直到不再有新的项集加入 $C$
***
**记录 LR1 分析表中的冲突项**
- 在输入文法时，只要求输入文法为增广文法，而不强制要求其为 LR1 文法；如果不是 LR1 文法，即构建 LR1 分析表时，发现表中项目发生冲突，则发出 Warning 信息并将对应的 LR1 项集的索引值以及对应的下一个输入符号存入 `ambiguity_terms` 中，之后在 `LR1Processor::solve_ambiguity()` 中对所有冲突项进行集中处理，消除冲突项，使其能够被 LR1 分析法分析

```cpp
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
```

```cpp
/*
*
*如果之后有其他二义性问题需要解决，也应该在该函数中添加相应的功能函数
*/
void LR1Processor::solve_ambiguity()
{
	solve_ambiguity_if();
}
```

### 解决文法二义性
- 我定义的文法中存在二义性文法 $if$-$else$，如下图所示，构建出的 LR1 分析表中存在冲突项。例如，第一个 Warning 中的信息表示第 153 个状态中，当下一个读入符号为 $else$ 时，存在移进-归约冲突，其中 $R_{10}$ 代表用第10条产生式 $stmt\rightarrow C stmt$ 归约，而 $S_{152}$ 代表移进 $else$ 并进入 152 状态
![在这里插入图片描述](https://img-blog.csdnimg.cn/20210108223242839.png#pic_center)
- 解决思路：遍历之前记录的二义性项集 `ambiguity_terms`，如果其中存在移进 - 归约冲突，且向前看符号为 $else$，则强制移进，使 $else$ 能够和 $if$ 就近配对

```cpp
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
```
## 符号表
- 符号表用来存储变量信息。因为我定义的文法比较简单，所以变量的属性只有类型 `type` ($int$ / $real$) 和值 `value` 两个，因此我将变量抽象为了 `Id` 类，而符号表则抽象为 `SymTable` 类
- `SymTable` 类只有一个成员变量 `unordered_map<std::string, Id> table`，因此查表的过程都是利用了哈希函数，给出变量名，就可以以较高的效率对符号表进行相应的查找与插入操作。同时 `SymTable` 类还进一步进行了封装，提供了在符号表中插入新的变量、获取符号表中信息、更改符号表中变量的值等成员函数
***
- 对于块结构语言来说，每进入一个块 (读入 “`{`”) 都要新增一个符号表用来存储该块内的变量信息；每退出一个块 (读入 “`}`”) 都要删除该块对应的符号表
- 整个编译过程中使用的所有符号表都保存在一个 `vector<SymTable>` 类型的数据结构中，它保存在类 `LR1Processor` 中；类 `LR1Processor` 同时还提供了两个成员函数用于和符号表进行交互：
  -  `LR1Processor::get_info()` 用于在符号表中通过标识符查找指定变量的信息，需要从最内层块对应的符号表开始，逐层向外查找所需信息，如果所有块对应的符号表中都没有该标识符，则返回 `false` 表示查找失败，否则返回 `true`
  - `LR1Processor::set_val()` 用于更改变量值，过程与 `get_info()` 类似
## 四元式数据结构
- 四元式用结构体 `Quad` 实现，属性有操作符、两个源操作数以及一个目标操作数
- 四元数在语法制导翻译阶段生成

```cpp
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
```

## LR 分析总控程序
- 总控程序由 `LR1Processor::driver()` 实现，它主要负责根据输入符号查找 LR1 分析表，执行分析表所规定的动作，对符号栈、状态栈、属性栈进行操作来完成语法及语义处理
***
**算法步骤**：
- (1) 初始化符号栈、状态栈、属性栈，状态栈压入状态0，符号栈压入$\#$，属性栈压入空属性
- (2) 当输入符号传未读完时，执行 (3)
- (3) 根据状态栈的栈顶状态 $S_m$ 以及下一个输入符号 $a$，查找 LR1 分析表，可能有如下情况：
  - **移进**：$ACTION[S_m,a]=S_i$
    - 把 $a$ 移入符号栈顶
    - 把状态 $S_i$ 压入状态栈顶
    - 将 $a$ 相应的属性压入属性栈；这里如果 $a$ 是数值常量的话，压入的属性为它的数值以及对应行号；如果是标识符，压入的属性为它的词素以及对应行号；其余符号均只压入它们的行号
    - 如果 $a$ 为 “`{`”，则表示进入了一个新的块，需要压入一个新的符号表；如果为 “`}`”，则表示退出了一个块，需要弹出一个符号表
  - **归约**： $ACTION[S_m,a]=r_i$，即 用 $G$ 的第 $i$ 条规则 $A\rightarrow β$ 归约，其中 $|β|=n$
    - 从状态栈和符号栈**各弹出 $n$ 个符号**，即 $S_{m-n}$ 为栈顶状态
    - 将 $A$ 移入文法符号栈，$S_i=GOTO[S_{m-n} ,A]$ 压入状态栈顶
    - 调用 `LR1Processor::production_action()` 用于完成产生式相应的语义动作 (该函数之后会说明)
  - **接受**：$ACTION[S_m,\#]=acc$
    - 当输入符号串到达右界符 $\#$ ，且符号栈只有 $\#S$ 时，分析成功结束，输出 “`ACC!`”
    - 调用 `LR1Processor::production_action()` 用于完成第0条产生式相应的语义动作
  - **报错**：$ACTION[S_m,a]=err$
    - 在状态栈的栈顶状态为 $S_m$ 时，如果输入符号为不应该遇到的符号时，即 $ACTION [S_m,a]=空白$，则报错，说明输入符号串有语法错误，调用 `LR1Processor::err_proc()` 进行错误处理  (该函数之后会说明)
***
- 代码略长，因为篇幅关系就不贴出来了
## 错误处理
**短语层次错误恢复**
  - 思路如下：如果出错时，状态栈栈顶状态对应的 LR1 项目集中有一个归约项且向前看符号为 `;`  `}` `)` 这三个分界符中的一个，这种情况说明程序中可能忘写了一个分界符，此时在输入的符号串中直接插入缺少的分界符，并进行相应的归约，同时输出相应的出错信息
  - 在 LR1 分析时进行短语层次错误恢复还是比较简单的，如果有更多时间的话还可以针对更多的 LR1 项集进行相应的错误恢复
  - 通过错误恢复，编译程序可能通过一次编译发现更多的错误

**无法错误恢复**
- 当无法进行错误恢复时，输出相应的错误信息 (包括出错的行号、出错的状态号以及可能的合法符号)，之后就直接抛出异常结束编译程序的运行

## 语法制导翻译
- 语法制导翻译由 `LR1Processor::production_action()` 提供，该成员函数通过形参提供的产生式编号执行产生式相应的语义动作，生成四元式
- 执行的语义动作与之前在源语言介绍中的语义动作完全一致，在具体的实现中需要操作属性栈，即从属性栈中弹出产生式右部文法符号对应的属性，将这些属性经过语义动作计算新的属性 (产生式左部符号的属性) 压入属性栈
***
- 程序中我利用 `switch-case` 语句进行语义动作的执行：部分代码示例如下，每一个 `case` 分支都对应相应的产生式动作，可读性较高，也便于修改：

```cpp
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
// ...
```
# 程序使用方法
## 使用前注意
- **注意**：在使用之前要先把存放文法的文件 `grammer.txt` 以及所有源文件 `xxx.txt` 放入一个文件夹中，同时**一定要更改文件夹路径**；即修改 `main` 函数中的 `path_prefix`，例如我现在的源文件 `code.txt` 放在  `E:\\Workspace\\diy_compiler\\compiler\\Debug\\` 目录下，那修改 `path_prefix` 为该路径即可
## UI 界面
- 我制作了一个简单的 UI 界面，可以在命令行窗口里输入 `lianli xxx.txt` 来编译相应的源文件，命令末尾加上 `-v` 命令可以输出归约时使用的产生式，输出顺序为最右推导的逆序，效果如下图所示：
![在这里插入图片描述](https://img-blog.csdnimg.cn/20210109234732545.png#pic_center)
## 扩充文法
- 我在程序中留有的接口可以方便的**对源语言文法进行扩充**：
  - 首先在 `grammer.txt` 中添加相应的产生式 (最好添加在最后，这样不会改变其他产生式的序号)
  - 再根据新添加的产生式对应的序号，在 `LR1Processor::production_action` 函数的相应 `case` 分支下添加语义动作即可
  - 其余的工作均由程序自动完成

# 测试样例
- 所有样例文件都和程序打包在一起了
## 样例一
- 样例一为**正确的源程序**

```cpp
// code.txt
{
	int a, b;
	real c, d;
	int e, g, h;
	a = 1.3e3;
	
	if !(a * -b >= c + d)
	then
	{
		e = g + h;
	}
	else
		e = g * h;

	do{
		a = 1;
	}while (true);
	{
		int a;
		a = 1;
	}
	if (1)
	then a = 3;

	while a >= 10
	do
	{
		if (a == a * b)
		then
			a = a - 1;
	}
}
```

**output:**
- 首先输出的是词法分析结果，输出每一个词法单元以及它们的行数
- 如果在输入命令时后面加上 `-v`，还会输出语法分析时所使用的产生式，即输出最右推导的逆序列，但是因为输出信息太多，所以这里没加
- 然后输出 Warning 信息，因为是正确的源程序，所以没有 Warning
- 最后输出四元式
```cpp
>>> lianli code.txt
line1: <{>
line2: <KEY_WORD,int>
line2: <ID,a>
line2: <,>
line2: <ID,b>
line2: <;>
line3: <KEY_WORD,real>
line3: <ID,c>
line3: <,>
line3: <ID,d>
line3: <;>
line4: <KEY_WORD,int>
line4: <ID,e>
line4: <,>
line4: <ID,g>
line4: <,>
line4: <ID,h>
line4: <;>
line5: <ID,a>
line5: <=>
line5: <NUM,1300>
line5: <;>
line7: <KEY_WORD,if>
line7: <!>
line7: <(>
line7: <ID,a>
line7: <*>
line7: <->
line7: <ID,b>
line7: <RELOPT,>=>
line7: <ID,c>
line7: <+>
line7: <ID,d>
line7: <)>
line8: <KEY_WORD,then>
line9: <{>
line10: <ID,e>
line10: <=>
line10: <ID,g>
line10: <+>
line10: <ID,h>
line10: <;>
line11: <}>
line12: <KEY_WORD,else>
line13: <ID,e>
line13: <=>
line13: <ID,g>
line13: <*>
line13: <ID,h>
line13: <;>
line15: <KEY_WORD,do>
line15: <{>
line16: <ID,a>
line16: <=>
line16: <NUM,1>
line16: <;>
line17: <}>
line17: <KEY_WORD,while>
line17: <(>
line17: <KEY_WORD,true>
line17: <)>
line17: <;>
line18: <{>
line19: <KEY_WORD,int>
line19: <ID,a>
line19: <;>
line20: <ID,a>
line20: <=>
line20: <NUM,1>
line20: <;>
line21: <}>
line22: <KEY_WORD,if>
line22: <(>
line22: <NUM,1>
line22: <)>
line23: <KEY_WORD,then>
line23: <ID,a>
line23: <=>
line23: <NUM,3>
line23: <;>
line25: <KEY_WORD,while>
line25: <ID,a>
line25: <RELOPT,>=>
line25: <NUM,10>
line26: <KEY_WORD,do>
line27: <{>
line28: <KEY_WORD,if>
line28: <(>
line28: <ID,a>
line28: <RELOPT,==>
line28: <ID,a>
line28: <*>
line28: <ID,b>
line28: <)>
line29: <KEY_WORD,then>
line30: <ID,a>
line30: <=>
line30: <ID,a>
line30: <->
line30: <NUM,1>
line30: <;>
line31: <}>
line32: <}>
***********************************
No Warning!

************************************

四元式:
1: ( rtoi, 1300.000000, _, t0 )
2: ( =, t0, _, ENTRY(a) )
3: ( @, ENTRY(b), _, t1 )
4: ( *, ENTRY(a), t1, t2 )
5: ( +, ENTRY(c), ENTRY(d), t3 )
6: ( >=, t2, t3, t4 )
7: ( jtrue, t4, _, 9 )
8: ( j, _, _, 9 )
9: ( !, t4, _, t5 )
10: ( jtrue, t5, _, 12 )
11: ( j, _, _, 15 )
12: ( +, ENTRY(g), ENTRY(h), t6 )
13: ( =, t6, _, ENTRY(e) )
14: ( j, _, _, 17 )
15: ( *, ENTRY(g), ENTRY(h), t7 )
16: ( =, t7, _, ENTRY(e) )
17: ( rtoi, 1.000000, _, t8 )
18: ( =, t8, _, ENTRY(a) )
19: ( jtrue, true, _, 21 )
20: ( j, _, _, 21 )
21: ( jtrue, true, _, 17 )
22: ( j, _, _, 23 )
23: ( rtoi, 1.000000, _, t9 )
24: ( =, t9, _, ENTRY(a) )
25: ( jtrue, 1.000000, _, 27 )
26: ( j, _, _, 27 )
27: ( jtrue, 1.000000, _, 29 )
28: ( j, _, _, 31 )
29: ( rtoi, 3.000000, _, t10 )
30: ( =, t10, _, ENTRY(a) )
31: ( >=, ENTRY(a), 10.000000, t11 )
32: ( jtrue, t11, _, 34 )
33: ( j, _, _, 45 )
34: ( *, ENTRY(a), ENTRY(b), t12 )
35: ( ==, ENTRY(a), t12, t13 )
36: ( jtrue, t13, _, 38 )
37: ( j, _, _, 38 )
38: ( jtrue, t13, _, 40 )
39: ( j, _, _, 31 )
40: ( itor, ENTRY(a), _, t14 )
41: ( -, t14, 1.000000, t15 )
42: ( rtoi, t15, _, t16 )
43: ( =, t16, _, ENTRY(a) )
44: ( j, _, _, 31 )
45: ( End, _, _, _ )
```

## 样例二
- 样例二中科学计数法的表示存在错误，`e` 后面必须跟一个数字
```cpp
// code1.txt
{
	int a, b;
	real c, d;
	int e, g, h;
	a = 1.3e;	// 词法分析阶段报错
	
	if !(a * -b >= c + d)
	then
	{
		e = g + h;
	}
	else
		e = g * h;

	while a >= 10
	do
	{
		if (a == a * b)
		then
			a = a - 1;
		b = b + 1
	}
	{
		int a;
		a = 1;
	}
	do{
		a = 1;
	}while (true);

	if (1
	then a = 3
}

```
**output:**


```cpp
>>> lianli code1.txt
line1: <{>
line2: <KEY_WORD,int>
line2: <ID,a>
line2: <,>
line2: <ID,b>
line2: <;>
line3: <KEY_WORD,real>
line3: <ID,c>
line3: <,>
line3: <ID,d>
line3: <;>
line4: <KEY_WORD,int>
line4: <ID,e>
line4: <,>
line4: <ID,g>
line4: <,>
line4: <ID,h>
line4: <;>
line5: <ID,a>
line5: <=>
************************************
ERROR!
Line5: No number after 'e' !
************************************
>>>
```
## 样例三
- 样例三中存在多重定义的错误，该错误将在语义分析阶段报错
```cpp
// code2.txt
{
	int a, b;
	real c, d;
    real a;     // 多重定义
	int e, g, h;
	a = 1.3e3;
	
	if !(a * -b >= c + d)
	then
	{
		e = g + h;
	}
	else
		e = g * h;

	while a >= 10
	do
	{
		if (a == a * b)
		then
			a = a - 1;
		b = b + 1
	}
	{
		int a;
		a = 1;
	}
	do{
		a = 1;
	}while (true);

	if (1
	then a = 3
}
```
**output:**

```cpp
>>> lianli code2.txt
line1: <{>
// 省略了部分词法分析输出
line34: <}>
************************************
ERROR!
Line4: multiple definition!
************************************
>>>
```
## 样例四
- 样例四中使用了变量 `a`，但却没有定义，因此存在无定义错误

```cpp
// code3.txt
{
	int b;  // 无定义
	real c, d;
	int e, g, h;
	a = 1.3e3;
	
	if !(a * -b >= c + d)
	then
	{
		e = g + h;
	}
	else
		e = g * h;

	while a >= 10
	do
	{
		if (a == a * b)
		then
			a = a - 1;
		b = b + 1
	}
	{
		int a;
		a = 1;
	}
	do{
		a = 1;
	}while (true);

	if (1
	then a = 3
}
```
**output:**

```cpp
>>> lianli code3.txt
line1: <{>
// 省略了部分词法分析输出
line33: <}>
************************************
ERROR!
Line5: Undefined variant a !
************************************
```
## 样例五
- 样例五在程序中除以了0，会产生除以0的报错，该报错在语义分析阶段完成

```cpp
// code4.txt
{
	int a, b;
	real c, d;
	int e, g, h;
	a = 1.3e3 / 0;  // 除以0
	
	if !(a * -b >= c + d)
	then
	{
		e = g + h;
	}
	else
		e = g * h;

	while a >= 10
	do
	{
		if (a == a * b)
		then
			a = a - 1;
		b = b + 1
	}
	{
		int a;
		a = 1;
	}
	do{
		a = 1;
	}while (true);

	if (1
	then a = 3
}
```
**output:**

```cpp
>>> lianli code4.txt
line1: <{>
// 省略了部分词法分析输出
line33: <}>
************************************
ERROR!
Line5: Err: Divided by zero!
************************************
```
## 样例六
- 样例六中的错误为部分语句缺少 `)` / `;` / `}` 这样的分界符，在这种情况下会在语法分析阶段报错。但我使用了错误恢复，因此会自动在程序输入流程中插入缺少的分界符，最后依然能够生成正确的四元式，但会产生相应的 Warning，提示哪一行没有加分界符

```cpp
// code5.txt
{
	int a, b;
	real c, d;
	int e, g, h;
	a = 1.3e3;
	
	if !(a * -b >= c + d    // 缺少右圆括号
	then
	{
		e = g + h;
	}
	else
		e = g * h;

	do{
		a = 1;
	}while (true)   // 缺少分号
	{
		int a;
		a = 1;      // 缺少右大括号
            
	if (1)
	then a = 3;

	while a >= 10
	do
	{
		if (a == a * b)
		then
			a = a - 1;  // 缺少右大括号
    
}
```
**output:**
- 可以看到，最后输出的四元式与样例一中生成的四元式相同
```cpp
>>> lianli code5.txt
line1: <{>
// 省略了部分词法分析输出
line32: <}>
***********************************

Line7:
Warning 0: Lack of )

Line17:
Warning 1: Lack of ;

Line32:
Warning 2: Lack of }

Line32:
Warning 3: Lack of }

4 Warning(s)!
************************************

四元式:
1: ( rtoi, 1300.000000, _, t0 )
2: ( =, t0, _, ENTRY(a) )
3: ( @, ENTRY(b), _, t1 )
4: ( *, ENTRY(a), t1, t2 )
5: ( +, ENTRY(c), ENTRY(d), t3 )
6: ( >=, t2, t3, t4 )
7: ( jtrue, t4, _, 9 )
8: ( j, _, _, 9 )
9: ( !, t4, _, t5 )
10: ( jtrue, t5, _, 12 )
11: ( j, _, _, 15 )
12: ( +, ENTRY(g), ENTRY(h), t6 )
13: ( =, t6, _, ENTRY(e) )
14: ( j, _, _, 17 )
15: ( *, ENTRY(g), ENTRY(h), t7 )
16: ( =, t7, _, ENTRY(e) )
17: ( rtoi, 1.000000, _, t8 )
18: ( =, t8, _, ENTRY(a) )
19: ( jtrue, true, _, 21 )
20: ( j, _, _, 21 )
21: ( jtrue, true, _, 17 )
22: ( j, _, _, 23 )
23: ( rtoi, 1.000000, _, t9 )
24: ( =, t9, _, ENTRY(a) )
25: ( jtrue, 1.000000, _, 27 )
26: ( j, _, _, 27 )
27: ( jtrue, 1.000000, _, 29 )
28: ( j, _, _, 31 )
29: ( rtoi, 3.000000, _, t10 )
30: ( =, t10, _, ENTRY(a) )
31: ( >=, ENTRY(a), 10.000000, t11 )
32: ( jtrue, t11, _, 34 )
33: ( j, _, _, 45 )
34: ( *, ENTRY(a), ENTRY(b), t12 )
35: ( ==, ENTRY(a), t12, t13 )
36: ( jtrue, t13, _, 38 )
37: ( j, _, _, 38 )
38: ( jtrue, t13, _, 40 )
39: ( j, _, _, 31 )
40: ( itor, ENTRY(a), _, t14 )
41: ( -, t14, 1.000000, t15 )
42: ( rtoi, t15, _, t16 )
43: ( =, t16, _, ENTRY(a) )
44: ( j, _, _, 31 )
45: ( End, _, _, _ )
```
## 样例七
- 样例七展示了一般错误的处理情况：输出错误的行数、表明在读入哪一个符号时出错、以及可能的正确输入符号
- 源程序中的错误为缺少一个运算符

```cpp
// code6.txt
{
	int a, b;
	real c, d;
	int e, g, h;
	a = 1.3e3;
	
	if !(a * -b >= c + d)
	then
	{
		e = g + h;
	}
	else
		e = g * h;

	do{
		a = 1;
	}while (true);
	{
		int a;
		a = 1;
	}
	if (1)
	then a = 3;

	while a >= 10
	do
	{
		if a == a  b	// 缺少运算符
		then
			a = a - 1;
	}
}
```
**output:**
- 可以看到，提供的可能正确输入符号中包含了各种算逻运算符以及结束 `if` 语句的 `then`
```cpp
>>> lianli code6.txt
line1: <{>
// 省略了部分词法分析输出
line32: <}>
************************************
ERROR!
Line28:
state 32 can't accept id !
possible symbols are:
<= / then >= + - * or and == != < >
************************************
```

