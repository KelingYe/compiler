#include "TypeCheck.h"

// maps to store the type information. Feel free to design new data structures if you need.
typeMap token2Type;           // token ids to type(既包括全局也包括局部)
typeMap funcparam_token2Type; // func params token ids to type
typeMap g_fnDec2Type;         // 存储声明过的函数及其返回值
typeMap g_fnDef2Type;         // 存储已定义过的函数及其返回值
typeMap curFuncParams;        // 存储当前函数的参数列表中的每个参数id对应的type
std::string cur_func_name = "";
paramMemberMap func2Param;     // funtion name以及对应的函数列表
paramMemberMap struct2Members; // struct name 以及对应的成员参数

// 全局层数为1, 进入函数 ++, 离开函数 --
int cur_scope = 1;

void addCurScope()
{
    cur_scope++;
}

// 清除所有curScope的varDecl
void minusCurScope()
{
    vector<string> allDeleteNames;
    // 清除 all the variables
    for (auto i : token2Type)
    {
        // 也就是在存变量表时，需要存该变量的作用域
        if (i.second->cur_scope == cur_scope)
        {
            // 不能在循环中改变结构体的值
            allDeleteNames.push_back(i.first);
        }
    }
    for (auto i : allDeleteNames)
        token2Type.erase(i);
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
            check_FnDef1(out, ele->u.fnDef);
        }
        break;
        }
    }

    struct2Members.clear();
    for (auto ele : p->programElements)
    {
        switch (ele->kind)
        {

        case A_programStructDefKind:
        {
            check_StructDef(out, ele->u.structDef);
        }
        break;
        case A_programFnDefKind:
        {
            check_FnDef1(out, ele->u.fnDef);
        }
        break;
        }
    }

    (*out) << "Typecheck passed!" << std::endl;
    return;
}

// 根据type 获得名字(这里只有Var有，struct类型本身是没有的aA_type )
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
    if (name != "int" && struct2Members.find(name) == struct2Members.end())
        error_print(out, varType->pos, "Undeclared variable type!");
}

// 检查是否重复声明（可以var，struct一起检查，并且不需要判断type，因为它们的名字是唯一的）
void check_multiDeclaration(std::ostream *out, string name, A_pos pos)
{

    if (token2Type.find(name) != token2Type.end())
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
    type->is_array = false;
    token2Type[name] = type;
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
    // 保存信息至变量表
    type->cur_scope = cur_scope;
    type->len = len;
    type->is_array = true;
    token2Type[name] = type;
}

void check_VarDec(std::ostream *out, aA_varDecl vd)
{
    if (!vd)
        return;

    if (vd->kind == A_varDeclType::A_varDeclScalarKind)
    {
        check_VarDeclScalar(out, vd->u.declScalar);
    }
    else if (vd->kind == A_varDeclType::A_varDeclArrayKind)
    {
        check_VarDeclArray(out, vd->u.declArray);
    }
    return;
}

// 实现对右值的检查
aA_type getBoolType()
{
    aA_type type = new aA_type_;
    type->is_bool = true;
    type->is_array = false;
    return type;
}

aA_type getIntType()
{
    aA_type intType = new aA_type_;
    intType->is_array = false;
    intType->type = A_dataType::A_nativeTypeKind;
    return intType;
}

