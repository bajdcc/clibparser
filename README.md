# clibparser（GLR Parser）

C++实现的LR Parser Generator。

- 文法书写方式：以C++重载为基础的Parser Generator。
- 识别方式：**以下推自动机为基础，向看查看一个字符、带回溯的LR分析**。
- 内存管理：自制内存池。

## 文章

- [【Parser系列】实现LR分析——开篇](https://zhuanlan.zhihu.com/p/52478414)
- [【Parser系列】实现LR分析——生成AST](https://zhuanlan.zhihu.com/p/52528516)

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
int main() {
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

结果：`((((((int))) ((main ( ))) ({ ((((((int))) ((((a))) , (((b))) , (((c)))) ;)) (((((float))) ((((d))) , (((e))) , (((f)))) ;))) }))))`
结果未去括号。

## 调试信息

实现C语言的解析。

生成NGA图，去EPSILON化，生成PDA表，生成AST。

以下为下推自动机：

**由于太长，文法定义和下推自动机在根目录下的grammar.txt文件中！**

以下为下推自动机的识别过程：

```cpp
State:   0 => To:   1   -- Action: shift      -- Rule: compilationUnit => @ translationUnit [ compilationUnit ]
State:   1 => To:   2   -- Action: shift      -- Rule: translationUnit => [ @ translationUnit ] externalDeclaration
State:   2 => To:   4   -- Action: shift      -- Rule: externalDeclaration => @ ( functionDefinition | declaration | ';' )
State:   4 => To:   7   -- Action: shift      -- Rule: functionDefinition => [ @ declarationSpecifiers ] declarator [ declarationList ] compoundStatement
State:   7 => To:  11   -- Action: shift      -- Rule: declarationSpecifiers => @ declarationSpecifier [ declarationSpecifiers ]
State:  11 => To:  18   -- Action: shift      -- Rule: declarationSpecifier => @ ( storageClassSpecifier | typeSpecifier | typeQualifier )
State:  18 => To:  34   -- Action: move       -- Rule: typeSpecifier => @ ( void | char | short | int | long | float | double | signed | unsigned | bool | structOrUnionSpecifier | enumSpecifier | typedefName | typeSpecifier pointer )
State:  34 => To:  55   -- Action: reduce     -- Rule: typeSpecifier => ( void | char | short | int @ | long | float | double | signed | unsigned | bool | structOrUnionSpecifier | enumSpecifier | typedefName | typeSpecifier pointer )
State:  55 => To:  73   -- Action: reduce     -- Rule: declarationSpecifier => ( storageClassSpecifier | typeSpecifier @ | typeQualifier )
State:  73 => To:   7   -- Action: shift      -- Rule: declarationSpecifiers => declarationSpecifier @ [ declarationSpecifiers ]
State:   7 => To:  11   -- Action: shift      -- Rule: declarationSpecifiers => @ declarationSpecifier [ declarationSpecifiers ]
State:  11 => To:  18   -- Action: shift      -- Rule: declarationSpecifier => @ ( storageClassSpecifier | typeSpecifier | typeQualifier )
State:  18 => To:  43   -- Action: shift      -- Rule: typeSpecifier => @ ( void | char | short | int | long | float | double | signed | unsigned | bool | structOrUnionSpecifier | enumSpecifier | typedefName | typeSpecifier pointer )
State:  43 => To:  60   -- Action: move       -- Rule: typedefName => @ #identifier#
State:  60 => To:  88   -- Action: reduce     -- Rule: typedefName => #identifier# @
State:  88 => To:  55   -- Action: reduce     -- Rule: typeSpecifier => ( void | char | short | int | long | float | double | signed | unsigned | bool | structOrUnionSpecifier | enumSpecifier | typedefName @ | typeSpecifier pointer )
State:  55 => To:  73   -- Action: reduce     -- Rule: declarationSpecifier => ( storageClassSpecifier | typeSpecifier @ | typeQualifier )
State:  73 => To: 110   -- Action: reduce     -- Rule: declarationSpecifiers => declarationSpecifier @ [ declarationSpecifiers ]
State: 110 => To: 113   -- Action: reduce     -- Rule: declarationSpecifiers => declarationSpecifier [ declarationSpecifiers @ ]
State: 113 => To:   8   -- Action: shift      -- Rule: functionDefinition => [ declarationSpecifiers @ ] declarator [ declarationList ] compoundStatement
State:   8 => To:  13   -- Action: shift      -- Rule: declarator => [ @ pointer ] directDeclarator
State:  13 => To:  23   -- Action: move       -- Rule: directDeclarator => @ ( #identifier# | '(' declarator ')' | directDeclarator ( '[' [ typeQualifierList ] ( assignmentExpression | '*' ) ']' | '(' ( parameterTypeList | [ identifierList ] ) ')' ) )
parsing error: directDeclarator => ( #identifier# | '(' @ declarator ')' | directDeclarator ( '[' [ typeQualifierList ] ( assignmentExpression | '*' ) ']' | '(' ( parameterTypeList | [ identifierList ] ) ')' ) )
State:  73 => To: 113   -- Action: reduce     -- Rule: declarationSpecifiers => declarationSpecifier @ [ declarationSpecifiers ]
State: 113 => To:   8   -- Action: shift      -- Rule: functionDefinition => [ declarationSpecifiers @ ] declarator [ declarationList ] compoundStatement
State:   8 => To:  13   -- Action: shift      -- Rule: declarator => [ @ pointer ] directDeclarator
State:  13 => To:  22   -- Action: move       -- Rule: directDeclarator => @ ( #identifier# | '(' declarator ')' | directDeclarator ( '[' [ typeQualifierList ] ( assignmentExpression | '*' ) ']' | '(' ( parameterTypeList | [ identifierList ] ) ')' ) )
State:  22 => To:  50   -- Action: recursion  -- Rule: directDeclarator => ( #identifier# @ | '(' declarator ')' | directDeclarator ( '[' [ typeQualifierList ] ( assignmentExpression | '*' ) ']' | '(' ( parameterTypeList | [ identifierList ] ) ')' ) )
State:  50 => To:  71   -- Action: move       -- Rule: directDeclarator => ( #identifier# | '(' declarator ')' | directDeclarator @ ( '[' [ typeQualifierList ] ( assignmentExpression | '*' ) ']' | '(' ( parameterTypeList | [ identifierList ] ) ')' ) )
State:  71 => To: 104   -- Action: move       -- Rule: directDeclarator => ( #identifier# | '(' declarator ')' | directDeclarator ( '[' [ typeQualifierList ] ( assignmentExpression | '*' ) ']' | '(' @ ( parameterTypeList | [ identifierList ] ) ')' ) )
State: 104 => To:  49   -- Action: reduce     -- Rule: directDeclarator => ( #identifier# | '(' declarator ')' | directDeclarator ( '[' [ typeQualifierList ] ( assignmentExpression | '*' ) ']' | '(' ( parameterTypeList | [ identifierList ] ) ')' @ ) )
State:  49 => To:  66   -- Action: reduce     -- Rule: declarator => [ pointer ] directDeclarator @
State:  66 => To:  98   -- Action: shift      -- Rule: functionDefinition => [ declarationSpecifiers ] declarator @ [ declarationList ] compoundStatement
State:  98 => To: 132   -- Action: move       -- Rule: compoundStatement => @ '{' [ blockItemList ] '}'
State: 132 => To: 163   -- Action: shift      -- Rule: compoundStatement => '{' @ [ blockItemList ] '}'
State: 163 => To: 198   -- Action: shift      -- Rule: blockItemList => [ @ blockItemList ] blockItem
State: 198 => To:   5   -- Action: shift      -- Rule: blockItem => @ ( statement | declaration )
State:   5 => To:   7   -- Action: shift      -- Rule: declaration => @ declarationSpecifiers [ initDeclaratorList ] ';'
State:   7 => To:  11   -- Action: shift      -- Rule: declarationSpecifiers => @ declarationSpecifier [ declarationSpecifiers ]
State:  11 => To:  18   -- Action: shift      -- Rule: declarationSpecifier => @ ( storageClassSpecifier | typeSpecifier | typeQualifier )
State:  18 => To:  34   -- Action: move       -- Rule: typeSpecifier => @ ( void | char | short | int | long | float | double | signed | unsigned | bool | structOrUnionSpecifier | enumSpecifier | typedefName | typeSpecifier pointer )
State:  34 => To:  55   -- Action: reduce     -- Rule: typeSpecifier => ( void | char | short | int @ | long | float | double | signed | unsigned | bool | structOrUnionSpecifier | enumSpecifier | typedefName | typeSpecifier pointer )
State:  55 => To:  73   -- Action: reduce     -- Rule: declarationSpecifier => ( storageClassSpecifier | typeSpecifier @ | typeQualifier )
State:  73 => To:   7   -- Action: shift      -- Rule: declarationSpecifiers => declarationSpecifier @ [ declarationSpecifiers ]
State:   7 => To:  11   -- Action: shift      -- Rule: declarationSpecifiers => @ declarationSpecifier [ declarationSpecifiers ]
State:  11 => To:  18   -- Action: shift      -- Rule: declarationSpecifier => @ ( storageClassSpecifier | typeSpecifier | typeQualifier )
State:  18 => To:  43   -- Action: shift      -- Rule: typeSpecifier => @ ( void | char | short | int | long | float | double | signed | unsigned | bool | structOrUnionSpecifier | enumSpecifier | typedefName | typeSpecifier pointer )
State:  43 => To:  60   -- Action: move       -- Rule: typedefName => @ #identifier#
State:  60 => To:  88   -- Action: reduce     -- Rule: typedefName => #identifier# @
State:  88 => To:  55   -- Action: reduce     -- Rule: typeSpecifier => ( void | char | short | int | long | float | double | signed | unsigned | bool | structOrUnionSpecifier | enumSpecifier | typedefName @ | typeSpecifier pointer )
State:  55 => To:  73   -- Action: reduce     -- Rule: declarationSpecifier => ( storageClassSpecifier | typeSpecifier @ | typeQualifier )
State:  73 => To: 110   -- Action: reduce     -- Rule: declarationSpecifiers => declarationSpecifier @ [ declarationSpecifiers ]
State: 110 => To: 109   -- Action: reduce     -- Rule: declarationSpecifiers => declarationSpecifier [ declarationSpecifiers @ ]
parsing error: declaration => declarationSpecifiers @ [ initDeclaratorList ] ';'
State:  73 => To: 109   -- Action: reduce     -- Rule: declarationSpecifiers => declarationSpecifier @ [ declarationSpecifiers ]
State: 109 => To: 146   -- Action: shift      -- Rule: declaration => declarationSpecifiers @ [ initDeclaratorList ] ';'
State: 146 => To: 179   -- Action: shift      -- Rule: initDeclaratorList => [ @ initDeclaratorList ',' ] initDeclarator
State: 179 => To:   8   -- Action: shift      -- Rule: initDeclarator => @ declarator [ '=' initializer ]
State:   8 => To:  13   -- Action: shift      -- Rule: declarator => [ @ pointer ] directDeclarator
State:  13 => To:  22   -- Action: move       -- Rule: directDeclarator => @ ( #identifier# | '(' declarator ')' | directDeclarator ( '[' [ typeQualifierList ] ( assignmentExpression | '*' ) ']' | '(' ( parameterTypeList | [ identifierList ] ) ')' ) )
State:  22 => To:  49   -- Action: reduce     -- Rule: directDeclarator => ( #identifier# @ | '(' declarator ')' | directDeclarator ( '[' [ typeQualifierList ] ( assignmentExpression | '*' ) ']' | '(' ( parameterTypeList | [ identifierList ] ) ')' ) )
State:  49 => To:  67   -- Action: reduce     -- Rule: declarator => [ pointer ] directDeclarator @
State:  67 => To: 101   -- Action: reduce     -- Rule: initDeclarator => declarator @ [ '=' initializer ]
State: 101 => To: 136   -- Action: recursion  -- Rule: initDeclaratorList => [ initDeclaratorList ',' ] initDeclarator @
State: 136 => To: 165   -- Action: move       -- Rule: initDeclaratorList => [ initDeclaratorList @ ',' ] initDeclarator
State: 165 => To: 179   -- Action: shift      -- Rule: initDeclaratorList => [ initDeclaratorList ',' @ ] initDeclarator
State: 179 => To:   8   -- Action: shift      -- Rule: initDeclarator => @ declarator [ '=' initializer ]
State:   8 => To:  13   -- Action: shift      -- Rule: declarator => [ @ pointer ] directDeclarator
State:  13 => To:  22   -- Action: move       -- Rule: directDeclarator => @ ( #identifier# | '(' declarator ')' | directDeclarator ( '[' [ typeQualifierList ] ( assignmentExpression | '*' ) ']' | '(' ( parameterTypeList | [ identifierList ] ) ')' ) )
State:  22 => To:  49   -- Action: reduce     -- Rule: directDeclarator => ( #identifier# @ | '(' declarator ')' | directDeclarator ( '[' [ typeQualifierList ] ( assignmentExpression | '*' ) ']' | '(' ( parameterTypeList | [ identifierList ] ) ')' ) )
State:  49 => To:  67   -- Action: reduce     -- Rule: declarator => [ pointer ] directDeclarator @
State:  67 => To: 101   -- Action: reduce     -- Rule: initDeclarator => declarator @ [ '=' initializer ]
State: 101 => To: 136   -- Action: recursion  -- Rule: initDeclaratorList => [ initDeclaratorList ',' ] initDeclarator @
State: 136 => To: 165   -- Action: move       -- Rule: initDeclaratorList => [ initDeclaratorList @ ',' ] initDeclarator
State: 165 => To: 179   -- Action: shift      -- Rule: initDeclaratorList => [ initDeclaratorList ',' @ ] initDeclarator
State: 179 => To:   8   -- Action: shift      -- Rule: initDeclarator => @ declarator [ '=' initializer ]
State:   8 => To:  13   -- Action: shift      -- Rule: declarator => [ @ pointer ] directDeclarator
State:  13 => To:  22   -- Action: move       -- Rule: directDeclarator => @ ( #identifier# | '(' declarator ')' | directDeclarator ( '[' [ typeQualifierList ] ( assignmentExpression | '*' ) ']' | '(' ( parameterTypeList | [ identifierList ] ) ')' ) )
State:  22 => To:  49   -- Action: reduce     -- Rule: directDeclarator => ( #identifier# @ | '(' declarator ')' | directDeclarator ( '[' [ typeQualifierList ] ( assignmentExpression | '*' ) ']' | '(' ( parameterTypeList | [ identifierList ] ) ')' ) )
State:  49 => To:  67   -- Action: reduce     -- Rule: declarator => [ pointer ] directDeclarator @
State:  67 => To: 101   -- Action: reduce     -- Rule: initDeclarator => declarator @ [ '=' initializer ]
State: 101 => To: 134   -- Action: reduce     -- Rule: initDeclaratorList => [ initDeclaratorList ',' ] initDeclarator @
State: 134 => To: 145   -- Action: move       -- Rule: declaration => declarationSpecifiers [ initDeclaratorList @ ] ';'
State: 145 => To: 176   -- Action: reduce     -- Rule: declaration => declarationSpecifiers [ initDeclaratorList ] ';' @
State: 176 => To: 211   -- Action: reduce     -- Rule: blockItem => ( statement | declaration @ )
State: 211 => To: 249   -- Action: recursion  -- Rule: blockItemList => [ blockItemList ] blockItem @
State: 249 => To: 198   -- Action: shift      -- Rule: blockItemList => [ blockItemList @ ] blockItem
State: 198 => To:   5   -- Action: shift      -- Rule: blockItem => @ ( statement | declaration )
State:   5 => To:   7   -- Action: shift      -- Rule: declaration => @ declarationSpecifiers [ initDeclaratorList ] ';'
State:   7 => To:  11   -- Action: shift      -- Rule: declarationSpecifiers => @ declarationSpecifier [ declarationSpecifiers ]
State:  11 => To:  18   -- Action: shift      -- Rule: declarationSpecifier => @ ( storageClassSpecifier | typeSpecifier | typeQualifier )
State:  18 => To:  36   -- Action: move       -- Rule: typeSpecifier => @ ( void | char | short | int | long | float | double | signed | unsigned | bool | structOrUnionSpecifier | enumSpecifier | typedefName | typeSpecifier pointer )
State:  36 => To:  55   -- Action: reduce     -- Rule: typeSpecifier => ( void | char | short | int | long | float @ | double | signed | unsigned | bool | structOrUnionSpecifier | enumSpecifier | typedefName | typeSpecifier pointer )
State:  55 => To:  73   -- Action: reduce     -- Rule: declarationSpecifier => ( storageClassSpecifier | typeSpecifier @ | typeQualifier )
State:  73 => To:   7   -- Action: shift      -- Rule: declarationSpecifiers => declarationSpecifier @ [ declarationSpecifiers ]
State:   7 => To:  11   -- Action: shift      -- Rule: declarationSpecifiers => @ declarationSpecifier [ declarationSpecifiers ]
State:  11 => To:  18   -- Action: shift      -- Rule: declarationSpecifier => @ ( storageClassSpecifier | typeSpecifier | typeQualifier )
State:  18 => To:  43   -- Action: shift      -- Rule: typeSpecifier => @ ( void | char | short | int | long | float | double | signed | unsigned | bool | structOrUnionSpecifier | enumSpecifier | typedefName | typeSpecifier pointer )
State:  43 => To:  60   -- Action: move       -- Rule: typedefName => @ #identifier#
State:  60 => To:  88   -- Action: reduce     -- Rule: typedefName => #identifier# @
State:  88 => To:  55   -- Action: reduce     -- Rule: typeSpecifier => ( void | char | short | int | long | float | double | signed | unsigned | bool | structOrUnionSpecifier | enumSpecifier | typedefName @ | typeSpecifier pointer )
State:  55 => To:  73   -- Action: reduce     -- Rule: declarationSpecifier => ( storageClassSpecifier | typeSpecifier @ | typeQualifier )
State:  73 => To: 110   -- Action: reduce     -- Rule: declarationSpecifiers => declarationSpecifier @ [ declarationSpecifiers ]
State: 110 => To: 109   -- Action: reduce     -- Rule: declarationSpecifiers => declarationSpecifier [ declarationSpecifiers @ ]
parsing error: declaration => declarationSpecifiers @ [ initDeclaratorList ] ';'
State:  73 => To: 109   -- Action: reduce     -- Rule: declarationSpecifiers => declarationSpecifier @ [ declarationSpecifiers ]
State: 109 => To: 146   -- Action: shift      -- Rule: declaration => declarationSpecifiers @ [ initDeclaratorList ] ';'
State: 146 => To: 179   -- Action: shift      -- Rule: initDeclaratorList => [ @ initDeclaratorList ',' ] initDeclarator
State: 179 => To:   8   -- Action: shift      -- Rule: initDeclarator => @ declarator [ '=' initializer ]
State:   8 => To:  13   -- Action: shift      -- Rule: declarator => [ @ pointer ] directDeclarator
State:  13 => To:  22   -- Action: move       -- Rule: directDeclarator => @ ( #identifier# | '(' declarator ')' | directDeclarator ( '[' [ typeQualifierList ] ( assignmentExpression | '*' ) ']' | '(' ( parameterTypeList | [ identifierList ] ) ')' ) )
State:  22 => To:  49   -- Action: reduce     -- Rule: directDeclarator => ( #identifier# @ | '(' declarator ')' | directDeclarator ( '[' [ typeQualifierList ] ( assignmentExpression | '*' ) ']' | '(' ( parameterTypeList | [ identifierList ] ) ')' ) )
State:  49 => To:  67   -- Action: reduce     -- Rule: declarator => [ pointer ] directDeclarator @
State:  67 => To: 101   -- Action: reduce     -- Rule: initDeclarator => declarator @ [ '=' initializer ]
State: 101 => To: 136   -- Action: recursion  -- Rule: initDeclaratorList => [ initDeclaratorList ',' ] initDeclarator @
State: 136 => To: 165   -- Action: move       -- Rule: initDeclaratorList => [ initDeclaratorList @ ',' ] initDeclarator
State: 165 => To: 179   -- Action: shift      -- Rule: initDeclaratorList => [ initDeclaratorList ',' @ ] initDeclarator
State: 179 => To:   8   -- Action: shift      -- Rule: initDeclarator => @ declarator [ '=' initializer ]
State:   8 => To:  13   -- Action: shift      -- Rule: declarator => [ @ pointer ] directDeclarator
State:  13 => To:  22   -- Action: move       -- Rule: directDeclarator => @ ( #identifier# | '(' declarator ')' | directDeclarator ( '[' [ typeQualifierList ] ( assignmentExpression | '*' ) ']' | '(' ( parameterTypeList | [ identifierList ] ) ')' ) )
State:  22 => To:  49   -- Action: reduce     -- Rule: directDeclarator => ( #identifier# @ | '(' declarator ')' | directDeclarator ( '[' [ typeQualifierList ] ( assignmentExpression | '*' ) ']' | '(' ( parameterTypeList | [ identifierList ] ) ')' ) )
State:  49 => To:  67   -- Action: reduce     -- Rule: declarator => [ pointer ] directDeclarator @
State:  67 => To: 101   -- Action: reduce     -- Rule: initDeclarator => declarator @ [ '=' initializer ]
State: 101 => To: 136   -- Action: recursion  -- Rule: initDeclaratorList => [ initDeclaratorList ',' ] initDeclarator @
State: 136 => To: 165   -- Action: move       -- Rule: initDeclaratorList => [ initDeclaratorList @ ',' ] initDeclarator
State: 165 => To: 179   -- Action: shift      -- Rule: initDeclaratorList => [ initDeclaratorList ',' @ ] initDeclarator
State: 179 => To:   8   -- Action: shift      -- Rule: initDeclarator => @ declarator [ '=' initializer ]
State:   8 => To:  13   -- Action: shift      -- Rule: declarator => [ @ pointer ] directDeclarator
State:  13 => To:  22   -- Action: move       -- Rule: directDeclarator => @ ( #identifier# | '(' declarator ')' | directDeclarator ( '[' [ typeQualifierList ] ( assignmentExpression | '*' ) ']' | '(' ( parameterTypeList | [ identifierList ] ) ')' ) )
State:  22 => To:  49   -- Action: reduce     -- Rule: directDeclarator => ( #identifier# @ | '(' declarator ')' | directDeclarator ( '[' [ typeQualifierList ] ( assignmentExpression | '*' ) ']' | '(' ( parameterTypeList | [ identifierList ] ) ')' ) )
State:  49 => To:  67   -- Action: reduce     -- Rule: declarator => [ pointer ] directDeclarator @
State:  67 => To: 101   -- Action: reduce     -- Rule: initDeclarator => declarator @ [ '=' initializer ]
State: 101 => To: 134   -- Action: reduce     -- Rule: initDeclaratorList => [ initDeclaratorList ',' ] initDeclarator @
State: 134 => To: 145   -- Action: move       -- Rule: declaration => declarationSpecifiers [ initDeclaratorList @ ] ';'
State: 145 => To: 176   -- Action: reduce     -- Rule: declaration => declarationSpecifiers [ initDeclaratorList ] ';' @
State: 176 => To: 211   -- Action: reduce     -- Rule: blockItem => ( statement | declaration @ )
State: 211 => To: 250   -- Action: reduce     -- Rule: blockItemList => [ blockItemList ] blockItem @
State: 250 => To: 162   -- Action: move       -- Rule: compoundStatement => '{' [ blockItemList @ ] '}'
State: 162 => To: 196   -- Action: reduce     -- Rule: compoundStatement => '{' [ blockItemList ] '}' @
State: 196 => To: 233   -- Action: reduce     -- Rule: functionDefinition => [ declarationSpecifiers ] declarator [ declarationList ] compoundStatement @
State: 233 => To:   6   -- Action: reduce     -- Rule: externalDeclaration => ( functionDefinition @ | declaration | ';' )
State:   6 => To:   9   -- Action: reduce     -- Rule: translationUnit => [ translationUnit ] externalDeclaration @
State:   9 => To:   9   -- Action: finish     -- Rule: compilationUnit => translationUnit @ [ compilationUnit ]
((((((int))) ((main ( ))) ({ ((((((int))) ((((a))) , (((b))) , (((c)))) ;)) (((((float))) ((((d))) , (((e))) , (((f)))) ;))) }))))
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
- [x] 生成下推自动机
    - [x] 求First集合，并输出
    - [x] 检查文法有效性（如不产生Epsilon）
    - [x] 检查纯左递归
    - [x] 生成PDA
    - [x] 打印PDA结构（独立于内存池）
- [x] 生成抽象语法树
    - [x] 自动生成AST结构
    - [ ] 语义动作
- [x] 设计语言
    - [x] 使用[C语言文法](https://github.com/antlr/grammars-v4/blob/master/c/C.g4)
    - [x] 实现回溯，解决移进/归约冲突问题

1. 将文法树转换表（完成）
2. 根据PDA表生成AST（完成）

## 改进

- [ ] 生成LR项目集时将@符号提到集合的外面，减少状态
- [x] PDA表的生成时使用了内存池来保存结点，当生成PDA表后，内存池可以全部回收
- [x] 生成AST时减少嵌套结点
- [ ] 优化回溯时产生的数据结构，减少拷贝

## 参考

1. [CParser](https://github.com/bajdcc/CParser)
2. [CMiniLang](https://github.com/bajdcc/CMiniLang)
3. [vczh GLR Parser](https://github.com/vczh-libraries/Vlpp/tree/master/Source/Parsing)
4. [antlr/grammars-v4 C.g4](https://github.com/antlr/grammars-v4/blob/master/c/C.g4)