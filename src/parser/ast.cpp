module;

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <sstream>
#include <variant>
#include <optional>
#include <map>

export module ast;

import token;
import symbol;
import terminalfactory;

export enum DataType {
    INT_T,
    FLOAT_T,
    STR_T,
    BOOL_T,
    FUNC_T,
    ANY_T,
    NONE_T
};

const std::map<DataType, std::string_view> dataTypeNamesMap {
    {DataType::INT_T, "int"},
    {DataType::FLOAT_T, "float"},
    {DataType::STR_T, "str"},
    {DataType::BOOL_T, "bool"},
    {DataType::ANY_T, "any"},
    {DataType::FUNC_T, "func"},
    {DataType::NONE_T, "none"}
};

export struct TypeCheckSuccess {
    DataType type;
};
export struct TypeCheckError {
    std::string message;
    std::string where;
};
export using TypeCheckResult = std::variant<TypeCheckSuccess, TypeCheckError>;

export struct SymbolTableEntry {
    std::string name;
    DataType type;
    bool isArray;

    SymbolTableEntry(const std::string& name, DataType type, bool isArray)
        : name(name), type(type), isArray(isArray) {}
};

export using SymbolTable = std::map<std::string, SymbolTableEntry>;

export struct SymbolTableNode {
    SymbolTable* table;
    int scope;
    const SymbolTableNode* parent;

    SymbolTableNode(SymbolTable* table)
        : table(table), scope(0), parent(nullptr) {}

    SymbolTableNode(SymbolTable* table, int scope, const SymbolTableNode& parent)
        : table(table), scope(scope), parent(&parent) {}

    SymbolTableNode createChild(SymbolTable* childTable) const {
        return SymbolTableNode{ childTable, scope + 1, *this };
    }
};

export struct Quadruple {
    std::string op;
    std::string arg1;
    std::string arg2;
    std::string result;

    std::string toString() const {
        std::ostringstream oss;
        oss << "( " << op << ", " << arg1 << ", " << arg2 << ", " << result << " )";
        return oss.str();
    }
};

export struct Label {
    int id;

    std::string getName() const {
        std::ostringstream oss;
        oss << "L" << id;
        return oss.str();
    }

    std::string toString() const {
        std::ostringstream oss;
        oss << getName() << ":";
        return oss.str();
    }
};

export using Quadruples = std::vector<std::variant<Quadruple, Label>>;

export struct GeQ {
    Quadruples quads;
    std::string result;
};

export class AstNode {
    private:
        const std::string name;

    protected:
        std::string getIntermediate(int intermediateId) const {
            std::ostringstream oss;
            oss << "t" << intermediateId;
            return oss.str();
        }

        Label getLabel(int& globalLabelId) const {
            Label label{ globalLabelId };
            globalLabelId++;
            return label;
        }

        bool checkType(const DataType type, const std::vector<DataType>& allowedTypes) const {
            for (const auto& allowedType : allowedTypes) {
                if (type == allowedType) {
                    return true;
                }
            }
            if (type == DataType::ANY_T) {
                return true;
            }
            return false;
        }

        bool typeEquals(const DataType type1, const DataType type2) const {
            return type1 == type2 || type1 == DataType::ANY_T || type2 == DataType::ANY_T;
        }

        std::string getTypeName(const DataType& type) const {
            auto it = dataTypeNamesMap.find(type);
            if (it != dataTypeNamesMap.end()) {
                return std::string(it->second);
            }
            return "unknown";
        }

        std::optional<SymbolTableEntry> findSymbol(const SymbolTableNode& symbolTableNode, const std::string& name) const {
            auto it = symbolTableNode.table->find(name);
            if (it != symbolTableNode.table->end()) {
                return it->second;
            }
            if (symbolTableNode.parent != nullptr) {
                return findSymbol(*symbolTableNode.parent, name);
            }
            return std::nullopt;
        }

        // std::string openString() const {
        //     std::ostringstream oss;
        //     oss << name << "( ";
        //     return oss.str();
        // }
        // std::string closeString() const {
        //     return " )";
        // }
        // std::string itemToString(const std::variant<Token, AstNode*>& child, const int i) const {
        //     std::ostringstream oss;
        //     if (i > 0) {
        //         oss << ", ";
        //     }
        //     if (std::holds_alternative<Token>(child)) {
        //         oss << std::get<Token>(child).toStringPrint();
        //     } else {
        //         oss << std::get<AstNode*>(child)->toString();
        //     }
        //     return oss.str();
        // }

    public:
        AstNode(const std::string& name): name(name) {}
        virtual ~AstNode() = default;

        // virtual std::string toString() const = 0;

        virtual std::string getWhere() const = 0;

        virtual GeQ toQuadruples(int& globalLabelId, int intermediateId = 0) const = 0;

        std::string toQuadrupleString() const {
            int globalLabelId = 0;
            const auto quads = toQuadruples(globalLabelId, 0).quads;
            std::ostringstream oss;
            for (const auto& quad : quads) {
                if (std::holds_alternative<Quadruple>(quad)) {
                    oss << std::get<Quadruple>(quad).toString() << "\n";
                } else {
                    oss << std::get<Label>(quad).toString() << "\n";
                }
            }
            return oss.str();
        }

        virtual TypeCheckResult typeCheck(const SymbolTableNode& symbolTableNode, const DataType assignedType) const = 0;

        TypeCheckResult startTypeCheck() const {
            SymbolTable symbolTable{}; // starting table
            SymbolTableNode symbolTableNode{&symbolTable};
            return this->typeCheck(symbolTableNode, DataType::NONE_T);
        }
};

export class Start : public AstNode {
    private:
        std::vector<std::unique_ptr<AstNode>> declarations;

    public:
        Start(std::vector<std::unique_ptr<AstNode>> declarations): AstNode("Start"), declarations(std::move(declarations)) {}
        ~Start() = default;

        std::string getWhere() const override {
            return declarations[0]->getWhere();
        }

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            for (const auto& decl : declarations) {
                const auto declGeQ = decl->toQuadruples(globalLabelId);
                const auto declQuads = declGeQ.quads;
                quads.insert(quads.end(), declQuads.begin(), declQuads.end());
            }
            return { quads, "" };
        }

        TypeCheckResult typeCheck(const SymbolTableNode& symbolTableNode, const DataType assignedType) const override {
            for (const auto& decl : declarations) {
                const auto result = decl->typeCheck(symbolTableNode, DataType::NONE_T);
                if (std::holds_alternative<TypeCheckError>(result)) {
                    return result;
                }
            }
            return TypeCheckSuccess{ DataType::NONE_T };
        }
}; // declarations: AstNode[]

export class FuncDef : public AstNode {
    private:
        std::unique_ptr<AstNode> type;
        Token id;
        std::vector<std::unique_ptr<AstNode>> params;
        std::unique_ptr<AstNode> body;

    public:
        FuncDef(std::unique_ptr<AstNode> type, Token id, std::vector<std::unique_ptr<AstNode>> params, std::unique_ptr<AstNode> body)
            : AstNode("FuncDef"), type(std::move(type)), id(id), params(std::move(params)), body(std::move(body)) {}
        ~FuncDef() = default;

        std::string getWhere() const override {
            return type->getWhere();
        }

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            quads.emplace_back(Quadruple{"FUNCTION", id.getValue(), std::to_string(params.size()), ""});
            for (int i = 0; i < params.size(); ++i) {
                const auto paramsGeQ = params[i]->toQuadruples(globalLabelId);
                quads.emplace_back(Quadruple{"PARAM", paramsGeQ.result, std::to_string(i + 1), ""});
            }
            const auto bodyGeQ = body->toQuadruples(globalLabelId);
            quads.insert(quads.end(), bodyGeQ.quads.begin(), bodyGeQ.quads.end());
            quads.emplace_back(Quadruple{"ENDFUNC", id.getValue(), "", ""});
            return { quads, "" };
        }

        TypeCheckResult typeCheck(const SymbolTableNode& symbolTableNode, const DataType assignedType) const override {
            SymbolTable symbolTable{};
            SymbolTableNode newSymbolTableNode = symbolTableNode.createChild(&symbolTable);
            const auto typeResult = type->typeCheck(newSymbolTableNode, DataType::NONE_T);
            if (std::holds_alternative<TypeCheckError>(typeResult)) {
                return typeResult;
            }
            symbolTableNode.table->emplace(id.getValue(), SymbolTableEntry{id.getValue(), DataType::FUNC_T, false});
            for (const auto& param : params) {
                const auto result = param->typeCheck(newSymbolTableNode, DataType::NONE_T);
                if (std::holds_alternative<TypeCheckError>(result)) {
                    return result;
                }
            }
            return body->typeCheck(newSymbolTableNode, DataType::NONE_T);
        }
}; // type: Token, id: Token, params: AstNode[], body: AstNode

export class Param : public AstNode {
    private:
        std::unique_ptr<AstNode> type;
        Token id;
        bool array;

