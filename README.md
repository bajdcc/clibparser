# clibparser（GLR Parser）

C++实现的LR Parser Generator，正在设计中。

## 文章

暂无

## 功能

- 词法分析阶段由lexer完成，返回各类终结符。
- 语法分析阶段由parser完成，输入产生式，通过生成LALR表来进行分析。

文法支持顺序、分支、可选。

## 调试信息

先实现四则运算的解析。

生成NGA图，去EPSILON化，未去重复边及优化。

```cpp
==== RULE ====
exp0 => [ exp0 ( '+' | '-' ) ] exp1
exp1 => [ exp1 ( '*' | '/' ) ] exp2
exp2 => #int#
root => exp0
==== NGA  ====
** Rule: exp0
Status #0 - exp0 => [ @ exp0 ( '+' | '-' ) ] exp1
  To #1:  exp0
  To #2:  exp1
Status #1 - exp0 => [ exp0 @ ( '+' | '-' ) ] exp1
  To #3:  '-'
  To #4:  '+'
Status #2 [FINAL] - exp0 => [ exp0 ( '+' | '-' ) ] exp1 @
Status #3 - exp0 => [ exp0 ( '+' | '-' @ ) ] exp1
  To #2:  exp1
Status #4 - exp0 => [ exp0 ( '+' @ | '-' ) ] exp1
  To #2:  exp1

** Rule: exp1
Status #0 - exp1 => [ @ exp1 ( '*' | '/' ) ] exp2
  To #1:  exp1
  To #2:  exp2
Status #1 - exp1 => [ exp1 @ ( '*' | '/' ) ] exp2
  To #3:  '/'
  To #4:  '*'
Status #2 [FINAL] - exp1 => [ exp1 ( '*' | '/' ) ] exp2 @
Status #3 - exp1 => [ exp1 ( '*' | '/' @ ) ] exp2
  To #2:  exp2
Status #4 - exp1 => [ exp1 ( '*' @ | '/' ) ] exp2
  To #2:  exp2

** Rule: exp2
Status #0 - exp2 => @ #int#
  To #1:  #int#
Status #1 [FINAL] - exp2 => #int# @

** Rule: root
Status #0 - root => @ exp0
  To #1:  exp0
Status #1 [FINAL] - root => exp0 @
```

## 使用

```cpp
void cparser::gen() {
    auto &program = unit.rule("root");
    auto &exp0 = unit.rule("exp0");
    auto &exp1 = unit.rule("exp1");
    auto &exp2 = unit.rule("exp2");
    auto &plus = unit.token(op_plus);
    auto &minus = unit.token(op_minus);
    auto &times = unit.token(op_times);
    auto &divide = unit.token(op_divide);
    auto &integer = unit.token(l_int);
    program = exp0;
    exp0 = *(exp0 + (plus | minus)) + exp1;
    exp1 = *(exp1 + (times | divide)) + exp2;
    exp2 = integer;
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
- [x] 生成非确定性文法自动机
    - [x] 闭包求解
    - [x] 去Epsilon
    - [x] 打印NGA结构
- [ ] 生成下推自动机
    - [x] 求First集合
    - [x] 检查文法有效性（如不产生Epsilon）
    - [ ] 检查纯左递归
    - [ ] 生成PDA
    - [ ] 打印PDA结构

1. 将文法树转换成图（进行中，NGA部分已完成）

## 改进

- [ ] 生成LR项目集时将@符号提到集合的外面，减少状态

## 参考

1. [CParser](https://github.com/bajdcc/CParser)
2. [CMiniLang](https://github.com/bajdcc/CMiniLang)
3. [vczh GLR Parser](https://github.com/vczh-libraries/Vlpp/tree/master/Source/Parsing)