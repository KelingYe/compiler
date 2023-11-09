#include "TypeCheck.h"

// maps to store the type information. Feel free to design new data structures if you need.
typeMap g_token2Type;         // global token ids to type
typeMap funcparam_token2Type; // func params token ids to type
typeMap g_fnDec2Type;         // 存储声明过的函数及其返回值
typeMap g_fnDef2Type;         // 存储已定义过的函数及其返回值
typeMap curFuncParams;

paramMemberMap func2Param; // funtion name以及对应的函数列表
paramMemberMap struct2Members;  //struct name 以及对应的成员参数

// cur_scope,global is 1, entering scope ++, leaving scope --
int cur_scope = 1;

void addCurScope()
{
    cur_scope++;
    // printf("Entered a new code block %d\n", cur_scope);
}

// 清除所有curScope的varDecl
void minusCurScope()
{
    vector<string> allDeleteNames;
    // kill all the variables
    for (auto i : g_token2Type)
    {
        // 也就是在存变量表时，需要存该变量的作用域
        if (i.second->cur_scope == cur_scope)
        {
            // 不能在循环中改变结构体的值
            allDeleteNames.push_back(i.first);
        }
    }
    for (auto i : allDeleteNames)
        g_token2Type.erase(i);

    cur_scope--;
}

// private util functions. You can use these functions to help you debug.
void error_print(std::ostream *out, A_pos p, string info)
{
    *out << "Typecheck error in line " << p->line << ", col " << p->col << ": " << info << std::endl;
    exit(0);
}

void print_token_map(typeMap *map)
{
    for (auto it = map->begin(); it != map->end(); it++)
    {
        std::cout << it->first << " : ";
        switch (it->second->type)
        {
        case A_dataType::A_nativeTypeKind:
            switch (it->second->u.nativeType)
            {
            case A_nativeType::A_intTypeKind:
                std::cout << "int";
                break;
            default:
                break;
            }
            break;
        case A_dataType::A_structTypeKind:
            std::cout << *(it->second->u.structType);
            break;
        default:
            break;
        }
        std::cout << std::endl;
    }
}

// public functions
// This is the entrace of this file.
void check_Prog(std::ostream *out, aA_program p)
{
    // p is the root of AST tree.
    for (auto ele : p->programElements)
    {
        /*
            Write your code here.

            Hint:
            1. Design the order of checking the program elements to meet the requirements that funtion declaration and global variable declaration can be used anywhere in the program.

            2. Many types of statements indeed collapse to some same units, so a good abstract design will help you reduce the amount of your code.
        */
        switch (ele->kind)
        {
        case A_programVarDeclStmtKind:
        {
            check_VarDecl(out, ele->u.varDeclStmt);
            // check_VarDeclStmt(out, ele->u.varDeclStmt);
        }
        break;
        case A_programStructDefKind:
        {
            check_StructDef(out, ele->u.structDef);
        }
        break;
        case A_programFnDeclStmtKind:
        {
            check_FnDeclStmt(out, ele->u.fnDeclStmt);
        }
        break;
        case A_programFnDefKind:
        {

            // check_FnDef(out, ele->u.fnDef);
        }
        break;
        }
    }

    for (auto ele : p->programElements)
    {
    }

    for (auto ele : p->programElements)
    {
    }

    (*out) << "Typecheck passed!" << std::endl;
    return;
}

// 根据type 获得名字
//  without array
string getATypeName(aA_type type)
{
    if (!type)
        return;
    switch (type->type)
    {
    case (A_dataType::A_nativeTypeKind):
    {
        return "int";
    }
    case (A_dataType::A_structTypeKind):
    {
        return *(type->u.structType);
    }
    }
    return "nulltype";
}

// 根据type判断该type是否有效
void check_type_valid(std::ostream *out, aA_type varType)
{
    if (!varType)
        return;
    string name = getATypeName(varType);
    if (name != "int" && struct2Members.find(name) == struct2Members.end()) // end如何求解？
        error_print(out, varType->pos, "Undeclared variable type!");
}