    public:
        Param(std::unique_ptr<AstNode> type, Token id, bool array): AstNode("Param"), type(std::move(type)), id(id), array(array) {}
        ~Param() = default;

        std::string getWhere() const override {
            return type->getWhere();
        }

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            return { {}, id.getValue() };
        }

        TypeCheckResult typeCheck(const SymbolTableNode& symbolTableNode, const DataType assignedType) const override {
            const auto typeResult = type->typeCheck(symbolTableNode, DataType::NONE_T);
            if (std::holds_alternative<TypeCheckError>(typeResult)) {
                return typeResult;
            }
            const auto typeType = std::get<TypeCheckSuccess>(typeResult).type;
            SymbolTableEntry entry{id.getValue(), typeType, array};
            symbolTableNode.table->emplace(id.getValue(), entry);
            return TypeCheckSuccess{ DataType::NONE_T };
        }
}; // type: Token, id: Token, array: bool

export class VarDecl : public AstNode {
    private:
        std::unique_ptr<AstNode> type;
        std::vector<std::unique_ptr<AstNode>> varAssignables;

    public:
        VarDecl(std::unique_ptr<AstNode> type, std::vector<std::unique_ptr<AstNode>> varAssignables): AstNode("VarDecl"), type(std::move(type)), varAssignables(std::move(varAssignables)) {}
        ~VarDecl() = default;

        std::string getWhere() const override {
            return type->getWhere();
        }

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            for (const auto& var : varAssignables) {
                const auto varGeQ = var->toQuadruples(globalLabelId);
                const auto varQuads = varGeQ.quads;
                quads.insert(quads.end(), varQuads.begin(), varQuads.end());
            }
            return { quads, "" };
        }

        TypeCheckResult typeCheck(const SymbolTableNode& symbolTableNode, const DataType assignedType) const override {
            const auto typeResult = type->typeCheck(symbolTableNode, DataType::NONE_T);
            if (std::holds_alternative<TypeCheckError>(typeResult)) {
                return typeResult;
            }
            const auto typeType = std::get<TypeCheckSuccess>(typeResult).type;
            for (const auto& var : varAssignables) {
                const auto result = var->typeCheck(symbolTableNode, typeType);
                if (std::holds_alternative<TypeCheckError>(result)) {
                    return result;
                }
            }
            return TypeCheckSuccess{ DataType::NONE_T };
        }
}; // type: Token, varAssignables: AstNode[]

export class VarAssignable : public AstNode {
    private:
        std::unique_ptr<AstNode> var;
        std::optional<std::unique_ptr<AstNode>> expr;

    public:
        VarAssignable(std::unique_ptr<AstNode> var, std::optional<std::unique_ptr<AstNode>> expr)
            : AstNode("VarAssignable"), var(std::move(var)), expr(std::move(expr)) {}
        ~VarAssignable() = default;

        std::string getWhere() const override {
            return var->getWhere();
        }

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            if (expr) {
                const auto varGeQ = var->toQuadruples(globalLabelId);
                const auto varResult = varGeQ.result;

                const auto valueGeQ = (*expr)->toQuadruples(globalLabelId, intermediateId + 1);
                const auto valueQuads = valueGeQ.quads;
                const auto valueResult = valueGeQ.result;
                quads.insert(quads.end(), valueQuads.begin(), valueQuads.end());
                quads.emplace_back(Quadruple{"=", valueResult, "", varResult});
            }
            return { quads, "" };
        }

        TypeCheckResult typeCheck(const SymbolTableNode& symbolTableNode, const DataType assignedType) const override {
            DataType valueType = DataType::NONE_T;
            if (expr) {
                const auto valueResult = (*expr)->typeCheck(symbolTableNode, DataType::NONE_T);
                if (std::holds_alternative<TypeCheckError>(valueResult)) {
                    return valueResult;
                }
                valueType = std::get<TypeCheckSuccess>(valueResult).type;
            }
            const auto varResult = var->typeCheck(symbolTableNode, assignedType);
            if (std::holds_alternative<TypeCheckError>(varResult)) {
                return varResult;
            }
            const auto varType = std::get<TypeCheckSuccess>(varResult).type;
            if (expr) {
                if (expr && !typeEquals(varType, valueType)) {
                    return TypeCheckError{ "Type mismatch: " + getTypeName(varType) + " and " + getTypeName(valueType), getWhere() };
                }
                return TypeCheckSuccess{ valueType };
            }
            return TypeCheckSuccess{ DataType::NONE_T };
        }
}; // var: AstNode, expr?: AstNode

export class Var : public AstNode {
    private:
        Token id;
        std::optional<std::unique_ptr<AstNode>> arrayIndex;

    public:
        Var(Token id, std::optional<std::unique_ptr<AstNode>> arrayIndex): AstNode("Var"), id(id), arrayIndex(std::move(arrayIndex)) {}
        ~Var() = default;

        std::string getWhere() const override {
            return id.getPosition();
        }

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            if (arrayIndex) {
                Quadruples quads;
                const auto arrayIndexGeQ = (*arrayIndex)->toQuadruples(globalLabelId, intermediateId + 1);
                quads.insert(quads.end(), arrayIndexGeQ.quads.begin(), arrayIndexGeQ.quads.end());
                const auto intermediate = getIntermediate(intermediateId);
                quads.emplace_back(Quadruple{"[]", id.getValue(), arrayIndexGeQ.result, intermediate});
                return { quads, intermediate };
            }
            return { {}, id.getValue() };
        }

        TypeCheckResult typeCheck(const SymbolTableNode& symbolTableNode, const DataType assignedType) const override {
            if (assignedType != DataType::NONE_T) {
                SymbolTableEntry entry{id.getValue(), assignedType, arrayIndex.has_value()};
                symbolTableNode.table->insert_or_assign(id.getValue(), entry);
            }
            auto entry = findSymbol(symbolTableNode, id.getValue());
            if (entry.has_value()) {
                if (entry->isArray) {
                    if (!arrayIndex.has_value()) {
                        return TypeCheckError{ "Array variable used without index: " + id.getValue(), getWhere() };
                    }
                    const auto arrayIndexResult = (*arrayIndex)->typeCheck(symbolTableNode, DataType::NONE_T);
                    if (std::holds_alternative<TypeCheckError>(arrayIndexResult)) {
                        return arrayIndexResult;
                    }
                    const auto arrayIndexType = std::get<TypeCheckSuccess>(arrayIndexResult).type;
                    if (!checkType(arrayIndexType, {DataType::INT_T})) {
                        return TypeCheckError{ "Array index must be int: " + id.getValue(), getWhere() };
                    }
                }
                else {
                    if (arrayIndex.has_value()) {
                        return TypeCheckError{ "Non-array variable used with index: " + id.getValue(), getWhere() };
                    }
                }
                return TypeCheckSuccess{ entry.value().type }; // has type
            }
            return TypeCheckError{ "Variable not found: " + id.getValue(), getWhere() };
        }
}; // id: Token, arrayIndex?: Token

export class Type : public AstNode {
    private:
        Token type;

    public:
        Type(Token type): AstNode("Type"), type(type) {}
        ~Type() = default;

        std::string getWhere() const override {
            return type.getPosition();
        }

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            return { {}, type.getValue() };
        }

        TypeCheckResult typeCheck(const SymbolTableNode& symbolTableNode, const DataType assignedType) const override {
            if (TerminalFactory::getKeyword("int").matchesToken(type)) {
                return TypeCheckSuccess{ DataType::INT_T };
            }
            else if (TerminalFactory::getKeyword("float").matchesToken(type)) {
                return TypeCheckSuccess{ DataType::FLOAT_T };
            }
            else if (TerminalFactory::getKeyword("str").matchesToken(type)) {
                return TypeCheckSuccess{ DataType::STR_T };
            }
            return TypeCheckError{ "Invalid type", getWhere() };
        }
}; // type: Token

export class Constant : public AstNode {
    private:
        Token value;

    public:
        Constant(Token value): AstNode("Constant"), value(value) {}
        ~Constant() = default;

        std::string getWhere() const override {
            return value.getPosition();
        }

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            return { {}, value.getValue() };
        }

        TypeCheckResult typeCheck(const SymbolTableNode& symbolTableNode, const DataType assignedType) const override {
            if (value.getType() == TokenType::INTEGER) {
                return TypeCheckSuccess{ DataType::INT_T };
            }
            else if (value.getType() == TokenType::FLOAT) {
                return TypeCheckSuccess{ DataType::FLOAT_T };
            }
            else if (value.getType() == TokenType::STRING) {
                return TypeCheckSuccess{ DataType::STR_T };
            }
            return TypeCheckError{ "Invalid constant type", getWhere() };
        }
}; // value: Token

export class BlockStmt : public AstNode {
    private:
        std::vector<std::unique_ptr<AstNode>> statements;

    public:
        BlockStmt(std::vector<std::unique_ptr<AstNode>> stmts): AstNode("BlockStmt"), statements(std::move(stmts)) {}
        ~BlockStmt() = default;