aA_type check_BoolUnit(std::ostream *out, aA_boolUnit bu)
{
    if (!bu)
        return nullptr;
    switch (bu->kind)
    {
    case A_boolUnitType::A_comOpExprKind:
    {
        aA_type exprUnit1 = check_ExprUnit(out, bu->u.comExpr->left);
        aA_type exprUnit2 = check_ExprUnit(out, bu->u.comExpr->right);
        if (check_aATypeSame(exprUnit1, getIntType()) == false)
            error_print(out, bu->u.comExpr->left->pos, "左值不是int类型");
        if (check_aATypeSame(exprUnit2, getIntType()) == false)
            error_print(out, bu->u.comExpr->right->pos, "右值不是int类型");
        aA_type retType = new aA_type_;
        retType->is_array = false;
        retType->is_bool = true;
        retType->pos = bu->pos;
        return retType;
    }
    break;
    case A_boolUnitType::A_boolExprKind:
    {
        return check_BoolExpr(out, bu->u.boolExpr);
    }
    break;
    case A_boolUnitType::A_boolUOpExprKind:
    {
        return check_BoolUnit(out, bu->u.boolUOpExpr->cond);
        break;
    }
    default:
        break;
    }
    return nullptr;
}
aA_type check_BoolExpr(std::ostream *out, aA_boolExpr be)
{
    if (!be)
        return nullptr;
    switch (be->kind)
    {
    case A_boolExprType::A_boolBiOpExprKind:
    {
        aA_type type1 = check_BoolExpr(out, be->u.boolBiOpExpr->left);
        aA_type type2 = check_BoolExpr(out, be->u.boolBiOpExpr->right);
        if (check_aATypeSame(getBoolType(), type1) == false)
            error_print(out, be->pos, "左值不是bool类型");
        if (check_aATypeSame(getBoolType(), type2) == false)
            error_print(out, be->pos, "右值不是bool类型");
        aA_type retType = new aA_type_;
        retType->is_array = false;
        retType->is_bool = true;
        retType->pos = be->pos;
        return retType;
    }
    break;
    case A_boolExprType::A_boolUnitKind:
    {
        aA_type type1 = check_BoolUnit(out, be->u.boolUnit);
        if (check_aATypeSame(getBoolType(), type1) == false)
            error_print(out, be->pos, "不是bool类型");
        return type1;
    }
    break;

    default:
        break;
    }
    return nullptr;
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
        string name = *(eu->u.id);
        auto t = get_TypeById(name);
        if (t == nullptr)
            error_print(out, eu->pos, "未找到变量名：" + name);
        return t;
    }
    break;
    case A_exprUnitType::A_numExprKind:
    {
        /* write your code here */
        ret = new aA_type_;
        ret->is_array = false;
        ret->pos = eu->pos;
        ret->type = A_dataType::A_nativeTypeKind;
        ret->u.nativeType = A_nativeType::A_intTypeKind;
        return ret;
    }
    break;
    case A_exprUnitType::A_fnCallKind:
    {
        /* write your code here */
        aA_type fnRetType = check_FuncCall(out, eu->u.callExpr);
        return fnRetType;
    }
    break;
    case A_exprUnitType::A_arrayExprKind:
    {
        /* write your code here */
        return check_ArrayExpr(out, eu->u.arrayExpr);
    }
    break;
    case A_exprUnitType::A_memberExprKind:
    {
        /* write your code here */
        return check_MemberExpr(out, eu->u.memberExpr);
    }
    break;
    case A_exprUnitType::A_arithExprKind:
    {
        /* write your code here */
        return check_arithExprValValid(out, eu->u.arithExpr);
    }
    break;
    case A_exprUnitType::A_arithUExprKind:
    {
        /* write your code here */
        return check_ExprUnit(out, eu->u.arithUExpr->expr);
    }
    break;
    }
    return ret;
}

aA_type check_arithBiopExpr(std::ostream *out, aA_arithBiOpExpr biopExpr)
{
    aA_type operand1Type = check_arithExprValValid(out, biopExpr->left);
    aA_type operand2Type = check_arithExprValValid(out, biopExpr->right);
    if (check_aATypeSame(getIntType(), operand1Type) == false)
        error_print(out, operand1Type->pos, "运算单元不是int类型");
    if (check_aATypeSame(getIntType(), operand2Type) == false)
        error_print(out, operand2Type->pos, "运算单元不是int类型");
    return operand1Type;
}

