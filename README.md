# clibparser（GLR Parser）

C++实现的LR Parser Generator。

- 文法书写方式：以C++重载为基础的Parser Generator。
- 语义分析：在LR解析的过程中，状态机向符号表提供分析接口，使得状态转换可手动干涉，解决“`A*b`”问题
- 识别方式：**以下推自动机为基础，向看查看一个字符、带回溯的LR分析**。
- 内存管理：自制内存池。

## 文章

- [【Parser系列】实现LR分析——开篇](https://zhuanlan.zhihu.com/p/52478414)
- [【Parser系列】实现LR分析——生成AST](https://zhuanlan.zhihu.com/p/52528516)
- [【Parser系列】实现LR分析——支持C语言文法](https://zhuanlan.zhihu.com/p/52812144)
- [【Parser系列】实现LR分析——完成编译器前端！](https://zhuanlan.zhihu.com/p/53070412)
- [【Parser系列】实现LR分析——识别变量声明](https://zhuanlan.zhihu.com/p/54716082)
- [【Parser系列】实现LR分析——解决二义性问题](https://zhuanlan.zhihu.com/p/54766453)

## 功能

- 词法分析阶段由lexer完成，返回各类终结符。
- 语法分析阶段由parser完成，输入产生式，通过生成LALR表来进行分析。
- **LR识别完成，生成AST完成。**
- **[C语言文法](https://github.com/antlr/grammars-v4/blob/master/c/C.g4)基本实现！请参阅cparser.cpp！支持多种基本数据类型。**
- 虚拟机指令生成。

文法支持顺序、分支、可选、跳过终结符。目前可以根据LR文法**自动**生成AST。后续会对AST进行标记。

## 示例代码

```cpp
struct sx {
    int *a;
    double b;
};
int *a, *_a, _b = 1, _c = "Hello world!";
double *b;
int *main(int c, float *d, sx x, char y) {
    unsigned int *a, *b;
    float d, *e, f;
    sx *s1;
    //sy *s2;
}
int main2() {
    int a, b, c;
    a + b + 1 - 2;
    a = b = c;
    --1+++---1++;
    a.b----[1](1,2);
}
int ta, test(unsigned int a, double b, char c), tb(long d), tc, td(int e);
```

结果（每个单词都保存了在源文件中的位置，这里省略）：
```txt
[DEBUG] Type: int
[DEBUG] Id: int* a, Class: struct id, Addr: 0
[DEBUG] Type: double
[DEBUG] Id: double b, Class: struct id, Addr: 0
[DEBUG] Type: int
[DEBUG] Id: int* a, Class: global id, Addr: 0
[DEBUG] Id: int* _a, Class: global id, Addr: 4
[DEBUG] Id: int _b, Class: global id, Init: (type: int, int: 1), Addr: 8
[DEBUG] Id: int _c, Class: global id, Init: (type: char*, string: "Hello world!"), Addr: 12
[DEBUG] Type: double
[DEBUG] Id: double* b, Class: global id, Addr: 28
[DEBUG] Type: int
[DEBUG] Type: int
[DEBUG] Type: float
[DEBUG] Type: sx
[DEBUG] Type: char
[DEBUG] Func: int* main, Param: [int c, Class: param id, Addr: 0; float* d, Class: param id, Addr: 4; sx x, Class: param
 id, Addr: 8; char y, Class: param id, Addr: 20], Class: func id, Addr: 0
[DEBUG] Type: uint
[DEBUG] Id: uint* a, Class: local id, Addr: 32
[DEBUG] Id: uint* b, Class: local id, Addr: 36
[DEBUG] Type: float
[DEBUG] Id: float d, Class: local id, Addr: 40
[DEBUG] Id: float* e, Class: local id, Addr: 44
[DEBUG] Id: float f, Class: local id, Addr: 48
[DEBUG] Type: sx
[DEBUG] Id: sx* s1, Class: local id, Addr: 52
[DEBUG] *GEN* ==> LEV
[DEBUG] Type: int
[DEBUG] Func: int main2, Param: [], Class: func id, Addr: 1
[DEBUG] Type: int
[DEBUG] Id: int a, Class: local id, Addr: 8
[DEBUG] Id: int b, Class: local id, Addr: 12
[DEBUG] Id: int c, Class: local id, Addr: 16
[DEBUG] Exp: (binop, operator: -, exp1: (binop, operator: +, exp1: (binop, operator: +, exp1: (type: int, id: a), exp2:
(type: int, id: b)), exp2: (type: int, int: 1)), exp2: (type: int, int: 2))
[DEBUG] Exp: (type: int, id: c)
[DEBUG] Exp: (binop, operator: +, exp1: (unop, operator: --, exp: (sinop, operator: ++, exp: (type: int, int: 1))), exp2
: (unop, operator: --, exp: (unop, operator: -, exp: (sinop, operator: ++, exp: (type: int, int: 1)))))
[DEBUG] Exp: (binop, operator: (, exp1: (binop, operator: [, exp1: (sinop, operator: --, exp: (sinop, operator: --, exp:
 (binop, operator: ., exp1: (type: int, id: a), exp2: (type: int, id: b)))), exp2: (type: int, int: 1)), exp2: (list, (t
ype: int, int: 1)))
[DEBUG] *GEN* ==> LEV
[DEBUG] Type: int
[DEBUG] Type: uint
[DEBUG] Type: double
[DEBUG] Type: char
[DEBUG] Func: int test, Param: [uint a, Class: param id, Addr: 0; double b, Class: param id, Addr: 4; char c, Class: par
am id, Addr: 12], Class: func id, Addr: 2
[DEBUG] Type: long
[DEBUG] Func: int tb, Param: [long d, Class: param id, Addr: 0], Class: func id, Addr: 2
[DEBUG] Type: int
[DEBUG] Func: int td, Param: [int e, Class: param id, Addr: 0], Class: func id, Addr: 2
[DEBUG] Id: int ta, Class: global id, Addr: 32
[DEBUG] Id: int tc, Class: global id, Addr: 36
```

## 调试信息

实现C语言的解析。

生成NGA图，去EPSILON化，生成PDA表，生成AST。

以下为下推自动机：

**由于太长，文法定义和下推自动机在根目录下的grammar.txt文件中！**

以下为下推自动机的识别过程（**太长，略**），如需查看，请修改cparser.cpp中的：

```cpp
#define TRACE_PARSING 0
#define DUMP_PDA 0
#define DEBUG_AST 0
#define CHECK_AST 0
```

将值改为1即可。

## 测试用例

采用[Antlr 4 - C test](https://github.com/antlr/grammars-v4/tree/master/c/examples)中的9个c文件。

测试文件在test.cpp中，编译为clibparser-test。

**9个测试用例均通过，AST结果在test.txt中。Release模式下时间不超过1秒。**

## 目标

当前进展：

- [x] 生成文法表达式
- [x] 生成LR项目
- [x] 生成非确定性文法自动机
    - [x] 闭包求解
    - [x] 去Epsilon
    - [x] 打印NGA结构
- [x] 生成下推自动机
    - [x] 求First集合，并输出
    - [x] 检查文法有效性（如不产生Epsilon）
    - [x] 检查纯左递归
    - [x] 生成PDA
    - [x] 打印PDA结构（独立于内存池）
- [x] 生成抽象语法树
    - [x] 自动生成AST结构
    - [x] 美化AST结构
    - [x] 美化表达式树（减少深度）
- [x] 设计语言
    - [x] 使用[C语言文法](https://github.com/antlr/grammars-v4/blob/master/c/C.g4)
    - [x] 实现回溯，解决移进/归约冲突问题，解决回溯的诸多BUG
    - [x] 调整优先级
    - [x] 【关键】解决“`A*b`”问题，部分语义分析嵌入到LR分析之中
- [ ] 语义分析
    - [x] 识别重名
    - [x] 输出错误单词的位置
    - [x] 基本类型（单一类型及其指针，不支持枚举、函数指针和结构体）
    - [x] 识别变量声明（类型可为结构体）
    - [x] 识别函数声明（识别返回类型、参数）
    - [x] 识别结构体声明和类型
    - [x] 计算变量声明地址
    - [ ] 识别语句
    - [ ] 识别表达式
- [ ] 虚拟机
    - [ ] 设计指令（寄存器式分配）
    - [ ] 设计符号表
    - [ ] 解析类型系统
    - [ ] 生成指令
    - [ ] 虚页分配（已实现）
    - [ ] 运行指令

1. 将文法树转换表（完成）
2. 根据PDA表生成AST（完成）

## 改进

- [ ] 生成LR项目集时将@符号提到集合的外面，减少状态
- [x] PDA表的生成时使用了内存池来保存结点，当生成PDA表后，内存池可以全部回收
- [x] 生成AST时减少嵌套结点
- [ ] 优化回溯时产生的数据结构，减少拷贝
- [ ] 解析成功时释放结点内存
- [x] 将集合结点的标记修改成枚举
- [x] 可配置归约与纯左递归的优先级
- [x] LR分析阶段提供语义分析接口
- [x] 美化表达式树（减少深度）

## 参考

1. [CParser](https://github.com/bajdcc/CParser)
2. [CMiniLang](https://github.com/bajdcc/CMiniLang)
3. [vczh GLR Parser](https://github.com/vczh-libraries/Vlpp/tree/master/Source/Parsing)
4. [antlr/grammars-v4 C.g4](https://github.com/antlr/grammars-v4/blob/master/c/C.g4)