        std::string getWhere() const override {
            return statements[0]->getWhere();
        }

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            for (const auto& stmt : statements) {
                const auto stmtGeQ = stmt->toQuadruples(globalLabelId);
                const auto stmtQuads = stmtGeQ.quads;
                quads.insert(quads.end(), stmtQuads.begin(), stmtQuads.end());
            }
            return { quads, "" };
        }

        TypeCheckResult typeCheck(const SymbolTableNode& symbolTableNode, const DataType assignedType) const override {
            SymbolTable symbolTable{};
            SymbolTableNode newSymbolTableNode = symbolTableNode.createChild(&symbolTable);
            for (const auto& stmt : statements) {
                const auto result = stmt->typeCheck(newSymbolTableNode, DataType::NONE_T);
                if (std::holds_alternative<TypeCheckError>(result)) {
                    return result;
                }
            }
            return TypeCheckSuccess{ DataType::NONE_T };
        }
}; // stmts: AstNode[]

export class IfStmt : public AstNode {
    private:
        std::unique_ptr<AstNode> condExpr;
        std::unique_ptr<AstNode> thenBody;
        std::optional<std::unique_ptr<AstNode>> elseBody;

    public:
        IfStmt(std::unique_ptr<AstNode> condExpr, std::unique_ptr<AstNode> thenBody, std::optional<std::unique_ptr<AstNode>> elseBody)
            : AstNode("IfStmt"), condExpr(std::move(condExpr)), thenBody(std::move(thenBody)), elseBody(std::move(elseBody)) {}
        ~IfStmt() = default;

        std::string getWhere() const override {
            return condExpr->getWhere();
        }

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            const auto label1 = getLabel(globalLabelId);
            const auto label2 = getLabel(globalLabelId);

            const auto condGeQ = condExpr->toQuadruples(globalLabelId, intermediateId + 1);
            quads.insert(quads.end(), condGeQ.quads.begin(), condGeQ.quads.end());
            quads.emplace_back(Quadruple{"if", condGeQ.result, "", label1.getName()});
            quads.emplace_back(Quadruple{"goto", "", "", label2.getName()});

            quads.emplace_back(label1);
            const auto thenGeQ = thenBody->toQuadruples(globalLabelId);
            const auto thenQuads = thenGeQ.quads;
            quads.insert(quads.end(), thenQuads.begin(), thenQuads.end());

            if (elseBody) {
                const auto label3 = getLabel(globalLabelId);
                quads.emplace_back(Quadruple{"goto", "", "", label3.getName()});

                quads.emplace_back(label2);
                const auto elseGeQ = (*elseBody)->toQuadruples(globalLabelId);
                const auto elseQuads = elseGeQ.quads;
                quads.insert(quads.end(), elseQuads.begin(), elseQuads.end());

                quads.emplace_back(label3);
            }
            else {
                quads.emplace_back(label2);
            }
            return { quads, "" };
        }

        TypeCheckResult typeCheck(const SymbolTableNode& symbolTableNode, const DataType assignedType) const override {
            SymbolTable symbolTable{};
            SymbolTableNode newSymbolTableNode = symbolTableNode.createChild(&symbolTable);
            const auto condResult = condExpr->typeCheck(newSymbolTableNode, DataType::NONE_T);
            if (std::holds_alternative<TypeCheckError>(condResult)) {
                return condResult;
            }
            if (!checkType(std::get<TypeCheckSuccess>(condResult).type, {DataType::BOOL_T, DataType::INT_T})) {
                return TypeCheckError{ "Condition must be boolean", condExpr->getWhere() };
            }
            const auto thenResult = thenBody->typeCheck(newSymbolTableNode, DataType::NONE_T);
            if (std::holds_alternative<TypeCheckError>(thenResult)) {
                return thenResult;
            }
            if (elseBody) {
                const auto elseResult = (*elseBody)->typeCheck(newSymbolTableNode, DataType::NONE_T);
                if (std::holds_alternative<TypeCheckError>(elseResult)) {
                    return elseResult;
                }
            }
            return TypeCheckSuccess{ DataType::NONE_T };
        }
}; // condExpr: AstNode, thenBody: AstNode, elseBody: AstNode

export class WhileStmt : public AstNode {
    private:
        std::unique_ptr<AstNode> condExpr;
        std::unique_ptr<AstNode> body;

    public:
        WhileStmt(std::unique_ptr<AstNode> condExpr, std::unique_ptr<AstNode> body)
            : AstNode("WhileStmt"), condExpr(std::move(condExpr)), body(std::move(body)) {}
        ~WhileStmt() = default;

        std::string getWhere() const override {
            return condExpr->getWhere();
        }

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            const auto label1 = getLabel(globalLabelId);
            const auto label2 = getLabel(globalLabelId);
            const auto label3 = getLabel(globalLabelId);

            quads.emplace_back(label1);
            const auto condGeQ = condExpr->toQuadruples(globalLabelId, intermediateId + 1);
            quads.insert(quads.end(), condGeQ.quads.begin(), condGeQ.quads.end());
            quads.emplace_back(Quadruple{"if", condGeQ.result, "", label2.getName()});
            quads.emplace_back(Quadruple{"goto", "", "", label3.getName()});

            quads.emplace_back(label2);
            const auto bodyGeQ = body->toQuadruples(globalLabelId);
            const auto bodyQuads = bodyGeQ.quads;
            quads.insert(quads.end(), bodyQuads.begin(), bodyQuads.end());
            quads.emplace_back(Quadruple{"goto", "", "", label1.getName()});

            quads.emplace_back(label3);

            return { quads, "" };
        }

        TypeCheckResult typeCheck(const SymbolTableNode& symbolTableNode, const DataType assignedType) const override {
            SymbolTable symbolTable{};
            SymbolTableNode newSymbolTableNode = symbolTableNode.createChild(&symbolTable);
            const auto condResult = condExpr->typeCheck(newSymbolTableNode, DataType::NONE_T);
            if (std::holds_alternative<TypeCheckError>(condResult)) {
                return condResult;
            }
            if (!checkType(std::get<TypeCheckSuccess>(condResult).type, {DataType::BOOL_T, DataType::INT_T})) {
                return TypeCheckError{ "Condition must be boolean", condExpr->getWhere() };
            }
            const auto bodyResult = body->typeCheck(newSymbolTableNode, DataType::NONE_T);
            if (std::holds_alternative<TypeCheckError>(bodyResult)) {
                return bodyResult;
            }
            return TypeCheckSuccess{ DataType::NONE_T };
        }
}; // condExpr: AstNode, body: AstNode

export class ForStmt : public AstNode {
    private:
        std::unique_ptr<AstNode> forVarDecl;
        std::unique_ptr<AstNode> condExpr;
        std::unique_ptr<AstNode> incrExpr;
        std::unique_ptr<AstNode> body;

    public:
        ForStmt(std::unique_ptr<AstNode> forVarDecl, std::unique_ptr<AstNode> condExpr, std::unique_ptr<AstNode> incrExpr, std::unique_ptr<AstNode> body)
            : AstNode("ForStmt"), forVarDecl(std::move(forVarDecl)), condExpr(std::move(condExpr)), incrExpr(std::move(incrExpr)), body(std::move(body)) {}
        ~ForStmt() = default;

        std::string getWhere() const override {
            return forVarDecl->getWhere();
        }

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            const auto label1 = getLabel(globalLabelId);
            const auto label2 = getLabel(globalLabelId);
            const auto label3 = getLabel(globalLabelId);

            const auto varGeQ = forVarDecl->toQuadruples(globalLabelId);
            const auto varQuads = varGeQ.quads;
            quads.insert(quads.end(), varQuads.begin(), varQuads.end());

            quads.emplace_back(label1);
            const auto condGeQ = condExpr->toQuadruples(globalLabelId, intermediateId + 1);
            quads.insert(quads.end(), condGeQ.quads.begin(), condGeQ.quads.end());
            quads.emplace_back(Quadruple{"if", condGeQ.result, "", label2.getName()});
            quads.emplace_back(Quadruple{"goto", "", "", label3.getName()});

            quads.emplace_back(label2);
            const auto bodyGeQ = body->toQuadruples(globalLabelId);
            const auto bodyQuads = bodyGeQ.quads;
            quads.insert(quads.end(), bodyQuads.begin(), bodyQuads.end());
            const auto incrGeQ = incrExpr->toQuadruples(globalLabelId);
            const auto incrQuads = incrGeQ.quads;
            quads.insert(quads.end(), incrQuads.begin(), incrQuads.end());
            quads.emplace_back(Quadruple{"goto", "", "", label1.getName()});

            quads.emplace_back(label3);