// 检查是否重复声明（可以var，struct一起检查，并且不需要判断type，因为它们的名字是唯一的）//后续可以再加上fn
void check_multiDeclaration(std::ostream *out, string name, A_pos pos)
{

    if (g_token2Type.find(name) != g_token2Type.end())
    {
        error_print(out, pos, "This name " + name + " has been declared in variable map!");
    }
    // struct
    if (struct2Members.find(name) != struct2Members.end())
    {
        error_print(out, pos, "This name " + name + " has been declared in struct map!");
    }

    // func
    if (g_fnDec2Type.find(name) != g_fnDec2Type.end())
    {
        error_print(out, pos, "This name " + name + " has been declared in func name map!");
    }

    // func 的参数
    if (curFuncParams.find(name) != curFuncParams.end())
    {
        error_print(out, pos, "This name " + name + " has been declared in func variable list!");
    }
}

void check_VarDeclScalar(std::ostream *out, aA_varDeclScalar vd)
{
    // 检查类型
    aA_type type = vd->type;
    check_type_valid(out, type);
    // 检查是否重复声明
    string name = *(vd->id);
    A_pos pos = vd->pos;
    check_multiDeclaration(out, name, pos);
    // 保存信息至变量表
    type->cur_scope = cur_scope;
    g_token2Type[name] = type;
}

void check_VarDeclArray(std::ostream *out, aA_varDeclArray vd)
{
    // 检查类型
    aA_type type = vd->type;
    check_type_valid(out, type);
    // 检查是否重复声明
    string name = *(vd->id);
    A_pos pos = vd->pos;
    int len = vd->len;
    check_multiDeclaration(out, name, pos);
    // 保存信息至变量表  len没有用上，但是后续可能要用这个检查  ？？？？
    type->cur_scope = cur_scope;
    g_token2Type[name] = type;
}

void check_VarDec(std::ostream *out, aA_varDecl vd)
{
    if (!vd)
        return;

    if (vd->kind == A_varDeclStmtType::A_varDeclKind)
    {
        check_VarDeclScalar(out, vd->u.declScalar);
    }
    else if (vd->kind == A_varDeclStmtType::A_varDefKind)
    {
        check_VarDeclArray(out, vd->u.declArray);
    }
    return;
}

void check_VarDefScalar(std::ostream *out, aA_varDefScalar vd)
{
    if (!vd)
        return;
    // 检查类型
    aA_type type = vd->type;
    check_type_valid(out, type);
    // 检查是否重复声明
    string name = *(vd->id);
    A_pos pos = vd->pos;
    check_multiDeclaration(out, name, pos);
    // 保存信息至变量表  val没有用上，但是后续可能要用这个检查  ？？？？
    aA_rightVal val = vd->val;
    type->cur_scope = cur_scope;
    g_token2Type[name] = type;
}

void check_VarDefArray(std::ostream *out, aA_varDefArray vd)
{
    if (!vd)
        return;
    // 检查类型
    aA_type type = vd->type;
    check_type_valid(out, type);
    // 检查是否重复声明
    string name = *(vd->id);
    A_pos pos = vd->pos;
    check_multiDeclaration(out, name, pos);
    // 保存信息至变量表  len,vals没有用上，但是后续可能要用这个检查  ？？？
    int len = vd->len;
    vector<aA_rightVal> vals = vd->vals;
    type->cur_scope = cur_scope;
    g_token2Type[name] = type;
}

void check_VarDef(std::ostream *out, aA_varDef vd)
{
    if (!vd)
        return;
    string name;
    if (vd->kind == A_varDeclStmtType::A_varDeclKind)
    {
        check_VarDefScalar(out, vd->u.defScalar);
    }
    else if (vd->kind == A_varDeclStmtType::A_varDefKind)
    {
        check_VarDefArray(out, vd->u.defArray);
    }
    return;
}

