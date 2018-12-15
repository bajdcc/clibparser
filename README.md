# clibparser（GLR Parser）

C++实现的LR Parser Generator，正在设计中，PDA表已生成。

## 文章

准备中。

## 功能

- 词法分析阶段由lexer完成，返回各类终结符。
- 语法分析阶段由parser完成，输入产生式，通过生成LALR表来进行分析。
- **LR识别完成，接下来做生成AST功能。**

文法支持顺序、分支、可选。

## 调试信息

先实现四则运算的解析。

生成NGA图，去EPSILON化，生成PDA表。

```cpp
==== RULE ====
exp0 => [ exp0 ( '+' | '-' ) ] exp1
exp1 => [ exp1 ( '*' | '/' ) ] exp2
exp2 => #int#
root => exp0
==== NGA  ====
** Rule: exp0 => [ exp0 ( '+' | '-' ) ] exp1
-- Tokens: #int#
-- First-set tokens: #int#
-- First-set rules: exp1 exp0
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

** Rule: exp1 => [ exp1 ( '*' | '/' ) ] exp2
-- Tokens: #int#
-- First-set tokens: #int#
-- First-set rules: exp2 exp1
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

** Rule: exp2 => #int#
-- Tokens: #int#
-- First-set tokens: #int#
-- First-set rules:
Status #0 - exp2 => @ #int#
  To #1:  #int#
Status #1 [FINAL] - exp2 => #int# @

** Rule: root => exp0
-- Tokens: #int#
-- First-set tokens: #int#
-- First-set rules: exp0
Status #0 - root => @ exp0
  To #1:  exp0
Status #1 [FINAL] - root => exp0 @

==== PDA  ====
** [Initial] State: root => @ exp0

** State #0: root => @ exp0
    --> __________________
    --> #1: exp0 => [ @ exp0 ( '+' | '-' ) ] exp1
    -->     Type: shift, Inst: shift
    -->     LA: #int#

** State #1: exp0 => [ @ exp0 ( '+' | '-' ) ] exp1
    --> __________________
    --> #2: exp1 => [ @ exp1 ( '*' | '/' ) ] exp2
    -->     Type: shift, Inst: shift
    -->     LA: #int#

** State #2: exp1 => [ @ exp1 ( '*' | '/' ) ] exp2
    --> __________________
    --> #3: exp2 => @ #int#
    -->     Type: shift, Inst: shift
    -->     LA: #int#

** State #3: exp2 => @ #int#
    --> __________________
    --> #4: exp2 => #int# @
    -->     Type: move, Inst: pass
    -->     LA: #int#

** [FINAL] State #4: exp2 => #int# @
    --> __________________
    --> #5: exp1 => [ exp1 ( '*' | '/' ) ] exp2 @
    -->     Type: reduce, Inst: pass_reduce
    -->     Reduce: exp1 => [ @ exp1 ( '*' | '/' ) ] exp2
    --> __________________
    --> #5: exp1 => [ exp1 ( '*' | '/' ) ] exp2 @
    -->     Type: reduce, Inst: pass_reduce
    -->     Reduce: exp1 => [ exp1 ( '*' | '/' @ ) ] exp2
    --> __________________
    --> #5: exp1 => [ exp1 ( '*' | '/' ) ] exp2 @
    -->     Type: reduce, Inst: pass_reduce
    -->     Reduce: exp1 => [ exp1 ( '*' @ | '/' ) ] exp2

** [FINAL] State #5: exp1 => [ exp1 ( '*' | '/' ) ] exp2 @
    --> __________________
    --> #6: exp0 => [ exp0 ( '+' | '-' ) ] exp1 @
    -->     Type: reduce, Inst: pass_reduce
    -->     Reduce: exp0 => [ @ exp0 ( '+' | '-' ) ] exp1
    --> __________________
    --> #6: exp0 => [ exp0 ( '+' | '-' ) ] exp1 @
    -->     Type: reduce, Inst: pass_reduce
    -->     Reduce: exp0 => [ exp0 ( '+' | '-' @ ) ] exp1
    --> __________________
    --> #6: exp0 => [ exp0 ( '+' | '-' ) ] exp1 @
    -->     Type: reduce, Inst: pass_reduce
    -->     Reduce: exp0 => [ exp0 ( '+' @ | '-' ) ] exp1
    --> __________________
    --> #7: exp1 => [ exp1 @ ( '*' | '/' ) ] exp2
    -->     Type: left_recursion, Inst: pass_recursion
    -->     LA: '*' '/'

** [FINAL] State #6: exp0 => [ exp0 ( '+' | '-' ) ] exp1 @
    --> __________________
    --> #8: exp0 => [ exp0 @ ( '+' | '-' ) ] exp1
    -->     Type: left_recursion, Inst: pass_recursion
    -->     LA: '+' '-'
    --> __________________
    --> #9: root => exp0 @
    -->     Type: reduce, Inst: pass_reduce
    -->     Reduce: root => @ exp0

** State #7: exp1 => [ exp1 @ ( '*' | '/' ) ] exp2
    --> __________________
    --> #10: exp1 => [ exp1 ( '*' | '/' @ ) ] exp2
    -->     Type: move, Inst: pass
    -->     LA: '/'
    --> __________________
    --> #11: exp1 => [ exp1 ( '*' @ | '/' ) ] exp2
    -->     Type: move, Inst: pass
    -->     LA: '*'

** State #8: exp0 => [ exp0 @ ( '+' | '-' ) ] exp1
    --> __________________
    --> #12: exp0 => [ exp0 ( '+' | '-' @ ) ] exp1
    -->     Type: move, Inst: pass
    -->     LA: '-'
    --> __________________
    --> #13: exp0 => [ exp0 ( '+' @ | '-' ) ] exp1
    -->     Type: move, Inst: pass
    -->     LA: '+'

** [FINAL] State #9: root => exp0 @
    --> __________________
    --> #9: root => exp0 @
    -->     Type: finish, Inst: finish

** State #10: exp1 => [ exp1 ( '*' | '/' @ ) ] exp2
    --> __________________
    --> #3: exp2 => @ #int#
    -->     Type: shift, Inst: shift
    -->     LA: #int#

** State #11: exp1 => [ exp1 ( '*' @ | '/' ) ] exp2
    --> __________________
    --> #3: exp2 => @ #int#
    -->     Type: shift, Inst: shift
    -->     LA: #int#

** State #12: exp0 => [ exp0 ( '+' | '-' @ ) ] exp1
    --> __________________
    --> #2: exp1 => [ @ exp1 ( '*' | '/' ) ] exp2
    -->     Type: shift, Inst: shift
    -->     LA: #int#

** State #13: exp0 => [ exp0 ( '+' @ | '-' ) ] exp1
    --> __________________
    --> #2: exp1 => [ @ exp1 ( '*' | '/' ) ] exp2
    -->     Type: shift, Inst: shift
    -->     LA: #int#
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
    - [x] 求First集合，并输出
    - [x] 检查文法有效性（如不产生Epsilon）
    - [x] 检查纯左递归
    - [x] 生成PDA
    - [x] 打印PDA结构（独立于内存池）

1. 将文法树转换成PDA表（完成）

## 改进

- [ ] 生成LR项目集时将@符号提到集合的外面，减少状态
- [x] PDA表的生成时使用了内存池来保存结点，当生成PDA表后，内存池可以全部回收

## 参考

1. [CParser](https://github.com/bajdcc/CParser)
2. [CMiniLang](https://github.com/bajdcc/CMiniLang)
3. [vczh GLR Parser](https://github.com/vczh-libraries/Vlpp/tree/master/Source/Parsing)