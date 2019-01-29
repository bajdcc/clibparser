# clibparser（C++ GLR Parser and VM）

C++实现的**LR编译器**及**C语言虚拟机**。代码量：约11k。

- 文法书写方式：以C++重载为基础的Parser Generator。
- 语义分析：在LR解析的过程中，状态机向符号表提供分析接口，使得状态转换可手动干涉，解决“`A*b`”问题
- 识别方式：**以下推自动机为基础，向看查看一个字符、带回溯的LR分析**。
- 内存管理：自制内存池。

**仿制了exec和fork调用，自动运行code文件夹下的代码。**

![img](https://raw.githubusercontent.com/bajdcc/clibparser/master/screenshots/2.gif)

**【管道示例】**

![img](https://raw.githubusercontent.com/bajdcc/clibparser/master/screenshots/3.gif)

## 计划

- 第一阶段：实现基本的C++编译器和虚拟机，支持控制流语句，模拟虚页机制。后续支持结构体、指针和汇编。
- 第二阶段：实现最小化标准库，搭建虚拟操作系统。
- 第三阶段：实现图形化用户界面，制作桌面。

## 文章

- [【Parser系列】实现LR分析——开篇](https://zhuanlan.zhihu.com/p/52478414)
- [【Parser系列】实现LR分析——生成AST](https://zhuanlan.zhihu.com/p/52528516)
- [【Parser系列】实现LR分析——支持C语言文法](https://zhuanlan.zhihu.com/p/52812144)
- [【Parser系列】实现LR分析——完成编译器前端！](https://zhuanlan.zhihu.com/p/53070412)
- [【Parser系列】实现LR分析——识别变量声明](https://zhuanlan.zhihu.com/p/54716082)
- [【Parser系列】实现LR分析——解决二义性问题](https://zhuanlan.zhihu.com/p/54766453)
- [【虚拟机系列】C语言虚拟机诞生！](https://zhuanlan.zhihu.com/p/55319251)
- [【虚拟机系列】实现Fork！](https://zhuanlan.zhihu.com/p/55613027)

## 功能

- 词法分析阶段由lexer完成，返回各类终结符。
- 语法分析阶段由parser完成，输入产生式，通过生成LALR表来进行分析。
- **LR识别完成，生成AST完成。**
- **[C语言文法](https://github.com/antlr/grammars-v4/blob/master/c/C.g4)基本实现！请参阅cparser.cpp！支持多种基本数据类型。**
- 虚拟机指令生成。
- 图形界面初始化。

文法支持顺序、分支、可选、跳过终结符。目前可以根据LR文法**自动**生成AST。后续会对AST进行标记。

目前已完成：

1. Shell
2. 测试用例

## 示例代码

代码均在[code文件夹](https://github.com/bajdcc/clibparser/tree/master/code)下，运行时从该文件夹中读取C文件。

Shell界面：

![img](https://raw.githubusercontent.com/bajdcc/clibparser/master/screenshots/1.png)

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

虚拟机的调试，修改cvm.cpp中的：

```cpp
#define LOG_INS 0
#define LOG_STACK 0
```

将值改为1即可。

## 测试用例

采用[Antlr 4 - C test](https://github.com/antlr/grammars-v4/tree/master/c/examples)中的9个c文件。

测试文件在test.cpp中，编译为clibparser-test。

**9个测试用例均通过。**

## 目标

当前进展：

- [x] 词法分析（LL手写识别）
   - [x] 识别数字（科学计数+十六进制）
   - [x] 识别变量名
   - [x] 识别空白字符
   - [x] 识别字符（支持所有转义）
   - [x] 识别字符串（支持所有转义）
   - [x] 识别数字（除去负号）
   - [x] 识别注释
   - [x] 识别关键字
   - [x] 识别操作符
   - [x] 错误处理
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
- [ ] 宏
    - [x] include指令（包含代码）
- [ ] 语义分析
    - [x] 识别重名
    - [x] 输出错误单词的位置
    - [x] 基本类型（单一类型及其指针，不支持枚举、函数指针和结构体）
    - [x] 识别变量声明（类型可为结构体）
    - [x] 识别函数声明（识别返回类型、参数）
    - [x] 识别结构体声明和类型
    - [x] 计算变量声明地址
    - [x] 识别语句
    - [x] 识别表达式
- [ ] 代码生成
    - [x] 全局变量及初始化
    - [x] 局部变量及初始化
    - [x] 形参
    - [x] 函数调用（及递归）
    - [x] 数组寻址（及左值），多维数组定义，一维数组使用
    - [x] 枚举
    - [ ] 结构体成员（点和指针）
    - [x] 一元运算
    - [x] 二元运算
    - [x] 短路运算
    - [x] 三元运算
    - [x] 赋值语句
    - [x] 返回语句
    - [x] if语句
    - [x] for语句
    - [x] sizeof
    - [x] while语句和do..while语句
    - [x] switch语句（default和case）
    - [x] break和continue
    - [x] 取址和解引用（及左值）
    - [x] 强制类型转换
- [x] 虚拟机
    - [x] 设计指令（寄存器式分配）
    - [x] 设计符号表
    - [x] 解析类型系统
    - [x] 生成指令
    - [x] 虚页分配（已实现）
    - [ ] 虚页权限管理
    - [x] 运行指令
    - [x] 中断指令
    - [ ] 扩展指令
- [ ] 操作系统
    - [x] 多任务机制
    - [x] 读取并执行C文件
    - [x] exec、wait、fork调用
    - [x] 命令行参数
    - [x] 用户态禁止修改代码段
    - [x] 只有代码段可以执行指令
    - [x] 键盘输入接口
    - [x] 句柄管理
    - [ ] 共享代码区
    - [ ] 内核与用户态分离
    - [ ] 内存权限管理
    - [ ] 中断机制
    - [ ] 虚拟文件系统
    - [x] 命令行管道
- [ ] 库代码文件
    - [x] exec
    - [x] fs
    - [x] io
    - [x] memory
    - [x] proc
    - [x] string
    - [x] sys
- [ ] 用户例程
    - [x] Shell(`sh`)
    - [x] 测试用例(`/usr/test`)
    - [x] echo
    - [x] pwd, whoami
    - [x] VFS: cd, mkdir, touch, cat
- [ ] 虚拟文件系统
    - [x] 数据结构
    - [x] 账户
    - [x] 读取
    - [x] 操作
    - [ ] 存储
    - [ ] 权限
    - [ ] 互斥
    - [ ] 链接
- [ ] 图形用户界面
    - [x] 用OpenGL创建窗口
    - [x] 实现控制台输出接口
    - [x] 延时功能
    - [x] 支持设置缓冲区大小
    - [x] 输入功能
    - [x] 键盘输入（任务独占）

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
- [x] 解决了字符串转义的问题
- [ ] 指针解引用的问题
- [x] 用户键盘输入交互功能
- [x] 解决命令行参数的读取问题

## 参考

1. [CParser](https://github.com/bajdcc/CParser)
2. [CMiniLang](https://github.com/bajdcc/CMiniLang)
3. [vczh GLR Parser](https://github.com/vczh-libraries/Vlpp/tree/master/Source/Parsing)
4. [antlr/grammars-v4 C.g4](https://github.com/antlr/grammars-v4/blob/master/c/C.g4)