void check_VarDecl(std::ostream *out, aA_varDeclStmt vd)
{
    // variable declaration statement
    if (!vd)
        return;
    string name;
    if (vd->kind == A_varDeclStmtType::A_varDeclKind)
    {
        // if declaration only
        // Example:
        //   let a:int;
        //   let a[5]:int;
        /* write your code here*/
        check_VarDec(out, vd->u.varDecl);
    }
    else if (vd->kind == A_varDeclStmtType::A_varDefKind)
    {
        // if both declaration and initialization
        // Example:
        //   let a:int = 5;
        /* write your code here */
        check_VarDef(out, vd->u.varDef);
    }
    return;
}

// 还需检查变量重名+所有变量类型合法  既适用于struct,也适用于函数传输的函数
void check_params(std::ostream *out, vector<aA_varDecl> varDeclList)
{

    vector<string> paramNames;
    for (auto varDecl : varDeclList)
    {
        string name;
        // 判断变量类型
        if (varDecl->kind == A_varDeclStmtType::A_varDeclKind)
        {

            aA_varDeclScalar vd = varDecl->u.declScalar;
            // 检查类型
            aA_type type = vd->type;
            check_type_valid(out, type);
            name = *(vd->id);
        }
        else if (varDecl->kind == A_varDeclStmtType::A_varDefKind)
        {
            aA_varDeclArray vd = varDecl->u.declArray;
            // 检查类型
            aA_type type = vd->type;
            check_type_valid(out, type);
            name = *(vd->id);
        }

        // 查看是否重名
        for (auto paramName : paramNames)
        {
            if (name == paramName)
            {
                error_print(out, varDecl->pos, "Declared a same variable name!");
            }
        }
        paramNames.push_back(name);
    }
}

void check_StructDef(std::ostream *out, aA_structDef sd)
{
    if (!sd)
        return;
    // structure definition
    // Example:
    //      struct A{
    //          a:int;
    //          b:int;
    //      }

    /* write your code here */
    if (!sd)
        return;
    // 检查是否重复声明
    string name = *(sd->id);
    A_pos pos = sd->pos;
    check_multiDeclaration(out, name, pos);
    // 还需检查变量重名+所有变量类型合法
    check_params(out, sd->varDecls);
    // 将其加入到struct2Members
    struct2Members[name] = sd->varDecls;
    return;
}

// paramMemberMap func2Param;
void check_FnDecl(std::ostream *out, aA_fnDecl fd)
{
    // Example:
    //      fn main(a:int, b:int)->int
    if (!fd)
        return;

    /*
        write your code here
        Hint: you may need to check if the function is already declared
    */
    // 检查返回类型
    aA_type type = fd->type;
    check_type_valid(out, type);
    // 还需检查变量重名+所有变量类型合法
    vector<aA_varDecl> varDecls = fd->paramDecl->varDecls;
    check_params(out, varDecls);
    return;
}

void check_FnDeclStmt(std::ostream *out, aA_fnDeclStmt fd)
{
    // Example:
    //      fn main(a:int, b:int)->int;
    if (!fd)
        return;
    // 检查是否重复声明（因为fndef和fndec共用此函数，但操作不同，需要移到外面）
    string name = *(fd->fnDecl->id);
    A_pos pos = fd->pos;
    check_multiDeclaration(out, name, pos);
    check_FnDecl(out, fd->fnDecl);
    // 将其加入到全局函数声明表中
    g_fnDec2Type[name] = fd->fnDecl->type;
    func2Param[name] = fd->fnDecl->paramDecl->varDecls;

    return;
}

// 检查函数是否重复定义
void check_multiFndef(std::ostream *out, string name, A_pos pos)
{

    if (g_fnDef2Type.find(name) != g_fnDef2Type.end())
    {
        error_print(out, pos, "This name " + name + " has been defined in func name map!");
    }
}