aA_type check_arithExprValValid(std::ostream *out, aA_arithExpr arithExpr)
{
    if (arithExpr == nullptr)
        return nullptr;
    switch (arithExpr->kind)
    {
    case (A_arithExprType::A_exprUnitKind):
    {
        return check_ExprUnit(out, arithExpr->u.exprUnit);
        break;
    }
    case (A_arithExprType::A_arithBiOpExprKind):
    {
        return check_arithBiopExpr(out, arithExpr->u.arithBiOpExpr);
        break;
    }
    }
    return nullptr;
}

aA_type check_rightValValid(std::ostream *out, aA_rightVal rightVal)
{
    if (rightVal == nullptr)
        return nullptr;
    switch (rightVal->kind)
    {
    case (A_rightValType::A_arithExprValKind):
    {
        return check_arithExprValValid(out, rightVal->u.arithExpr);
        break;
    }
    case (A_rightValType::A_boolExprValKind):
    {
        return check_BoolExpr(out, rightVal->u.boolExpr);
        break;
    }
    }
    return nullptr;
}

// 检查两个type是否相同
bool check_aATypeSame(aA_type a, aA_type b)
{
    if (a->is_bool == true && b->is_bool == true)
        return true;
    else if (a->is_bool == true || b->is_bool == true)
        return false;
    else
    {
        if (a->is_array == b->is_array)
        {
            if (a->type == b->type)
            {
                if (a->type == A_dataType::A_nativeTypeKind)
                {
                    return true;
                }
                else
                {
                    if (*(a->u.structType) == *(b->u.structType))
                    {
                        return true;
                    }
                    else
                        return false;
                }
            }
            else
                return false;
        }
        else
            return false;
    }
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
    // 检查右值 ？？？
    //  保存信息至变量表  (val没有用上，？？？）
    aA_rightVal val = vd->val;
    type->is_array = false;
    type->cur_scope = cur_scope;
    token2Type[name] = type;
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
    // 检查右值
    //  保存信息至变量表  vals没有用上 ？？？
    int len = vd->len;
    vector<aA_rightVal> vals = vd->vals;
    type->cur_scope = cur_scope;
    type->len = len;
    type->is_array = true;
    token2Type[name] = type;
}

