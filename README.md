# clibparser（GLR Parser）

C++实现的LR Parser Generator，正在设计中。

## 文章

暂无

## 功能

- 词法分析阶段由lexer完成，返回各类终结符。
- 语法分析阶段由parser完成，输入产生式，通过生成LALR表来进行分析。

## 调试信息

```cpp
==== RULE ====
root => '+' '*' | '+' | '*'
```

## 使用

```cpp
void cparser::gen() {
    auto &program = unit.rule("root");
    auto &plus = unit.token(op_plus);
    auto &times = unit.token(op_times);
    program = plus + times | plus | times;
    unit.dump(std::cout);
}
```

## 测试用例

暂无

## 目标

1. 将文法树转换成图

## 改进

暂无

## 参考

1. [CParser](https://github.com/bajdcc/CParser)
2. [CMiniLang](https://github.com/bajdcc/CMiniLang)
3. [vczh GLR Parser](https://github.com/vczh-libraries/Vlpp/tree/master/Source/Parsing)