// 只检查是否定义，不向内部检查
void check_FnDef1(std::ostream *out, aA_fnDef fd)
{
    // Example:
    //      fn main(a:int, b:int)->int{
    //          let c:int;
    //          c = a + b;
    //          return c;
    //      }
    if (!fd)
        return;
    // 检查是否重复定义
    string name = *(fd->fnDecl->id);
    A_pos pos = fd->pos;
    check_multiFndef(out, name, pos);
    check_FnDecl(out, fd->fnDecl);
    // 将其加入到全局函数定义表,在判断后选择是否加入全局函数声明表
    g_fnDef2Type[name] = fd->fnDecl->type;
    if (!g_fnDec2Type[name])
    {
        g_fnDec2Type[name] = fd->fnDecl->type;
        func2Param[name] = fd->fnDecl->paramDecl->varDecls;
    }
    else
    {
        // 这里可能会有重载的问题，但是本次lab不做处理
    }

    return;
}

// 向内部检查
void check_FnDef1(std::ostream *out, aA_fnDef fd)
{
    // Example:
    //      fn main(a:int, b:int)->int{
    //          let c:int;
    //          c = a + b;
    //          return c;
    //      }
    if (!fd)
        return;

    // 函数全局已经检查过可以向内部检查
    string name = *(fd->fnDecl->id);
    vector<aA_varDecl> varDeclVec = func2Param[name];
    for (auto varDecl : varDeclVec)
    {
        string name;
        aA_type type;
        // 判断变量类型
        if (varDecl->kind == A_varDeclType::A_varDeclScalarKind)
        {

            aA_varDeclScalar vd = varDecl->u.declScalar;
            type = vd->type;
            name = *(vd->id);
        }
        else if (varDecl->kind == A_varDeclType::A_varDeclArrayKind)
        {
            aA_varDeclArray vd = varDecl->u.declArray;
            type = vd->type;
            name = *(vd->id);
        }
        curFuncParams[name] = type;
    }

    addCurScope();

    for (auto stmt : fd->stmts)
    {
        check_CodeblockStmt(out, stmt);
    }
    minusCurScope();
    curFuncParams.clear();
    return;
}

void check_CodeblockStmt(std::ostream *out, aA_codeBlockStmt cs)
{
    if (!cs)
        return;
    switch (cs->kind)
    {
    case A_codeBlockStmtType::A_varDeclStmtKind:
        check_VarDecl(out, cs->u.varDeclStmt);
        break;
    case A_codeBlockStmtType::A_assignStmtKind:
        check_AssignStmt(out, cs->u.assignStmt);
        break;
    case A_codeBlockStmtType::A_ifStmtKind:
        check_IfStmt(out, cs->u.ifStmt);
        break;
    case A_codeBlockStmtType::A_whileStmtKind:
        check_WhileStmt(out, cs->u.whileStmt);
        break;
    case A_codeBlockStmtType::A_callStmtKind:
        check_CallStmt(out, cs->u.callStmt);
        break;
    case A_codeBlockStmtType::A_returnStmtKind:
        check_ReturnStmt(out, cs->u.returnStmt);
        break;
    default:
        break;
    }
    return;
}

/*

struct aA_leftVal_
{
    A_pos pos;
    A_leftValType kind;
    union
    {
        string *id;
        aA_arrayExpr arrExpr;
        aA_memberExpr memberExpr;
    } u;
};
*/

// only gets the parameter type!!!
aA_type get_TypeById(string id)
{
    // 先查询当前函数
    if (curFuncParams.find(id) != curFuncParams.end())
        return curFuncParams[id];
    else if (g_token2Type.find(id) != g_token2Type.end())
        return g_token2Type[id];
    else if (struct2Members.find(id) != struct2Members.end())
        return     
    return NULL;
}

void check_AssignStmt(std::ostream *out, aA_assignStmt as)
{
    if (!as)
        return;
    string name;
    switch (as->leftVal->kind)
    {
    case A_leftValType::A_varValKind:
    {
        /* write your code here */
        // 查看是否未声明
        string name = *(as->leftVal->u.id);
        // 获得左值的type
        aA_type type = get_TypeById(name);
        if (type == nullptr)
            error_print(out, as->leftVal->pos, "Left Val here is not declared!");
        // return leftValType;
    }
    break;
    case A_leftValType::A_arrValKind:
    {
        /* write your code here */
        /* struct aA_arrayExpr_
                {
                    A_pos pos;
                    string *arr;
                    aA_indexExpr idx;
                };

         */

    }
    break;
    case A_leftValType::A_memberValKind:
    {
        /* write your code here */
    }
    break;
    }
    return;
}