            return { quads, "" };
        }

        TypeCheckResult typeCheck(const SymbolTableNode& symbolTableNode, const DataType assignedType) const override {
            SymbolTable symbolTable{};
            SymbolTableNode newSymbolTableNode = symbolTableNode.createChild(&symbolTable);
            const auto varResult = forVarDecl->typeCheck(newSymbolTableNode, DataType::NONE_T);
            if (std::holds_alternative<TypeCheckError>(varResult)) {
                return varResult;
            }
            const auto condResult = condExpr->typeCheck(newSymbolTableNode, DataType::NONE_T);
            if (std::holds_alternative<TypeCheckError>(condResult)) {
                return condResult;
            }
            if (!checkType(std::get<TypeCheckSuccess>(condResult).type, {DataType::BOOL_T, DataType::INT_T})) {
                return TypeCheckError{ "Condition must be boolean", condExpr->getWhere() };
            }
            const auto incrResult = incrExpr->typeCheck(newSymbolTableNode, DataType::NONE_T);
            if (std::holds_alternative<TypeCheckError>(incrResult)) {
                return incrResult;
            }
            const auto bodyResult = body->typeCheck(newSymbolTableNode, DataType::NONE_T);
            if (std::holds_alternative<TypeCheckError>(bodyResult)) {
                return bodyResult;
            }
            return TypeCheckSuccess{ DataType::NONE_T };
        }
}; // type?: Token, forVarDecl: AstNode[], condExpr: AstNode, incrExpr: AstNode

export class VarAssign : public AstNode {
    private:
        std::unique_ptr<AstNode> var;
        std::unique_ptr<AstNode> expr;

    public:
        VarAssign(std::unique_ptr<AstNode> var, std::unique_ptr<AstNode> expr)
            : AstNode("VarAssign"), var(std::move(var)), expr(std::move(expr)) {}
        ~VarAssign() = default;

        std::string getWhere() const override {
            return var->getWhere();
        }

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            const auto varGeQ = var->toQuadruples(globalLabelId);
            const auto varResult = varGeQ.result;

            const auto valueGeQ = expr->toQuadruples(globalLabelId, intermediateId + 1);
            const auto valueQuads = valueGeQ.quads;
            const auto valueResult = valueGeQ.result;
            quads.insert(quads.end(), valueQuads.begin(), valueQuads.end());
            quads.emplace_back(Quadruple{"=", valueResult, "", varResult});
            return { quads, "" };
        }

        TypeCheckResult typeCheck(const SymbolTableNode& symbolTableNode, const DataType assignedType) const override {
            const auto valueResult = expr->typeCheck(symbolTableNode, DataType::NONE_T);
            if (std::holds_alternative<TypeCheckError>(valueResult)) {
                return valueResult;
            }
            const auto valueType = std::get<TypeCheckSuccess>(valueResult).type;
            const auto varResult = var->typeCheck(symbolTableNode, assignedType);
            if (std::holds_alternative<TypeCheckError>(varResult)) {
                return varResult;
            }
            const auto varType = std::get<TypeCheckSuccess>(varResult).type;
            if (!typeEquals(varType, valueType)) {
                return TypeCheckError{ "Type mismatch: " + getTypeName(varType) + " and " + getTypeName(valueType), getWhere() };
            }
            return TypeCheckSuccess{ valueType };
        }
}; // var: AstNode, expr: AstNode

export class ForVarDecl : public AstNode {
    private:
        std::vector<std::unique_ptr<AstNode>> varAssigns;

    public:
        ForVarDecl(std::vector<std::unique_ptr<AstNode>> varAssigns): AstNode("ForVarDecl"), varAssigns(std::move(varAssigns)) {}
        ~ForVarDecl() = default;

        std::string getWhere() const override {
            return varAssigns[0]->getWhere();
        }

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            for (const auto& var : varAssigns) {
                const auto varGeQ = var->toQuadruples(globalLabelId);
                const auto varQuads = varGeQ.quads;
                quads.insert(quads.end(), varQuads.begin(), varQuads.end());
            }
            return { quads, "" };
        }

        TypeCheckResult typeCheck(const SymbolTableNode& symbolTableNode, const DataType assignedType) const override {
            // if (type) {
            //     const auto typeResult = (*type)->typeCheck(symbolTableNode, DataType::NONE_T);
            //     if (std::holds_alternative<TypeCheckError>(typeResult)) {
            //         return typeResult;
            //     }
            //     const auto typeType = std::get<TypeCheckSuccess>(typeResult).type;
            //     for (const auto& var : varAssigns) {
            //         const auto result = var->typeCheck(symbolTableNode, typeType);
            //         if (std::holds_alternative<TypeCheckError>(result)) {
            //             return result;
            //         }
            //     }
            // }
            for (const auto& var : varAssigns) {
                const auto result = var->typeCheck(symbolTableNode, DataType::NONE_T);
                if (std::holds_alternative<TypeCheckError>(result)) {
                    return result;
                }
            }
            return TypeCheckSuccess{ DataType::NONE_T };
        }
}; // type: Token, varAssigns: AstNode[]

export class ReturnStmt : public AstNode {
    private:
        std::optional<std::unique_ptr<AstNode>> expr;

    public:
        ReturnStmt(std::optional<std::unique_ptr<AstNode>> expr): AstNode("ReturnStmt"), expr(std::move(expr)) {}
        ~ReturnStmt() = default;

        std::string getWhere() const override {
            return expr ? (*expr)->getWhere() : "return";
        }

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            if (expr) {
                const auto valueGeQ = (*expr)->toQuadruples(globalLabelId, intermediateId + 1);
                quads.insert(quads.end(), valueGeQ.quads.begin(), valueGeQ.quads.end());
                quads.emplace_back(Quadruple{"RETURN", valueGeQ.result, "", ""});
            } else {
                quads.emplace_back(Quadruple{"RETURN", "", "", ""});
            }
            return { quads, "" };
        }

        TypeCheckResult typeCheck(const SymbolTableNode& symbolTableNode, const DataType assignedType) const override {
            if (expr) {
                const auto valueResult = (*expr)->typeCheck(symbolTableNode, DataType::NONE_T);
                if (std::holds_alternative<TypeCheckError>(valueResult)) {
                    return valueResult;
                }
                // No return type checking for now
            }
            return TypeCheckSuccess{ DataType::NONE_T };
        }
}; // expr?: AstNode

export class AssignExpr : public AstNode {
    private:
        std::unique_ptr<AstNode> var;
        std::unique_ptr<AstNode> expr;

    public:
        AssignExpr(std::unique_ptr<AstNode> var, std::unique_ptr<AstNode> expr)
            : AstNode("AssignExpr"), var(std::move(var)), expr(std::move(expr)) {}
        ~AssignExpr() = default;

        std::string getWhere() const override {
            return var->getWhere();
        }

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            const auto varGeQ = var->toQuadruples(globalLabelId);
            const auto varResult = varGeQ.result;

            const auto valueGeQ = expr->toQuadruples(globalLabelId, intermediateId + 1);
            const auto valueQuads = valueGeQ.quads;
            const auto valueResult = valueGeQ.result;
            quads.insert(quads.end(), valueQuads.begin(), valueQuads.end());
            quads.emplace_back(Quadruple{"=", valueResult, "", varResult});
            return { quads, varResult };
        }

        TypeCheckResult typeCheck(const SymbolTableNode& symbolTableNode, const DataType assignedType) const override {
            const auto varResult = var->typeCheck(symbolTableNode, DataType::NONE_T);
            if (std::holds_alternative<TypeCheckError>(varResult)) {
                return varResult;
            }
            const auto varType = std::get<TypeCheckSuccess>(varResult).type;
            const auto valueResult = expr->typeCheck(symbolTableNode, DataType::NONE_T);
            if (std::holds_alternative<TypeCheckError>(valueResult)) {
                return valueResult;
            }
            const auto valueType = std::get<TypeCheckSuccess>(valueResult).type;
            if (!typeEquals(varType, valueType)) {
                return TypeCheckError{ "Type mismatch: " + getTypeName(varType) + " and " + getTypeName(valueType), getWhere() };
            }
            return TypeCheckSuccess{ valueType };
        }
}; // var: AstNode, expr: AstNode

export class OrExpr : public AstNode {
    private:
        std::unique_ptr<AstNode> lexpr;
        std::unique_ptr<AstNode> rexpr;

    public:
        OrExpr(std::unique_ptr<AstNode> lexpr, std::unique_ptr<AstNode> rexpr)
            : AstNode("OrExpr"), lexpr(std::move(lexpr)), rexpr(std::move(rexpr)) {}
        ~OrExpr() = default;

