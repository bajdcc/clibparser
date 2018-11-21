# clibparser（GLR Parser）

C++实现的LR Parser Generator，正在设计中。

## 文章

暂无

## 功能

- 词法分析阶段由lexer完成，返回各类终结符。
- 语法分析阶段由parser完成，输入产生式，通过生成LALR表来进行分析。

## 调试信息

```cpp
root => @ '+' '*' root | '+' | '*'
root => '+' '*' root | '+' | '*' @
root => @ '+' '*' root | '+' | '*'
root => '+' @ '*' root | '+' | '*'
root => '+' @ '*' root | '+' | '*'
root => '+' '*' @ root | '+' | '*'
root => '+' '*' @ root | '+' | '*'
root => '+' '*' root @ | '+' | '*'
root => '+' '*' root | @ '+' | '*'
root => '+' '*' root | '+' @ | '*'
root => '+' '*' root | '+' | @ '*'
root => '+' '*' root | '+' | '*' @
==== RULE ====
root => '+' '*' root | '+' | '*'
```

## 使用

```cpp
void cparser::gen() {
    auto &program = unit.rule("root");
    auto &plus = unit.token(op_plus);
    auto &times = unit.token(op_times);
    program = plus + times + program | plus | times;
    unit.gen((unit_rule &) program);
    unit.dump(std::cout);
}
```

## 测试用例

暂无

## 目标

当前进展：

- [x] 生成文法表达式
- [x] 生成LR项目
- [ ] 生成非确定性文法自动机

1. 将文法树转换成图（进行中）

## 改进

暂无

## 参考

1. [CParser](https://github.com/bajdcc/CParser)
2. [CMiniLang](https://github.com/bajdcc/CMiniLang)
3. [vczh GLR Parser](https://github.com/vczh-libraries/Vlpp/tree/master/Source/Parsing)