void check_ArrayExpr(std::ostream *out, aA_arrayExpr ae)
{
    if (!ae)
        return;
    /*
        Example:
            a[1] = 0;
        Hint:
            check the validity of the array index
    */
}

aA_type check_MemberExpr(std::ostream *out, aA_memberExpr me)
{
    // check if the member exists and return the tyep of the member
    // you may need to check if the type of this expression matches with its
    // leftvalue or rightvalue, so return its aA_type would be a good way. Feel
    // free to change the design pattern if you need.
    if (!me)
        return nullptr;
    /*
        Example:
            a.b
    */
    return nullptr;
}

void check_IfStmt(std::ostream *out, aA_ifStmt is)
{
    if (!is)
        return;
    check_BoolExpr(out, is->boolExpr);
    for (aA_codeBlockStmt s : is->ifStmts)
    {
        check_CodeblockStmt(out, s);
    }
    for (aA_codeBlockStmt s : is->elseStmts)
    {
        check_CodeblockStmt(out, s);
    }
    return;
}

void check_BoolExpr(std::ostream *out, aA_boolExpr be)
{
    if (!be)
        return;
    switch (be->kind)
    {
    case A_boolExprType::A_boolBiOpExprKind:
        /* write your code here */
        break;
    case A_boolExprType::A_boolUnitKind:
        check_BoolUnit(out, be->u.boolUnit);
        break;
    default:
        break;
    }
    return;
}

void check_BoolUnit(std::ostream *out, aA_boolUnit bu)
{
    if (!bu)
        return;
    switch (bu->kind)
    {
    case A_boolUnitType::A_comOpExprKind:
    {
        /* write your code here */
    }
    break;
    case A_boolUnitType::A_boolExprKind:
        /* write your code here */
        break;
    case A_boolUnitType::A_boolUOpExprKind:
        /* write your code here */
        break;
    default:
        break;
    }
    return;
}

aA_type check_ExprUnit(std::ostream *out, aA_exprUnit eu)
{
    // validate the expression unit and return the aA_type of it
    // you may need to check if the type of this expression matches with its
    // leftvalue or rightvalue, so return its aA_type would be a good way.
    // Feel free to change the design pattern if you need.
    if (!eu)
        return nullptr;
    aA_type ret;
    switch (eu->kind)
    {
    case A_exprUnitType::A_idExprKind:
    {
        /* write your code here */
    }
    break;
    case A_exprUnitType::A_numExprKind:
    {
        /* write your code here */
    }
    break;
    case A_exprUnitType::A_fnCallKind:
    {
        /* write your code here */
    }
    break;
    case A_exprUnitType::A_arrayExprKind:
    {
        /* write your code here */
    }
    break;
    case A_exprUnitType::A_memberExprKind:
    {
        /* write your code here */
    }
    break;
    case A_exprUnitType::A_arithExprKind:
    {
        /* write your code here */
    }
    break;
    case A_exprUnitType::A_arithUExprKind:
    {
        /* write your code here */
    }
    break;
    }
    return ret;
}

void check_FuncCall(std::ostream *out, aA_fnCall fc)
{
    if (!fc)
        return;
    // Example:
    //      foo(1, 2);

    /* write your code here */
    return;
}

void check_WhileStmt(std::ostream *out, aA_whileStmt ws)
{
    if (!ws)
        return;
    check_BoolExpr(out, ws->boolExpr);
    for (aA_codeBlockStmt s : ws->whileStmts)
    {
        check_CodeblockStmt(out, s);
    }
    return;
}

void check_CallStmt(std::ostream *out, aA_callStmt cs)
{
    if (!cs)
        return;
    check_FuncCall(out, cs->fnCall);
    return;
}

void check_ReturnStmt(std::ostream *out, aA_returnStmt rs)
{
    if (!rs)
        return;
    return;
}