        std::string getWhere() const override {
            return lexpr->getWhere();
        }

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            const auto lexprGeQ = lexpr->toQuadruples(globalLabelId, intermediateId + 1);
            const auto rexprGeQ = rexpr->toQuadruples(globalLabelId, intermediateId + 2);
            quads.insert(quads.end(), lexprGeQ.quads.begin(), lexprGeQ.quads.end());
            quads.insert(quads.end(), rexprGeQ.quads.begin(), rexprGeQ.quads.end());
            const auto intermediate = getIntermediate(intermediateId);
            quads.emplace_back(Quadruple{"||", lexprGeQ.result, rexprGeQ.result, intermediate});
            return { quads, intermediate };
        }

        TypeCheckResult typeCheck(const SymbolTableNode& symbolTableNode, const DataType assignedType) const override {
            const auto lexprResult = lexpr->typeCheck(symbolTableNode, DataType::NONE_T);
            if (std::holds_alternative<TypeCheckError>(lexprResult)) {
                return lexprResult;
            }
            const auto lexprType = std::get<TypeCheckSuccess>(lexprResult).type;
            if (!checkType(lexprType, {DataType::BOOL_T, DataType::INT_T})) {
                return TypeCheckError{ "Left operand must be boolean", lexpr->getWhere() };
            }
            const auto rexprResult = rexpr->typeCheck(symbolTableNode, DataType::NONE_T);
            if (std::holds_alternative<TypeCheckError>(rexprResult)) {
                return rexprResult;
            }
            const auto rexprType = std::get<TypeCheckSuccess>(rexprResult).type;
            if (!checkType(rexprType, {DataType::BOOL_T, DataType::INT_T})) {
                return TypeCheckError{ "Right operand must be boolean", rexpr->getWhere() };
            }
            return TypeCheckSuccess{ DataType::BOOL_T };
        }
}; // lexpr: AstNode, rexpr: AstNode

export class AndExpr : public AstNode {
    private:
        std::unique_ptr<AstNode> lexpr;
        std::unique_ptr<AstNode> rexpr;

    public:
        AndExpr(std::unique_ptr<AstNode> lexpr, std::unique_ptr<AstNode> rexpr)
            : AstNode("AndExpr"), lexpr(std::move(lexpr)), rexpr(std::move(rexpr)) {}
        ~AndExpr() = default;

        std::string getWhere() const override {
            return lexpr->getWhere();
        }

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            const auto lexprGeQ = lexpr->toQuadruples(globalLabelId, intermediateId + 1);
            const auto rexprGeQ = rexpr->toQuadruples(globalLabelId, intermediateId + 2);
            quads.insert(quads.end(), lexprGeQ.quads.begin(), lexprGeQ.quads.end());
            quads.insert(quads.end(), rexprGeQ.quads.begin(), rexprGeQ.quads.end());
            const auto intermediate = getIntermediate(intermediateId);
            quads.emplace_back(Quadruple{"&&", lexprGeQ.result, rexprGeQ.result, intermediate});
            return { quads, intermediate };
        }

        TypeCheckResult typeCheck(const SymbolTableNode& symbolTableNode, const DataType assignedType) const override {
            const auto lexprResult = lexpr->typeCheck(symbolTableNode, DataType::NONE_T);
            if (std::holds_alternative<TypeCheckError>(lexprResult)) {
                return lexprResult;
            }
            const auto lexprType = std::get<TypeCheckSuccess>(lexprResult).type;
            if (!checkType(lexprType, {DataType::BOOL_T, DataType::INT_T})) {
                return TypeCheckError{ "Left operand must be boolean", lexpr->getWhere() };
            }
            const auto rexprResult = rexpr->typeCheck(symbolTableNode, DataType::NONE_T);
            if (std::holds_alternative<TypeCheckError>(rexprResult)) {
                return rexprResult;
            }
            const auto rexprType = std::get<TypeCheckSuccess>(rexprResult).type;
            if (!checkType(rexprType, {DataType::BOOL_T, DataType::INT_T})) {
                return TypeCheckError{ "Right operand must be boolean", rexpr->getWhere() };
            }
            return TypeCheckSuccess{ DataType::BOOL_T };
        }
}; // lexpr: AstNode, rexpr: AstNode

export class EqualExpr : public AstNode {
    private:
        std::unique_ptr<AstNode> lexpr;
        std::unique_ptr<AstNode> rexpr;

    public:
        EqualExpr(std::unique_ptr<AstNode> lexpr, std::unique_ptr<AstNode> rexpr)
            : AstNode("EqualExpr"), lexpr(std::move(lexpr)), rexpr(std::move(rexpr)) {}
        ~EqualExpr() = default;

        std::string getWhere() const override {
            return lexpr->getWhere();
        }

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            const auto lexprGeQ = lexpr->toQuadruples(globalLabelId, intermediateId + 1);
            const auto rexprGeQ = rexpr->toQuadruples(globalLabelId, intermediateId + 2);
            quads.insert(quads.end(), lexprGeQ.quads.begin(), lexprGeQ.quads.end());
            quads.insert(quads.end(), rexprGeQ.quads.begin(), rexprGeQ.quads.end());
            const auto intermediate = getIntermediate(intermediateId);
            quads.emplace_back(Quadruple{"==", lexprGeQ.result, rexprGeQ.result, intermediate});
            return { quads, intermediate };
        }

        TypeCheckResult typeCheck(const SymbolTableNode& symbolTableNode, const DataType assignedType) const override {
            const auto lexprResult = lexpr->typeCheck(symbolTableNode, DataType::NONE_T);
            if (std::holds_alternative<TypeCheckError>(lexprResult)) {
                return lexprResult;
            }
            const auto lexprType = std::get<TypeCheckSuccess>(lexprResult).type;
            const auto rexprResult = rexpr->typeCheck(symbolTableNode, DataType::NONE_T);
            if (std::holds_alternative<TypeCheckError>(rexprResult)) {
                return rexprResult;
            }
            const auto rexprType = std::get<TypeCheckSuccess>(rexprResult).type;
            if (!(
                checkType(lexprType, {DataType::INT_T, DataType::FLOAT_T}) && checkType(rexprType, {DataType::INT_T, DataType::FLOAT_T}) ||
                checkType(lexprType, {DataType::STR_T}) && checkType(rexprType, {DataType::STR_T})
            )) {
                return TypeCheckError{ "Type mismatch in comparison", getWhere() };
            }
            return TypeCheckSuccess{ DataType::BOOL_T };
        }
}; // lexpr: AstNode, rexpr: AstNode

export class NotEqualExpr : public AstNode {
    private:
        std::unique_ptr<AstNode> lexpr;
        std::unique_ptr<AstNode> rexpr;

    public:
        NotEqualExpr(std::unique_ptr<AstNode> lexpr, std::unique_ptr<AstNode> rexpr)
            : AstNode("NotEqualExpr"), lexpr(std::move(lexpr)), rexpr(std::move(rexpr)) {}
        ~NotEqualExpr() = default;

        std::string getWhere() const override {
            return lexpr->getWhere();
        }

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            const auto lexprGeQ = lexpr->toQuadruples(globalLabelId, intermediateId + 1);
            const auto rexprGeQ = rexpr->toQuadruples(globalLabelId, intermediateId + 2);
            quads.insert(quads.end(), lexprGeQ.quads.begin(), lexprGeQ.quads.end());
            quads.insert(quads.end(), rexprGeQ.quads.begin(), rexprGeQ.quads.end());
            const auto intermediate = getIntermediate(intermediateId);
            quads.emplace_back(Quadruple{"!=", lexprGeQ.result, rexprGeQ.result, intermediate});
            return { quads, intermediate };
        }

        TypeCheckResult typeCheck(const SymbolTableNode& symbolTableNode, const DataType assignedType) const override {
            const auto lexprResult = lexpr->typeCheck(symbolTableNode, DataType::NONE_T);
            if (std::holds_alternative<TypeCheckError>(lexprResult)) {
                return lexprResult;
            }
            const auto lexprType = std::get<TypeCheckSuccess>(lexprResult).type;
            const auto rexprResult = rexpr->typeCheck(symbolTableNode, DataType::NONE_T);
            if (std::holds_alternative<TypeCheckError>(rexprResult)) {
                return rexprResult;
            }
            const auto rexprType = std::get<TypeCheckSuccess>(rexprResult).type;
            if (!(
                checkType(lexprType, {DataType::INT_T, DataType::FLOAT_T}) && checkType(rexprType, {DataType::INT_T, DataType::FLOAT_T}) ||
                checkType(lexprType, {DataType::STR_T}) && checkType(rexprType, {DataType::STR_T})
            )) {
                return TypeCheckError{ "Type mismatch in comparison", getWhere() };
            }
            return TypeCheckSuccess{ DataType::BOOL_T };
        }
}; // lexpr: AstNode, rexpr: AstNode

export class LessExpr : public AstNode {
    private:
        std::unique_ptr<AstNode> lexpr;
        std::unique_ptr<AstNode> rexpr;

    public:
        LessExpr(std::unique_ptr<AstNode> lexpr, std::unique_ptr<AstNode> rexpr)
            : AstNode("LessExpr"), lexpr(std::move(lexpr)), rexpr(std::move(rexpr)) {}
        ~LessExpr() = default;

