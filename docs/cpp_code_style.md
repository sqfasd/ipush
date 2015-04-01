# 简明c++编码规范

## 命名约定

1. 不要过度缩写，除非是一个约定俗成、广泛使用的术语时，才使用缩写
2. 普通变量名和结构体变量名全部使用小写，单词间用下划线连接，例如:my_variable
3. 类的成员变量全部使用小写，单词间用下划线连接，并且以下划线结尾，例如:my_member_variable_
4. 文件名全部使用小写，单词间用下滑线连接，头文件以.h结尾，实现文件以.cc结尾，例如:my_file.h，my_file.cc
5. 类型名中每个单词的首字母大写，中间不加下划线，例如:MyClass
6. 宏、const常量、枚举常量命名，全大写，单词间加下划线，例如:MY_CONST_VAIRABLE
7. 函数名中每个单词的首字母大写，例如:DoSomething()
8. 名字空间使用小写，单词间加下划线，例如:my_namespace

## 头文件保护

头文件使用#define防止多重包含， define后的命名为该头文件所在位置的全路径，
例如```src/bar/baz.h```

```
#ifndef SRC_BAR_BAZ_H_
#define SRC_BAR_BAZ_H_
#endif
```

## include语句

include路径要写全路径，include头文件的顺序依次为：（以dir/foo.cc举例）
尽量把同一个库的相关头文件放在一块

1. src/dir/foo.h(该cc文件对应的头文件)
2. c系统文件
3. c++系统文件
4. 第三方库的.h文件
5. 本项目其他模块的.h文件

## 类声明顺序

** 函数前，数据后。 public前，private后 **

### 访问权限区段顺序:

1. public
2. protected
3. private

### 区段内:

1. typedef
2. 枚举
3. 常量(static const成员)
4. 构造函数
5. 析构函数
6. 成员函数，包括静态成员函数
7. 数据成员，除static const外

## 注释：

1. 只使用//单行注释
2. 注释写在被注释的代码上面，不要写在被注释代码后面

## 条件/循环语句

1. if/for/while之后，左花括号之前留有空格
2. 每个if和else语句块必须加花括号，即使语句块只有一行代码

```
if (x > b) {
  // do some thing
} else {
  // do other thing
}
for (int i = 0; i < n; ++i) {
  // do something
}
while (true) {
  // do something
}
```

## 指针、引用变量声明

1. &和*靠近类型
2. 每行只能声明一个变量

```
char* c;
const string& str;
```

## 缩进

1. 只使用空格，不使用制表符
2. 每次缩进两个空格
3. 名字空间内不增加锁紧层次

## 布尔表达式断行

如果超过标准行宽，以布尔操作符结尾，进行断行

```
if (this_one_thing > this_other_thing &&
    a_third_thing == a_fourth_thing &&
    yet_another && last_one) {
  // do some thing
}
```
## 其他

1. 尽一切可能的使用const
2. 当一个类型内只有数据没有函数时，使用struct， 其他一律使用class
3. 函数参数顺序,输入参数在前，输出参数在后
4. 不要在头文件中使用using namespace， 在cc文件中或头文件的函数或类中可以使用
5. 不要提前声明局部变量， 只在马上要用到时才声明

## 尽量参考google c++ style
