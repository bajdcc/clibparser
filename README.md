# clibparser（GLR Parser）

C++实现的LR Parser Generator。

- 文法书写方式：以C++重载为基础的Parser Generator。
- 识别方式：**以下推自动机为基础，向看查看一个字符、带回溯的LR分析**。
- 内存管理：自制内存池。

## 文章

- [【Parser系列】实现LR分析——开篇](https://zhuanlan.zhihu.com/p/52478414)
- [【Parser系列】实现LR分析——生成AST](https://zhuanlan.zhihu.com/p/52528516)
- [【Parser系列】实现LR分析——支持C语言文法](https://zhuanlan.zhihu.com/p/52812144)

## 功能

- 词法分析阶段由lexer完成，返回各类终结符。
- 语法分析阶段由parser完成，输入产生式，通过生成LALR表来进行分析。
- **LR识别完成，生成AST完成。**
- **[C语言文法](https://github.com/antlr/grammars-v4/blob/master/c/C.g4)基本实现！请参阅cparser.cpp！支持多种基本数据类型。**

文法支持顺序、分支、可选。目前可以根据LR文法**自动**生成AST。后续会对AST进行标记。

## 顶层调用

```cpp
int main() {
    using namespace clib;
    try {
        cparser p(R"(
int main(int (*g)()) {
    int a, b, c;
    float d, e, f;
}
)");
        auto root = p.parse();
        cast::print(root, 0, std::cout);
    } catch (const cexception &e) {
        std::cout << "RUNTIME ERROR: " << e.msg << std::endl;
    }
    return 0;
}
```

结果：
```txt
((((((int))) ((main ( ((((((int))) ((( ((*) (g)) ) ( )))))) ))) ({ ((((((int))) ((((a))) , (((b))) , (((c)))) ;)) (((((f
loat))) ((((d))) , (((e))) , (((f)))) ;))) }))))
```
结果未去括号。

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

暂无

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
    - [ ] 美化AST结构
    - [ ] 语义动作
- [x] 设计语言
    - [x] 使用[C语言文法](https://github.com/antlr/grammars-v4/blob/master/c/C.g4)
    - [x] 实现回溯，解决移进/归约冲突问题，解决回溯的诸多BUG

1. 将文法树转换表（完成）
2. 根据PDA表生成AST（完成）

## 改进

- [ ] 生成LR项目集时将@符号提到集合的外面，减少状态
- [x] PDA表的生成时使用了内存池来保存结点，当生成PDA表后，内存池可以全部回收
- [x] 生成AST时减少嵌套结点
- [ ] 优化回溯时产生的数据结构，减少拷贝
- [ ] 解析成功时释放结点内存

## 参考

1. [CParser](https://github.com/bajdcc/CParser)
2. [CMiniLang](https://github.com/bajdcc/CMiniLang)
3. [vczh GLR Parser](https://github.com/vczh-libraries/Vlpp/tree/master/Source/Parsing)
4. [antlr/grammars-v4 C.g4](https://github.com/antlr/grammars-v4/blob/master/c/C.g4)