        std::string getWhere() const override {
            return lexpr->getWhere();
        }

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            const auto lexprGeQ = lexpr->toQuadruples(globalLabelId, intermediateId + 1);
            const auto rexprGeQ = rexpr->toQuadruples(globalLabelId, intermediateId + 2);
            quads.insert(quads.end(), lexprGeQ.quads.begin(), lexprGeQ.quads.end());
            quads.insert(quads.end(), rexprGeQ.quads.begin(), rexprGeQ.quads.end());
            const auto intermediate = getIntermediate(intermediateId);
            quads.emplace_back(Quadruple{"<", lexprGeQ.result, rexprGeQ.result, intermediate});
            return { quads, intermediate };
        }

        TypeCheckResult typeCheck(const SymbolTableNode& symbolTableNode, const DataType assignedType) const override {
            const auto lexprResult = lexpr->typeCheck(symbolTableNode, DataType::NONE_T);
            if (std::holds_alternative<TypeCheckError>(lexprResult)) {
                return lexprResult;
            }
            const auto lexprType = std::get<TypeCheckSuccess>(lexprResult).type;
            const auto rexprResult = rexpr->typeCheck(symbolTableNode, DataType::NONE_T);
            if (std::holds_alternative<TypeCheckError>(rexprResult)) {
                return rexprResult;
            }
            const auto rexprType = std::get<TypeCheckSuccess>(rexprResult).type;
            if (!(
                checkType(lexprType, {DataType::INT_T, DataType::FLOAT_T}) && checkType(rexprType, {DataType::INT_T, DataType::FLOAT_T}) ||
                checkType(lexprType, {DataType::STR_T}) && checkType(rexprType, {DataType::STR_T})
            )) {
                return TypeCheckError{ "Type mismatch in comparison", getWhere() };
            }
            return TypeCheckSuccess{ DataType::BOOL_T };
        }
}; // lexpr: AstNode, rexpr: AstNode

export class LessEqualExpr : public AstNode {
    private:
        std::unique_ptr<AstNode> lexpr;
        std::unique_ptr<AstNode> rexpr;

    public:
        LessEqualExpr(std::unique_ptr<AstNode> lexpr, std::unique_ptr<AstNode> rexpr)
            : AstNode("LessEqualExpr"), lexpr(std::move(lexpr)), rexpr(std::move(rexpr)) {}
        ~LessEqualExpr() = default;

        std::string getWhere() const override {
            return lexpr->getWhere();
        }

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            const auto lexprGeQ = lexpr->toQuadruples(globalLabelId, intermediateId + 1);
            const auto rexprGeQ = rexpr->toQuadruples(globalLabelId, intermediateId + 2);
            quads.insert(quads.end(), lexprGeQ.quads.begin(), lexprGeQ.quads.end());
            quads.insert(quads.end(), rexprGeQ.quads.begin(), rexprGeQ.quads.end());
            const auto intermediate = getIntermediate(intermediateId);
            quads.emplace_back(Quadruple{"<=", lexprGeQ.result, rexprGeQ.result, intermediate});
            return { quads, intermediate };
        }

        TypeCheckResult typeCheck(const SymbolTableNode& symbolTableNode, const DataType assignedType) const override {
            const auto lexprResult = lexpr->typeCheck(symbolTableNode, DataType::NONE_T);
            if (std::holds_alternative<TypeCheckError>(lexprResult)) {
                return lexprResult;
            }
            const auto lexprType = std::get<TypeCheckSuccess>(lexprResult).type;
            const auto rexprResult = rexpr->typeCheck(symbolTableNode, DataType::NONE_T);
            if (std::holds_alternative<TypeCheckError>(rexprResult)) {
                return rexprResult;
            }
            const auto rexprType = std::get<TypeCheckSuccess>(rexprResult).type;
            if (!(
                checkType(lexprType, {DataType::INT_T, DataType::FLOAT_T}) && checkType(rexprType, {DataType::INT_T, DataType::FLOAT_T}) ||
                checkType(lexprType, {DataType::STR_T}) && checkType(rexprType, {DataType::STR_T})
            )) {
                return TypeCheckError{ "Type mismatch in comparison", getWhere() };
            }
            return TypeCheckSuccess{ DataType::BOOL_T };
        }
}; // lexpr: AstNode, rexpr: AstNode

export class GreaterExpr : public AstNode {
    private:
        std::unique_ptr<AstNode> lexpr;
        std::unique_ptr<AstNode> rexpr;

    public:
        GreaterExpr(std::unique_ptr<AstNode> lexpr, std::unique_ptr<AstNode> rexpr)
            : AstNode("GreaterExpr"), lexpr(std::move(lexpr)), rexpr(std::move(rexpr)) {}
        ~GreaterExpr() = default;

        std::string getWhere() const override {
            return lexpr->getWhere();
        }

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            const auto lexprGeQ = lexpr->toQuadruples(globalLabelId, intermediateId + 1);
            const auto rexprGeQ = rexpr->toQuadruples(globalLabelId, intermediateId + 2);
            quads.insert(quads.end(), lexprGeQ.quads.begin(), lexprGeQ.quads.end());
            quads.insert(quads.end(), rexprGeQ.quads.begin(), rexprGeQ.quads.end());
            const auto intermediate = getIntermediate(intermediateId);
            quads.emplace_back(Quadruple{">", lexprGeQ.result, rexprGeQ.result, intermediate});
            return { quads, intermediate };
        }

        TypeCheckResult typeCheck(const SymbolTableNode& symbolTableNode, const DataType assignedType) const override {
            const auto lexprResult = lexpr->typeCheck(symbolTableNode, DataType::NONE_T);
            if (std::holds_alternative<TypeCheckError>(lexprResult)) {
                return lexprResult;
            }
            const auto lexprType = std::get<TypeCheckSuccess>(lexprResult).type;
            const auto rexprResult = rexpr->typeCheck(symbolTableNode, DataType::NONE_T);
            if (std::holds_alternative<TypeCheckError>(rexprResult)) {
                return rexprResult;
            }
            const auto rexprType = std::get<TypeCheckSuccess>(rexprResult).type;
            if (!(
                checkType(lexprType, {DataType::INT_T, DataType::FLOAT_T}) && checkType(rexprType, {DataType::INT_T, DataType::FLOAT_T}) ||
                checkType(lexprType, {DataType::STR_T}) && checkType(rexprType, {DataType::STR_T})
            )) {
                return TypeCheckError{ "Type mismatch in comparison", getWhere() };
            }
            return TypeCheckSuccess{ DataType::BOOL_T };
        }
}; // lexpr: AstNode, rexpr: AstNode

export class GreaterEqualExpr : public AstNode {
    private:
        std::unique_ptr<AstNode> lexpr;
        std::unique_ptr<AstNode> rexpr;

    public:
        GreaterEqualExpr(std::unique_ptr<AstNode> lexpr, std::unique_ptr<AstNode> rexpr)
            : AstNode("GreaterEqualExpr"), lexpr(std::move(lexpr)), rexpr(std::move(rexpr)) {}
        ~GreaterEqualExpr() = default;

        std::string getWhere() const override {
            return lexpr->getWhere();
        }

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            const auto lexprGeQ = lexpr->toQuadruples(globalLabelId, intermediateId + 1);
            const auto rexprGeQ = rexpr->toQuadruples(globalLabelId, intermediateId + 2);
            quads.insert(quads.end(), lexprGeQ.quads.begin(), lexprGeQ.quads.end());
            quads.insert(quads.end(), rexprGeQ.quads.begin(), rexprGeQ.quads.end());
            const auto intermediate = getIntermediate(intermediateId);
            quads.emplace_back(Quadruple{">=", lexprGeQ.result, rexprGeQ.result, intermediate});
            return { quads, intermediate };
        }

        TypeCheckResult typeCheck(const SymbolTableNode& symbolTableNode, const DataType assignedType) const override {
            const auto lexprResult = lexpr->typeCheck(symbolTableNode, DataType::NONE_T);
            if (std::holds_alternative<TypeCheckError>(lexprResult)) {
                return lexprResult;
            }
            const auto lexprType = std::get<TypeCheckSuccess>(lexprResult).type;
            const auto rexprResult = rexpr->typeCheck(symbolTableNode, DataType::NONE_T);
            if (std::holds_alternative<TypeCheckError>(rexprResult)) {
                return rexprResult;
            }
            const auto rexprType = std::get<TypeCheckSuccess>(rexprResult).type;
            if (!(
                checkType(lexprType, {DataType::INT_T, DataType::FLOAT_T}) && checkType(rexprType, {DataType::INT_T, DataType::FLOAT_T}) ||
                checkType(lexprType, {DataType::STR_T}) && checkType(rexprType, {DataType::STR_T})
            )) {
                return TypeCheckError{ "Type mismatch in comparison", getWhere() };
            }
            return TypeCheckSuccess{ DataType::BOOL_T };
        }
}; // lexpr: AstNode, rexpr: AstNode

export class AddExpr : public AstNode {
    private:
        std::unique_ptr<AstNode> lexpr;
        std::unique_ptr<AstNode> rexpr;

    public:
        AddExpr(std::unique_ptr<AstNode> lexpr, std::unique_ptr<AstNode> rexpr)
            : AstNode("AddExpr"), lexpr(std::move(lexpr)), rexpr(std::move(rexpr)) {}
        ~AddExpr() = default;