void check_VarDef(std::ostream *out, aA_varDef vd)
{
    if (!vd)
        return;
    string name;
    if (vd->kind == A_varDefType::A_varDefScalarKind)
    {
        check_VarDefScalar(out, vd->u.defScalar);
    }
    else if (vd->kind == A_varDefType::A_varDefScalarKind)
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
// 根据 varDecl得到type类型
aA_type getVarDeclType(aA_varDecl x)
{
    aA_type ret = nullptr;
    switch (x->kind)
    {
    case (A_varDeclType::A_varDeclScalarKind):
    {
        x->u.declScalar->type->is_array = false;
        return x->u.declScalar->type;
    }
    break;
    case (A_varDeclType::A_varDeclArrayKind):
    {
        x->u.declArray->type->is_array = true;
        return x->u.declArray->type;
    }
    break;
    }
    return ret;
}
// 根据 varDecl得到name
string getVarDeclName(aA_varDecl x)
{
    if (x == nullptr)
        return nullptr;
    switch (x->kind)
    {

    case (A_varDeclType::A_varDeclArrayKind):
    {
        return *(x->u.declArray->id);
    }
    break;
    case (A_varDeclType::A_varDeclScalarKind):
    {
        return *(x->u.declScalar->id);
    }
    break;
    }
    return "";
}

// 还需检查变量重名+所有变量类型合法  既适用于struct,也适用于函数传输的函数
void check_params(std::ostream *out, vector<aA_varDecl> varDeclList)
{

    vector<string> paramNames;
    for (auto varDecl : varDeclList)
    {

        string name = getVarDeclName(varDecl);
        aA_type type = getVarDeclType(varDecl);
        // 检查类型
        check_type_valid(out, type);
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
    else // 这里可能会有重载的问题，但是本次lab不做考虑所以只处理定义与声明不一致的问题 ？？？
    {
        // if(g_fnDec2Type[name]->ty)
    }

    return;
}

// 向内部检查
void check_FnDef2(std::ostream *out, aA_fnDef fd)
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
    // 存储所有的函数参数
    for (auto varDecl : varDeclVec)
    {
        string name = getVarDeclName(varDecl);
        aA_type type = getVarDeclType(varDecl);
        curFuncParams[name] = type;
    }
    // 获取当前的函数名
    cur_func_name = name;
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

// only gets the parameter type!!!（但是struct类型是获取不了的）
aA_type get_TypeById(string id)
{
    // 先查询当前函数
    if (curFuncParams.find(id) != curFuncParams.end())
        return curFuncParams[id];
    else if (token2Type.find(id) != token2Type.end())
        return token2Type[id];
    else if (struct2Members.find(id) != struct2Members.end())
    {
        aA_type atype = new aA_type_;
        atype->type = A_structTypeKind;
        return atype;
    }
    return NULL;
}

void check_AssignStmt(std::ostream *out, aA_assignStmt as)
{
    if (!as)
        return;
    // should assume that right val and left val may be nullptrs
    aA_type rightValType = check_rightValValid(out, as->rightVal);
    // check left val valid
    aA_type leftValType = check_leftValValid(out, as->leftVal);
    if (check_aATypeSame(leftValType, rightValType) != true)
        error_print(out, as->pos, "The type of left val and right val doesn't match here!");
    return;

    return;
}

aA_type check_leftValValid(std::ostream *out, aA_leftVal leftVal)
{
    switch (leftVal->kind)
    {
    case A_leftValType::A_varValKind:
    {
        /* write your code here */
        // 查看是否未声明
        string name = *(leftVal->u.id);
        // 获得左值的type
        aA_type type = get_TypeById(name);
        if (type == nullptr)
        {

            error_print(out, leftVal->pos, "Left Val here is not declared!");
        }

        return type;
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
       return check_ArrayExpr(out, leftVal->u.arrExpr);
    }
    break;
    case A_leftValType::A_memberValKind:
    {
        /* write your code here */
       return check_MemberExpr(out, leftVal->u.memberExpr);
    }
    break;
    }
}

aA_type check_ArrayExpr(std::ostream *out, aA_arrayExpr ae)
{
    if (!ae)
        return;
    /*
        Example:
            a[1] = 0;
        Hint:
            check the validity of the array index
    */
    if (!ae)
        return nullptr;
    string arrName = *(ae->arr);

    aA_type arrType = get_TypeById(arrName);
    if (arrType == nullptr)
        error_print(out, ae->pos, "Array Name here is not declared!");
    // check arrType
    if (arrType->is_array == false)
        error_print(out, ae->pos, "This variable " + arrName + " is a scalar type!");
    switch (ae->idx->kind)
    {
    case (A_indexExprKind::A_numIndexKind):
    {
        // 不直接让retType = arrType的目的是害怕retType->的修改也会影响到arrType的修改
        aA_type retType = new aA_type_;
        retType->cur_scope = arrType->cur_scope;
        retType->is_array = false;
        retType->pos = arrType->pos;
        retType->type = arrType->type;
        retType->u = arrType->u;
        return retType;
    }
    break;
    case (A_indexExprKind::A_idIndexKind):
    {
        string bTypeName = *(ae->idx->u.id);
        aA_type bType = get_TypeById(bTypeName);
        if (bType == nullptr)
            error_print(out, ae->idx->pos, "The name of the val in array brackets here is not declared!");
        if (bType->is_array == true)
            error_print(out, ae->idx->pos, "The name of the val in array brackets here is an array!");
        if (bType->type == A_dataType::A_structTypeKind)
            error_print(out, ae->idx->pos, "The name of the val in array brackets here is a struct!");
        aA_type retType = new aA_type_;
        retType->cur_scope = arrType->cur_scope;
        retType->is_array = false;
        retType->pos = arrType->pos;
        retType->type = arrType->type;
        retType->u = arrType->u;
        return retType;
    }
    }
    return nullptr;
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
    string structVarName = *(me->structId);
    aA_type structVarType = get_TypeById(structVarName);
    if (structVarType == nullptr)
        error_print(out, me->pos, "The variable " + structVarName + " defined here is not declared!");
    if (structVarType->is_array == true)
        error_print(out, me->pos, "The variable " + structVarName + " used here is an array!");
    if (structVarType->type == A_dataType::A_nativeTypeKind)
        error_print(out, me->pos, "The variable " + structVarName + " used here is a int!");
    // now we confirm a is a struct
    string innerVarName = *(me->memberId);

    // check innerVarName in struct or not
    vector<aA_varDecl> varDeclVec = struct2Members[*(structVarType->u.structType)];
    aA_type matchType = nullptr;
    for (auto varDecl : varDeclVec)
    {
        // if(varDecl==nullptr)
        // std::cout<<"Should not be a nullptr in varDecl"<<std::endl;
        string varDeclName = getVarDeclName(varDecl);
        // if(TypeCheckDebug) std::cout<<"This var decl name is "<<varDeclName<<std::endl;
        if (varDeclName == innerVarName)
        {
            matchType = getVarDeclType(varDecl);
        }
    }
    if (matchType == nullptr)
        error_print(out, me->pos, "The inner variable " + innerVarName + " used in this struct is not defined!");
    // return b type
    return matchType;
    return nullptr;
}

void check_IfStmt(std::ostream *out, aA_ifStmt is)
{
    if (!is)
        return;
    check_BoolExpr(out, is->boolExpr);
    addCurScope();
    for (aA_codeBlockStmt s : is->ifStmts)
    {
        check_CodeblockStmt(out, s);
    }
    minusCurScope();
    addCurScope();
    for (aA_codeBlockStmt s : is->elseStmts)
    {
        check_CodeblockStmt(out, s);
    }
    minusCurScope();
    return;
}

void check_WhileStmt(std::ostream *out, aA_whileStmt ws)
{
    if (!ws)
        return;
    check_BoolExpr(out, ws->boolExpr);
    addCurScope();
    for (aA_codeBlockStmt s : ws->whileStmts)
    {
        check_CodeblockStmt(out, s);
    }
    minusCurScope();
    return;
}

// 实际上来是不需要返回type值，因为fncall不作为右值
aA_type check_FuncCall(std::ostream *out, aA_fnCall fc)
{
    if (!fc)
        return;
    // Example:
    //      foo(1, 2);

    /* write your code here */
    string fnName = *(fc->fn);
    if (g_fnDef2Type.find(fnName) == g_fnDef2Type.end())
        error_print(out, fc->pos, "未找到函数名：" + fnName);
    auto varDeclVec = func2Param[fnName];
    if (varDeclVec.size() != fc->vals.size())
        error_print(out, fc->pos, "函数调用参数数量不合法：" + fnName);
    for (int i = 0; i < varDeclVec.size(); i++)
    {
        auto varDecl = varDeclVec[i];
        auto rightVal = fc->vals[i];
        if (!check_aATypeSame(getVarDeclType(varDecl), check_rightValValid(out, rightVal)))
        {
            error_print(out, fc->pos, "参数 " + std::to_string(i) + " 类型不合法：" + fnName);
        }
    }
    return g_fnDef2Type[fnName];
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
    aA_type rightValType = check_rightValValid(out, rs->retVal);
    aA_type funcRetType = g_fnDef2Type[cur_func_name];
    if (check_aATypeSame(funcRetType, rightValType) == false)
    {
        error_print(out, rs->pos, "返回值类型与函数定义类型不同");
    }
    return;
}
