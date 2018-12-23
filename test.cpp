#include <iostream>
#include "cexception.h"
#include "cparser.h"

int i = 0;

void test(const string_t &s) {
    using namespace clib;
    try {
        cparser p(s);
        auto root = p.parse();
        std::cout << "======== CODE ========" << std::endl;
        std::cout << s << std::endl;
        cast::print(root, 0, std::cout);
        std::cout << "PASSED " << ++i << std::endl;
    } catch (const cexception &e) {
        std::cout << "RUNTIME ERROR: " << e.msg << std::endl;
    }
}

int main() {
    // https://github.com/antlr/grammars-v4/blob/master/c/examples/FuncCallAsFuncArgument.c
    test(R"(
void aX(void);
int a1(int param1);
int a2(int param1, param2);
void a3();
void a3(void);

int f(int arg1, char arg2)
{
    a1(arg1);
    a2(arg1, arg2);
    a3();
    a1(a1());
    a1(a1(), a2(a1(), x1));
}
)");
    // https://github.com/antlr/grammars-v4/blob/master/c/examples/FuncCallwithVarArgs.c
    test(R"(
void aX(void);
int a1(int param1);
int a2(int param1, param2);
void a3();
void a3(void);
void a4(int, ...);
void a4(int param1, ...);
int f(int arg1, char arg2)
{
    a1(arg1);
    a2(arg1, arg2);
    a3();
    a1(a1());
    a1(a1(), a2(a1(), x1));
}
)");
    // https://github.com/antlr/grammars-v4/blob/master/c/examples/FuncForwardDeclaration.c
    test(R"(
void aX(void);
int a1(int param1);
int a2(int param1, param2);
void a3();
void a3(void);
int f(int arg1, char arg2)
{
    a1(arg1);
    a2(arg1, arg2);
    a3();
}
)");
    // https://github.com/antlr/grammars-v4/blob/master/c/examples/FunctionPointer.c
    test(R"(
typedef
void *
(*f1)(
    const MyType    *param1,
    long             param2,
    void            *param3,
    short            param4
  );
typedef
int
(*f2)(
    const MyType    *param1,
    long             param2,
    char            *param3,
    int              param4
  );
typedef
MyStruct
( *f3 ) (
    const MyType    *param1,
    double           param2,
    float           *param3,
    long             param4
  );
)");
    // https://github.com/antlr/grammars-v4/blob/master/c/examples/FunctionCall.c
    test(R"(
int f(int arg1, char arg2)
{
    a1(arg1);
    a2(arg1, arg2);
    a3();
}
)");
    // https://github.com/antlr/grammars-v4/blob/master/c/examples/TypeCast.c
    test(R"(
f()
{
    a1 = (int)(b1);
    a2 = (CustomType)(b2);
    a3 = (CustomType *)(b3);
    a4 = (CustomType **)(b4);
    a5 = b5();
}
)");
    // https://github.com/antlr/grammars-v4/blob/master/c/examples/add.c
    test(R"(
int main()
{
    int i, sum = 0;
    for ( i = 1; i <= LAST; i++ ) {
      sum += i;
    } /*-for-*/
    printf("sum = %d\n", sum);
    return 0;
}
)");
    // https://github.com/antlr/grammars-v4/blob/master/c/examples/bt.c
    test(R"(
struct tree_el {
   int val;
   struct tree_el * right, * left;
};

typedef struct tree_el node;

void insert(node ** tree, node * item) {
   if(!(*tree)) {
      *tree = item;
      return;
   }
   if(item->val<(*tree)->val)
      insert(&(*tree)->left, item);
   else if(item->val>(*tree)->val)
      insert(&(*tree)->right, item);
}

void printout(node * tree) {
   if(tree->left) printout(tree->left);
   printf("%d\n",tree->val);
   if(tree->right) printout(tree->right);
}

void main() {
   node * curr, * root;
   int i;

   root = NULL;

   for(i=1;i<=10;i++) {
      curr = (node *)malloc(sizeof(node));
      curr->left = curr->right = NULL;
      curr->val = rand();
      insert(&root, curr);
   }

   printout(root);
}
)");
    // https://github.com/antlr/grammars-v4/blob/master/c/examples/integrate.c
    test(R"(
float f(float x)
{
    return(1/(1+pow(x,2)));
}
void main()
{
    int i,n;
    float x0,xn,h,y[20],so,se,ans,x[20];
    printf("\n Enter values of x0,xn,h:\n");
    scanf("%f%f%f",&x0,&xn,&h);
    n=(xn-x0)/h;
    if(n%2==1)
    {
        n=n+1;
    }
    h=(xn-x0)/n;
    printf("\nrefined value of n and h are:%d  %f\n",n,h);
    printf("\n Y values \n");
    for(i=0; i<=n; i++)
    {
        x[i]=x0+i*h;
        y[i]=f(x[i]);
        printf("\n%f\n",y[i]);
    }
    so=0;
    se=0;
    for(i=1; i<n; i++)
    {
        if(i%2==1)
        {
            so=so+y[i];
        }
        else
        {
            se=se+y[i];
        }
    }
    ans=h/3*(y[0]+y[n]+4*so+2*se);
    printf("\nfinal integration is %f",ans);
    getch();
}
)");
}