        std::string getWhere() const override {
            return lexpr->getWhere();
        }

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            const auto lexprGeQ = lexpr->toQuadruples(globalLabelId, intermediateId + 1);
            const auto rexprGeQ = rexpr->toQuadruples(globalLabelId, intermediateId + 2);
            quads.insert(quads.end(), lexprGeQ.quads.begin(), lexprGeQ.quads.end());
            quads.insert(quads.end(), rexprGeQ.quads.begin(), rexprGeQ.quads.end());
            const auto intermediate = getIntermediate(intermediateId);
            quads.emplace_back(Quadruple{"+", lexprGeQ.result, rexprGeQ.result, intermediate});
            return { quads, intermediate };
        }

        TypeCheckResult typeCheck(const SymbolTableNode& symbolTableNode, const DataType assignedType) const override {
            const auto lexprResult = lexpr->typeCheck(symbolTableNode, DataType::NONE_T);
            if (std::holds_alternative<TypeCheckError>(lexprResult)) {
                return lexprResult;
            }
            const auto lexprType = std::get<TypeCheckSuccess>(lexprResult).type;
            const auto rexprResult = rexpr->typeCheck(symbolTableNode, DataType::NONE_T);
            if (std::holds_alternative<TypeCheckError>(rexprResult)) {
                return rexprResult;
            }
            const auto rexprType = std::get<TypeCheckSuccess>(rexprResult).type;
            if (!(
                checkType(lexprType, {DataType::INT_T, DataType::FLOAT_T}) && checkType(rexprType, {DataType::INT_T, DataType::FLOAT_T}) ||
                checkType(lexprType, {DataType::STR_T}) && checkType(rexprType, {DataType::STR_T})
            )) {
                return TypeCheckError{ "Cannot add types " + getTypeName(lexprType) + " and " + getTypeName(rexprType), getWhere() };
            }
            if (lexprType == DataType::FLOAT_T || rexprType == DataType::FLOAT_T) {
                return TypeCheckSuccess{ DataType::FLOAT_T };
            }
            else if (lexprType == DataType::STR_T || rexprType == DataType::STR_T) {
                return TypeCheckSuccess{ DataType::STR_T };
            }
            return TypeCheckSuccess{ DataType::INT_T };
        }
}; // lexpr: AstNode, rexpr: AstNode

export class SubExpr : public AstNode {
    private:
        std::unique_ptr<AstNode> lexpr;
        std::unique_ptr<AstNode> rexpr;

    public:
        SubExpr(std::unique_ptr<AstNode> lexpr, std::unique_ptr<AstNode> rexpr)
            : AstNode("SubExpr"), lexpr(std::move(lexpr)), rexpr(std::move(rexpr)) {}
        ~SubExpr() = default;

        std::string getWhere() const override {
            return lexpr->getWhere();
        }

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            const auto lexprGeQ = lexpr->toQuadruples(globalLabelId, intermediateId + 1);
            const auto rexprGeQ = rexpr->toQuadruples(globalLabelId, intermediateId + 2);
            quads.insert(quads.end(), lexprGeQ.quads.begin(), lexprGeQ.quads.end());
            quads.insert(quads.end(), rexprGeQ.quads.begin(), rexprGeQ.quads.end());
            const auto intermediate = getIntermediate(intermediateId);
            quads.emplace_back(Quadruple{"-", lexprGeQ.result, rexprGeQ.result, intermediate});
            return { quads, intermediate };
        }

        TypeCheckResult typeCheck(const SymbolTableNode& symbolTableNode, const DataType assignedType) const override {
            const auto lexprResult = lexpr->typeCheck(symbolTableNode, DataType::NONE_T);
            if (std::holds_alternative<TypeCheckError>(lexprResult)) {
                return lexprResult;
            }
            const auto lexprType = std::get<TypeCheckSuccess>(lexprResult).type;
            if (!checkType(lexprType, {DataType::INT_T, DataType::FLOAT_T})) {
                return TypeCheckError{ "The operands must be numeric", lexpr->getWhere() };
            }
            const auto rexprResult = rexpr->typeCheck(symbolTableNode, DataType::NONE_T);
            if (std::holds_alternative<TypeCheckError>(rexprResult)) {
                return rexprResult;
            }
            const auto rexprType = std::get<TypeCheckSuccess>(rexprResult).type;
            if (!checkType(rexprType, {DataType::INT_T, DataType::FLOAT_T})) {
                return TypeCheckError{ "The operands must be numeric", rexpr->getWhere() };
            }
            if (lexprType == DataType::FLOAT_T || rexprType == DataType::FLOAT_T) {
                return TypeCheckSuccess{ DataType::FLOAT_T };
            }
            return TypeCheckSuccess{ DataType::INT_T };
        }

}; // lexpr: AstNode, rexpr: AstNode
export class MulExpr : public AstNode {
    private:
        std::unique_ptr<AstNode> lexpr;
        std::unique_ptr<AstNode> rexpr;

    public:
        MulExpr(std::unique_ptr<AstNode> lexpr, std::unique_ptr<AstNode> rexpr)
            : AstNode("MulExpr"), lexpr(std::move(lexpr)), rexpr(std::move(rexpr)) {}
        ~MulExpr() = default;

        std::string getWhere() const override {
            return lexpr->getWhere();
        }

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            const auto lexprGeQ = lexpr->toQuadruples(globalLabelId, intermediateId + 1);
            const auto rexprGeQ = rexpr->toQuadruples(globalLabelId, intermediateId + 2);
            quads.insert(quads.end(), lexprGeQ.quads.begin(), lexprGeQ.quads.end());
            quads.insert(quads.end(), rexprGeQ.quads.begin(), rexprGeQ.quads.end());
            const auto intermediate = getIntermediate(intermediateId);
            quads.emplace_back(Quadruple{"*", lexprGeQ.result, rexprGeQ.result, intermediate});
            return { quads, intermediate };
        }

        TypeCheckResult typeCheck(const SymbolTableNode& symbolTableNode, const DataType assignedType) const override {
            const auto lexprResult = lexpr->typeCheck(symbolTableNode, DataType::NONE_T);
            if (std::holds_alternative<TypeCheckError>(lexprResult)) {
                return lexprResult;
            }
            const auto lexprType = std::get<TypeCheckSuccess>(lexprResult).type;
            if (!checkType(lexprType, {DataType::INT_T, DataType::FLOAT_T})) {
                return TypeCheckError{ "The operands must be numeric", lexpr->getWhere() };
            }
            const auto rexprResult = rexpr->typeCheck(symbolTableNode, DataType::NONE_T);
            if (std::holds_alternative<TypeCheckError>(rexprResult)) {
                return rexprResult;
            }
            const auto rexprType = std::get<TypeCheckSuccess>(rexprResult).type;
            if (!checkType(rexprType, {DataType::INT_T, DataType::FLOAT_T})) {
                return TypeCheckError{ "The operands must be numeric", rexpr->getWhere() };
            }
            if (lexprType == DataType::FLOAT_T || rexprType == DataType::FLOAT_T) {
                return TypeCheckSuccess{ DataType::FLOAT_T };
            }
            return TypeCheckSuccess{ DataType::INT_T };
        }
}; // lexpr: AstNode, rexpr: AstNode

export class DivExpr : public AstNode {
    private:
        std::unique_ptr<AstNode> lexpr;
        std::unique_ptr<AstNode> rexpr;

    public:
        DivExpr(std::unique_ptr<AstNode> lexpr, std::unique_ptr<AstNode> rexpr)
            : AstNode("DivExpr"), lexpr(std::move(lexpr)), rexpr(std::move(rexpr)) {}
        ~DivExpr() = default;

        std::string getWhere() const override {
            return lexpr->getWhere();
        }

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            const auto lexprGeQ = lexpr->toQuadruples(globalLabelId, intermediateId + 1);
            const auto rexprGeQ = rexpr->toQuadruples(globalLabelId, intermediateId + 2);
            quads.insert(quads.end(), lexprGeQ.quads.begin(), lexprGeQ.quads.end());
            quads.insert(quads.end(), rexprGeQ.quads.begin(), rexprGeQ.quads.end());
            const auto intermediate = getIntermediate(intermediateId);
            quads.emplace_back(Quadruple{"/", lexprGeQ.result, rexprGeQ.result, intermediate});
            return { quads, intermediate };
        }

        TypeCheckResult typeCheck(const SymbolTableNode& symbolTableNode, const DataType assignedType) const override {
            const auto lexprResult = lexpr->typeCheck(symbolTableNode, DataType::NONE_T);
            if (std::holds_alternative<TypeCheckError>(lexprResult)) {
                return lexprResult;
            }
            const auto lexprType = std::get<TypeCheckSuccess>(lexprResult).type;
            if (!checkType(lexprType, {DataType::INT_T, DataType::FLOAT_T})) {
                return TypeCheckError{ "The operands must be numeric", lexpr->getWhere() };
            }
            const auto rexprResult = rexpr->typeCheck(symbolTableNode, DataType::NONE_T);
            if (std::holds_alternative<TypeCheckError>(rexprResult)) {
                return rexprResult;
            }
            const auto rexprType = std::get<TypeCheckSuccess>(rexprResult).type;
            if (!checkType(rexprType, {DataType::INT_T, DataType::FLOAT_T})) {
                return TypeCheckError{ "The operands must be numeric", rexpr->getWhere() };
            }
            if (lexprType == DataType::FLOAT_T || rexprType == DataType::FLOAT_T) {
                return TypeCheckSuccess{ DataType::FLOAT_T };
            }
            return TypeCheckSuccess{ DataType::INT_T };
        }
}; // lexpr: AstNode, rexpr: AstNode

export class ModExpr : public AstNode {
    private:
        std::unique_ptr<AstNode> lexpr;
        std::unique_ptr<AstNode> rexpr;

    public:
        ModExpr(std::unique_ptr<AstNode> lexpr, std::unique_ptr<AstNode> rexpr)
            : AstNode("ModExpr"), lexpr(std::move(lexpr)), rexpr(std::move(rexpr)) {}
        ~ModExpr() = default;

        std::string getWhere() const override {
            return lexpr->getWhere();
        }

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            const auto lexprGeQ = lexpr->toQuadruples(globalLabelId, intermediateId + 1);
            const auto rexprGeQ = rexpr->toQuadruples(globalLabelId, intermediateId + 2);
            quads.insert(quads.end(), lexprGeQ.quads.begin(), lexprGeQ.quads.end());
            quads.insert(quads.end(), rexprGeQ.quads.begin(), rexprGeQ.quads.end());
            const auto intermediate = getIntermediate(intermediateId);
            quads.emplace_back(Quadruple{"%", lexprGeQ.result, rexprGeQ.result, intermediate});
            return { quads, intermediate };
        }

        TypeCheckResult typeCheck(const SymbolTableNode& symbolTableNode, const DataType assignedType) const override {
            const auto lexprResult = lexpr->typeCheck(symbolTableNode, DataType::NONE_T);
            if (std::holds_alternative<TypeCheckError>(lexprResult)) {
                return lexprResult;
            }
            const auto lexprType = std::get<TypeCheckSuccess>(lexprResult).type;
            if (!checkType(lexprType, {DataType::INT_T, DataType::FLOAT_T})) {
                return TypeCheckError{ "The operands must be numeric", lexpr->getWhere() };
            }
            const auto rexprResult = rexpr->typeCheck(symbolTableNode, DataType::NONE_T);
            if (std::holds_alternative<TypeCheckError>(rexprResult)) {
                return rexprResult;
            }
            const auto rexprType = std::get<TypeCheckSuccess>(rexprResult).type;
            if (!checkType(rexprType, {DataType::INT_T, DataType::FLOAT_T})) {
                return TypeCheckError{ "The operands must be numeric", rexpr->getWhere() };
            }
            if (lexprType == DataType::FLOAT_T || rexprType == DataType::FLOAT_T) {
                return TypeCheckSuccess{ DataType::FLOAT_T };
            }
            return TypeCheckSuccess{ DataType::INT_T };
        }
}; // lexpr: AstNode, rexpr: AstNode

export class UnaryPlusExpr : public AstNode {
    private:
        std::unique_ptr<AstNode> expr;

    public:
        UnaryPlusExpr(std::unique_ptr<AstNode> expr): AstNode("UnaryPlusExpr"), expr(std::move(expr)) {}
        ~UnaryPlusExpr() = default;

        std::string getWhere() const override {
            return expr->getWhere();
        }

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            const auto valueGeQ = expr->toQuadruples(globalLabelId, intermediateId + 1);
            quads.insert(quads.end(), valueGeQ.quads.begin(), valueGeQ.quads.end());
            const auto intermediate = getIntermediate(intermediateId);
            quads.emplace_back(Quadruple{"+", valueGeQ.result, "", intermediate});
            return { quads, intermediate };
        }

        TypeCheckResult typeCheck(const SymbolTableNode& symbolTableNode, const DataType assignedType) const override {
            const auto valueResult = expr->typeCheck(symbolTableNode, DataType::NONE_T);
            if (std::holds_alternative<TypeCheckError>(valueResult)) {
                return valueResult;
            }
            const auto valueType = std::get<TypeCheckSuccess>(valueResult).type;
            if (!checkType(valueType, {DataType::INT_T, DataType::FLOAT_T})) {
                return TypeCheckError{ "The operand must be numeric", expr->getWhere() };
            }
            return TypeCheckSuccess{ valueType };
        }
}; // expr: AstNode

export class UnaryMinusExpr : public AstNode {
    private:
        std::unique_ptr<AstNode> expr;

    public:
        UnaryMinusExpr(std::unique_ptr<AstNode> expr): AstNode("UnaryMinusExpr"), expr(std::move(expr)) {}
        ~UnaryMinusExpr() = default;

        std::string getWhere() const override {
            return expr->getWhere();
        }

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            const auto valueGeQ = expr->toQuadruples(globalLabelId, intermediateId + 1);
            quads.insert(quads.end(), valueGeQ.quads.begin(), valueGeQ.quads.end());
            const auto intermediate = getIntermediate(intermediateId);
            quads.emplace_back(Quadruple{"-", valueGeQ.result, "", intermediate});
            return { quads, intermediate };
        }

        TypeCheckResult typeCheck(const SymbolTableNode& symbolTableNode, const DataType assignedType) const override {
            const auto valueResult = expr->typeCheck(symbolTableNode, DataType::NONE_T);
            if (std::holds_alternative<TypeCheckError>(valueResult)) {
                return valueResult;
            }
            const auto valueType = std::get<TypeCheckSuccess>(valueResult).type;
            if (!checkType(valueType, {DataType::INT_T, DataType::FLOAT_T})) {
                return TypeCheckError{ "The operand must be numeric", expr->getWhere() };
            }
            return TypeCheckSuccess{ valueType };
        }
}; // expr: AstNode

export class NotExpr : public AstNode {
    private:
        std::unique_ptr<AstNode> expr;

    public:
        NotExpr(std::unique_ptr<AstNode> expr): AstNode("NotExpr"), expr(std::move(expr)) {}
        ~NotExpr() = default;

        std::string getWhere() const override {
            return expr->getWhere();
        }

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            const auto valueGeQ = expr->toQuadruples(globalLabelId, intermediateId + 1);
            quads.insert(quads.end(), valueGeQ.quads.begin(), valueGeQ.quads.end());
            const auto intermediate = getIntermediate(intermediateId);
            quads.emplace_back(Quadruple{"-", valueGeQ.result, "", intermediate});
            return { quads, intermediate };
        }

        TypeCheckResult typeCheck(const SymbolTableNode& symbolTableNode, const DataType assignedType) const override {
            const auto valueResult = expr->typeCheck(symbolTableNode, DataType::NONE_T);
            if (std::holds_alternative<TypeCheckError>(valueResult)) {
                return valueResult;
            }
            const auto valueType = std::get<TypeCheckSuccess>(valueResult).type;
            if (!checkType(valueType, {DataType::BOOL_T, DataType::INT_T})) {
                return TypeCheckError{ "The operand must be boolean", expr->getWhere() };
            }
            return TypeCheckSuccess{ DataType::BOOL_T };
        }
}; // expr: AstNode

export class FuncCall : public AstNode {
    private:
        Token id;
        std::vector<std::unique_ptr<AstNode>> exprs;

    public:
        FuncCall(Token id, std::vector<std::unique_ptr<AstNode>> arguments): AstNode("FuncCall"), id(id), exprs(std::move(arguments)) {}
        ~FuncCall() = default;

        std::string getWhere() const override {
            return id.getPosition();
        }

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            for (int i = exprs.size() - 1; i >= 0; --i) {
                const auto argGeQ = exprs[i]->toQuadruples(globalLabelId, intermediateId + 1);
                quads.insert(quads.end(), argGeQ.quads.begin(), argGeQ.quads.end());
                quads.emplace_back(Quadruple{"PUSH", argGeQ.result, "", ""});
            }
            const auto intermediate = getIntermediate(intermediateId);
            return { quads, intermediate };
        }

        TypeCheckResult typeCheck(const SymbolTableNode& symbolTableNode, const DataType assignedType) const override {
            const auto entry = findSymbol(symbolTableNode, id.getValue());
            if (!entry.has_value()) {
                return TypeCheckError{ "Function not found", id.getPosition() };
            }
            if (entry.value().type != DataType::FUNC_T) {
                return TypeCheckError{ "Function call on a non-function", id.getPosition() };
            }
            for (const auto& arg : exprs) {
                const auto argResult = arg->typeCheck(symbolTableNode, DataType::NONE_T);
                if (std::holds_alternative<TypeCheckError>(argResult)) {
                    return argResult;
                }
                // No argument type check for now
            }
            // Function call returns the any type for now
            return TypeCheckSuccess{ DataType::ANY_T };
        }
}; // id: Token, exprs: AstNode[]
